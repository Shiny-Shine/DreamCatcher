// Scoped unsaved mission regression tests; reuse the existing stage test world/fixtures.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "AI/DCEnemySpawner.h"
#include "AI/DCGASEnemyCharacter.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Stage/DCEncounterController.h"
#include "Stage/DCStageDirector.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCStageFlowTestTypes.h"
#include "Tests/DCEnemyGASTestTypes.h"
#include "TimerManager.h"
#include "UI/Stage/DCMissionListData.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

struct FDCMissionTestAccess
{
	static void Configure(ADCEnemySpawner* S, int32 Count, bool bConfigured = true)
	{ S->SpawnCount = Count; S->GASEnemyClass = bConfigured ? ADCGASEnemyCharacter::StaticClass() : nullptr; }
	static void Spawners(ADCEncounterController* E, const TArray<ADCEnemySpawner*>& Values)
	{ E->Spawners.Reset(); for (ADCEnemySpawner* S : Values) { E->Spawners.Add(S); } }
	static void Track(ADCEnemySpawner* S, AActor* A) { S->TrackEnemy(A); }
	static void DeathNotification(ADCEnemySpawner* S, AActor* A) { S->ConsumeEnemyDeath(A); }
	static void AdoptForAbort(ADCEnemySpawner* S, AActor* A)
	{ S->TrackEnemy(A); S->OwnedGASNPCs.Add(A, TWeakObjectPtr<AController>()); }
	static TArray<FDCStagePhaseDefinition>& Phases(ADCStageDirector* S) { return S->Phases; }
	static void Completion(ADCStageDirector* S, ADCEncounterController* E) { S->HandleEncounterCompleted(E); }
	static bool AdvancePending(ADCStageDirector* S) { return S->GetWorldTimerManager().IsTimerPending(S->PhaseTimerHandle); }
	static ADCR10EnemyFixture* LivingEnemy(ADCEnemySpawner* S)
	{
		for (const TWeakObjectPtr<AActor>& Actor : S->LivingEnemies)
		{ if (ADCR10EnemyFixture* Enemy = Cast<ADCR10EnemyFixture>(Actor.Get())) { return Enemy; } }
		return nullptr;
	}
};

namespace DCMissionTests
{
struct FFixture
{
	FTestWorldWrapper World;
	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCMissionIsolatedAutomation")))
		{ Test.AddError(TEXT("Use a separate unattended editor with -DCMissionIsolatedAutomation; saves no assets.")); return false; }
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR11ModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return true;
	}
	template<class T> T* Spawn()
	{
		FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform::Identity, Params);
	}
	ADCR11DirectorFixture* Director(UDCMissionListData* List, ADCEncounterController* Encounter = nullptr)
	{
		FDCStagePhaseDefinition Phase; Phase.PhaseType = Encounter ? EDCStagePhaseType::Encounter : EDCStagePhaseType::Delay;
		Phase.PhaseDelay = 60.f; Phase.Encounter = Encounter; Phase.MissionList = List;
		ADCR11DirectorFixture* S = Spawn<ADCR11DirectorFixture>();
		if (S) { S->Configure({Phase}); }
		return S;
	}
	USceneComponent* Root(AActor* Actor)
	{
		USceneComponent* Component = NewObject<USceneComponent>(Actor, NAME_None, RF_Transient);
		Actor->AddInstanceComponent(Component); Actor->SetRootComponent(Component); Component->RegisterComponent();
		return Component;
	}
	ADCR11EncounterFixture* Combat(ADCR10SpawnerFixture*& Spawner)
	{
		Spawner = Spawn<ADCR10SpawnerFixture>();
		ADCR11EncounterFixture* Encounter = Spawn<ADCR11EncounterFixture>();
		if (Spawner) { Spawner->ConfigureGAS(ADCR10EnemyFixture::StaticClass(), 1); }
		if (Encounter) { Encounter->Configure(0.f, Spawner); }
		return Encounter;
	}
};

FDCMissionDefinition Definition(FName Id, EDCMissionProgressSource Source, FName TargetKey = NAME_None)
{
	FDCMissionDefinition D; D.MissionId = Id; D.DisplayText = FText::FromName(Id);
	D.ProgressSource = Source; D.TargetKey = TargetKey; D.TargetCount = 4; return D;
}
const FDCMissionProgress* Find(const TArray<FDCMissionProgress>& Rows, FName Id)
{ return Rows.FindByPredicate([Id](const FDCMissionProgress& R) { return R.MissionId == Id; }); }

bool LethalDamage(ADCR10EnemyFixture* Enemy)
{
	// The existing R10 fixture uses the real HealthSet and original Death ability.
	UAbilitySystemComponent* ASC = IsValid(Enemy) && Enemy->IsGASReady() ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC)) { return false; }
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UDCHealthSet::GetDamageAttribute(); Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(1000.f);
	FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionCounterTest, "DreamCatcher.UI.Mission.CountsAndDeathGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionCounterTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCEnemySpawner* S = F.Spawn<ADCEnemySpawner>(); ADCEnemySpawner* Empty = F.Spawn<ADCEnemySpawner>();
	ADCR11EncounterFixture* E = F.Spawn<ADCR11EncounterFixture>();
	if (!TestNotNull(TEXT("Spawner"), S) || !TestNotNull(TEXT("Optional spawner"), Empty) || !TestNotNull(TEXT("Encounter"), E)) { return false; }
	FDCMissionTestAccess::Configure(S, 5); FDCMissionTestAccess::Configure(Empty, 100, false);
	FDCMissionTestAccess::Spawners(E, {S, S, nullptr, Empty});
	int32 Defeated, Total; E->GetMissionEnemyProgress(Defeated, Total);
	TestEqual(TEXT("Unspawned planned enemies count once"), Total, 5);
	TestEqual(TEXT("Planning creates no enemies"), S->GetSpawnedCount(), 0);
	TestEqual(TEXT("No death yet"), Defeated, 0);
	TestEqual(TEXT("Unconfigured optional spawn route plans zero"), Empty->GetPlannedEnemyCount(), 0);
	FTestWorldWrapper Other;
	if (!Other.CreateTestWorld(EWorldType::Game)) { Other.ForwardErrorMessages(this); return false; }
	ADCEnemySpawner* Foreign = Other.GetTestWorld()->SpawnActor<ADCEnemySpawner>();
	if (!TestNotNull(TEXT("Foreign-world spawner"), Foreign)) { return false; }
	FDCMissionTestAccess::Configure(Foreign, 20);
	FDCMissionTestAccess::Spawners(E, {S, S, Empty, Foreign}); E->GetMissionEnemyProgress(Defeated, Total);
	TestEqual(TEXT("Foreign-world configuration never contributes"), Total, 5);
	AActor* Killed = F.Spawn<AActor>(); AActor* Destroyed = F.Spawn<AActor>();
	if (!TestNotNull(TEXT("Death delivery actor"), Killed) || !TestNotNull(TEXT("Destroy actor"), Destroyed)) { return false; }
	FDCMissionTestAccess::Track(S, Killed); FDCMissionTestAccess::Track(S, Destroyed);
	// Exercise the actual production removal guard with duplicate notification.
	FDCMissionTestAccess::DeathNotification(S, Killed); FDCMissionTestAccess::DeathNotification(S, Killed);
	E->GetMissionEnemyProgress(Defeated, Total);
	TestEqual(TEXT("One guarded removal is one defeat"), Defeated, 1);
	TestEqual(TEXT("Unspawned portion stays in denominator"), Total, 5);
	AddExpectedError(TEXT("tracked enemy destroyed without a death notification"), EAutomationExpectedErrorFlags::Contains, 1);
	Destroyed->Destroy();
	TestEqual(TEXT("Destroy is not a defeat"), S->GetDefeatedCount(), 1);
	TestTrue(TEXT("Original unexpected-destroy failure remains"), S->HasSpawnFailure());
	ADCEnemySpawner* Owned = F.Spawn<ADCEnemySpawner>(); ADCR11EncounterFixture* Abort = F.Spawn<ADCR11EncounterFixture>();
	AActor* OwnedActor = F.Spawn<AActor>();
	if (!TestNotNull(TEXT("Owned spawner"), Owned) || !TestNotNull(TEXT("Abort encounter"), Abort) || !TestNotNull(TEXT("Owned actor"), OwnedActor)) { return false; }
	FDCMissionTestAccess::Configure(Owned, 3); FDCMissionTestAccess::AdoptForAbort(Owned, OwnedActor);
	Abort->Configure(60.f, Owned); Abort->StartEncounter(); Abort->AbortEncounter();
	F.World.TickTestWorld(.01f); F.World.TickTestWorld(.03f);
	TestEqual(TEXT("Abort ownership cleanup is not a defeat"), Owned->GetDefeatedCount(), 0);
	TestTrue(TEXT("Original abort removes configured spawner"), Owned->IsActorBeingDestroyed());
	TestTrue(TEXT("Original deferred cleanup removes owned actor"), OwnedActor->IsActorBeingDestroyed());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionEditingTest, "DreamCatcher.UI.Mission.DefinitionsAndRuntimeEditing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionEditingTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>(GetTransientPackage(), NAME_None, RF_Transient));
	TArray<FText> Errors;
	TestTrue(TEXT("Empty reusable asset is valid"), Data->ValidateDefinitions(Errors));
	Data->Missions = {Definition(TEXT("External"), EDCMissionProgressSource::External), Definition(TEXT("Guide"), EDCMissionProgressSource::Guidance, TEXT("Point"))};
	Data->DefaultTrackedMissionId = TEXT("External");
	ADCR11DirectorFixture* S = F.Director(Data.Get()); if (!TestNotNull(TEXT("Director"), S)) { return false; }
	S->StartStage();
	TestTrue(TEXT("External producer accepted"), S->UpdateExternalMissionProgress(TEXT("External"), 3));
	TestFalse(TEXT("Guidance has no inferred completion"), S->UpdateExternalMissionProgress(TEXT("Guide"), 1));
	TArray<FDCMissionProgress> Rows = S->GetMissionProgress();
	if (!TestEqual(TEXT("Two enabled definitions"), Rows.Num(), 2)) { return false; }
	TestEqual(TEXT("Authoring order"), Rows[0].MissionId, FName(TEXT("External")));
	TestEqual(TEXT("Runtime external count"), Rows[0].CurrentCount, 3);
	TestEqual(TEXT("Definition target is immutable runtime metadata"), Data->Missions[0].TargetCount, 4);
	S->UpdateExternalMissionProgress(TEXT("External"), 100);
	Rows = S->GetMissionProgress(); TestEqual(TEXT("External count bounded by its authored target"), Rows[0].CurrentCount, 4);
	S->SetExternalMissionCompleted(TEXT("External"), false);
	Rows = S->GetMissionProgress(); TestEqual(TEXT("Explicit external reset"), Rows[0].CurrentCount, 0);
	S->UpdateExternalMissionProgress(TEXT("External"), 3);
	Data->Missions.Swap(0, 1); Data->Missions[1].DisplayText = FText::FromString(TEXT("Edited"));
	Rows = S->GetMissionProgress();
	TestEqual(TEXT("Reordering follows definitions"), Rows[0].MissionId, FName(TEXT("Guide")));
	TestEqual(TEXT("Stable ID retains state after reorder"), Rows[1].CurrentCount, 3);
	TestEqual(TEXT("Edited text reaches snapshot"), Rows[1].DisplayText.ToString(), FString(TEXT("Edited")));
	TestTrue(TEXT("Explicit untrack"), S->SetTrackedMission(NAME_None));
	Rows = S->GetMissionProgress(); TestFalse(TEXT("Default is not reapplied every refresh"), Rows[1].bTracked);
	TestTrue(TEXT("Explicit tracking"), S->SetTrackedMission(TEXT("External")));
	const FDCMissionDefinition Saved = Data->Missions[1]; Data->Missions.RemoveAt(1);
	Rows = S->GetMissionProgress(); TestEqual(TEXT("Removed row vanishes"), Rows.Num(), 1);
	Data->Missions.Add(Saved); Rows = S->GetMissionProgress();
	TestEqual(TEXT("Deleted external state cannot resurrect"), Rows[1].CurrentCount, 0);
	TestFalse(TEXT("Deleted tracking cannot resurrect"), Rows[1].bTracked);
	S->UpdateExternalMissionProgress(TEXT("External"), 3); Data->Missions[1].bEnabled = false;
	S->GetMissionProgress(); Data->Missions[1].bEnabled = true; Rows = S->GetMissionProgress();
	TestEqual(TEXT("Disabled state is also discarded"), Rows[1].CurrentCount, 0);
	Data->Missions.Add(Saved);
	TestFalse(TEXT("Duplicate stable ID rejected by editor validation"), Data->ValidateDefinitions(Errors));
	Rows = S->GetMissionProgress(); TestEqual(TEXT("Both ambiguous copies excluded"), Rows.Num(), 1);
	TestFalse(TEXT("Ambiguous tracking rejected"), S->SetTrackedMission(TEXT("External")));
	Data->Missions.Pop(); Data->Missions[1].TargetCount = 0;
	TestFalse(TEXT("Non-positive external target rejected"), Data->ValidateDefinitions(Errors));
	Data->Missions[1].TargetCount = 4; Data->Missions[0].TargetKey = NAME_None;
	TestFalse(TEXT("Guidance requires an explicit target key"), Data->ValidateDefinitions(Errors));
	Data->Missions[0].TargetKey = TEXT("Point"); Data->DefaultTrackedMissionId = TEXT("Missing");
	TestFalse(TEXT("Missing default tracked identity rejected"), Data->ValidateDefinitions(Errors));
	Data->Missions[1].MissionId = NAME_None;
	TestFalse(TEXT("None identity rejected"), Data->ValidateDefinitions(Errors));
	Data->HUDStyle.PanelWidth = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Malformed UI geometry is detectable for HUD fallback"), Data->HUDStyle.IsValid());
	TestTrue(TEXT("Bad presentation never fails or advances combat"), S->GetRunState() == EDCStageRunState::Running && S->GetCurrentPhaseIndex() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionPhaseTest, "DreamCatcher.UI.Mission.PhaseResetAndCombatIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionPhaseTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>());
	Data->Missions = {Definition(TEXT("Count"), EDCMissionProgressSource::CurrentEncounterDefeats), Definition(TEXT("External"), EDCMissionProgressSource::External)};
	Data->DefaultTrackedMissionId = TEXT("External");
	ADCEnemySpawner* Current = F.Spawn<ADCEnemySpawner>(); ADCEnemySpawner* Future = F.Spawn<ADCEnemySpawner>();
	ADCR11EncounterFixture* E = F.Spawn<ADCR11EncounterFixture>();
	if (!TestNotNull(TEXT("Current spawner"), Current) || !TestNotNull(TEXT("Future spawner"), Future) || !TestNotNull(TEXT("Encounter"), E)) { return false; }
	FDCMissionTestAccess::Configure(Current, 5); FDCMissionTestAccess::Configure(Future, 20);
	E->Configure(60.f, Current);
	ADCR11DirectorFixture* S = F.Director(Data.Get(), E); if (!TestNotNull(TEXT("Director"), S)) { return false; }
	S->StartStage();
	TArray<FDCMissionProgress> Rows = S->GetMissionProgress();
	if (!TestEqual(TEXT("Current phase rows"), Rows.Num(), 2)) { return false; }
	TestEqual(TEXT("Only current encounter is counted"), Rows[0].TargetCount, 5);
	TestTrue(TEXT("External completion accepted"), S->SetExternalMissionCompleted(TEXT("External"), true));
	Rows = S->GetMissionProgress(); TestTrue(TEXT("External completion snapshot"), Rows[1].bCompleted);
	TestFalse(TEXT("Mission completion never completes encounter"), E->IsEncounterCompleted());
	TestEqual(TEXT("Mission completion never advances phase"), S->GetCurrentPhaseIndex(), 0);
	TestFalse(TEXT("Encounter counter cannot be set externally"), S->SetExternalMissionCompleted(TEXT("Count"), true));
	S->FailStage();
	TestTrue(TEXT("Stage end clears mission state"), S->GetMissionProgress().IsEmpty() && !S->GetCurrentMissionList());
	TestFalse(TEXT("Ended phase rejects external changes"), S->UpdateExternalMissionProgress(TEXT("External"), 1));
	FDCStagePhaseDefinition A; A.PhaseType = EDCStagePhaseType::Delay; A.PhaseDelay = .02f; A.MissionList = Data.Get();
	FDCStagePhaseDefinition B = A; B.PhaseDelay = 60.f;
	ADCR11DirectorFixture* Delays = F.Spawn<ADCR11DirectorFixture>();
	if (!TestNotNull(TEXT("Delay director"), Delays)) { return false; }
	Delays->Configure({A, B}); Delays->StartStage(); Delays->SetExternalMissionCompleted(TEXT("External"), true);
	// Arm pending timers first, expire the delay, then deliver the original
	// QueueNextPhase next-tick callback. Do not assume first-tick activation.
	F.World.TickTestWorld(.01f); F.World.TickTestWorld(.03f); F.World.TickTestWorld(.01f);
	Rows = Delays->GetMissionProgress();
	if (!TestEqual(TEXT("Same asset in the next phase has two rows"), Rows.Num(), 2)) { return false; }
	TestEqual(TEXT("Original timer advances phase"), Delays->GetCurrentPhaseIndex(), 1);
	TestEqual(TEXT("Same asset does not share runtime progress across phases"), Rows[1].CurrentCount, 0);
	TestTrue(TEXT("New phase applies default tracking once"), Rows[1].bTracked);
	// The original optional-spawner route still clears a combat phase even when
	// its mission definition is invalid. UI validation never gates ValidatePhases.
	ADCEnemySpawner* Optional = F.Spawn<ADCEnemySpawner>(); ADCR11EncounterFixture* Empty = F.Spawn<ADCR11EncounterFixture>();
	if (!TestNotNull(TEXT("Optional route"), Optional) || !TestNotNull(TEXT("Empty encounter"), Empty)) { return false; }
	Empty->Configure(0, Optional); Data->Missions[0].MissionId = NAME_None;
	ADCR11DirectorFixture* Clear = F.Director(Data.Get(), Empty);
	if (!TestNotNull(TEXT("Optional combat director"), Clear)) { return false; }
	Clear->StartStage();
	F.World.TickTestWorld(.01f); F.World.TickTestWorld(.01f);
	TestTrue(TEXT("Optional no-class spawner preserves original immediate clear"), Optional->IsSpawnerCleared());
	TestTrue(TEXT("Invalid UI definitions do not block actual stage completion"), Clear->GetRunState() == EDCStageRunState::Completed);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionTargetTest, "DreamCatcher.UI.Mission.TargetBindingIdentityAndWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionTargetTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>());
	Data->Missions = {Definition(TEXT("Guide"), EDCMissionProgressSource::Guidance, TEXT("Point"))};
	Data->DefaultTrackedMissionId = TEXT("Guide");
	ADCR11DirectorFixture* S = F.Director(Data.Get()); AActor* Target = F.Spawn<AActor>();
	if (!TestNotNull(TEXT("Director"), S) || !TestNotNull(TEXT("Target"), Target)) { return false; }
	USceneComponent* Root = F.Root(Target);
	FDCMissionTargetBinding Binding; Binding.TargetKey = TEXT("Point"); Binding.TargetActor = Target;
	FDCMissionTestAccess::Phases(S)[0].MissionTargets.Add(Binding); S->StartStage();
	TestTrue(TEXT("World-origin root is a valid actual target component"), Root->GetComponentLocation().IsZero() && S->GetTrackedMissionTargetComponent() == Root);
	TArray<FText> Errors;
	TestTrue(TEXT("Read-only binding validation accepts actual world-origin root"), S->ValidateMissionBindings(Errors));
	FDCMissionTestAccess::Phases(S)[0].MissionTargets.Reset();
	TestFalse(TEXT("Missing enabled mission binding is reported"), S->ValidateMissionBindings(Errors));
	FDCMissionTestAccess::Phases(S)[0].MissionTargets.Add(Binding);
	FDCMissionTestAccess::Phases(S)[0].MissionTargets.Add(Binding);
	TestNull(TEXT("Duplicate target binding is ambiguous"), S->GetTrackedMissionTargetComponent());
	TestFalse(TEXT("Duplicate binding key is reported"), S->ValidateMissionBindings(Errors));
	FDCMissionTestAccess::Phases(S)[0].MissionTargets.Pop();
	FDCMissionTestAccess::Phases(S)[0].MissionTargets[0].TargetKey = NAME_None;
	TestFalse(TEXT("Empty binding key is reported"), S->ValidateMissionBindings(Errors));
	FDCMissionTestAccess::Phases(S)[0].MissionTargets[0].TargetKey = TEXT("Point");
	S->SetTrackedMission(NAME_None); TestNull(TEXT("Untracking clears radar input"), S->GetTrackedMissionTargetComponent());
	S->SetTrackedMission(TEXT("Guide")); TestTrue(TEXT("Tracking returns the exact actor root identity"), S->GetTrackedMissionTargetComponent() == Root);
	AActor* NoRoot = F.Spawn<AActor>();
	FDCMissionTestAccess::Phases(S)[0].MissionTargets[0].TargetActor = NoRoot;
	TestNull(TEXT("Rootless logical actor does not become a fake origin target"), S->GetTrackedMissionTargetComponent());
	TestFalse(TEXT("Rootless target is reported for editor configuration"), S->ValidateMissionBindings(Errors));
	FTestWorldWrapper Other;
	if (!Other.CreateTestWorld(EWorldType::Game)) { Other.ForwardErrorMessages(this); return false; }
	AActor* Foreign = Other.GetTestWorld()->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Foreign actor"), Foreign)) { return false; }
	F.Root(Foreign); FDCMissionTestAccess::Phases(S)[0].MissionTargets[0].TargetActor = Foreign;
	TestNull(TEXT("Foreign-world binding rejected"), S->GetTrackedMissionTargetComponent());
	TestFalse(TEXT("Foreign-world target is reported for editor configuration"), S->ValidateMissionBindings(Errors));
	FDCMissionTestAccess::Phases(S)[0].MissionTargets[0].TargetActor = Target;
	Target->Destroy(); TestNull(TEXT("Destroyed target rejected"), S->GetTrackedMissionTargetComponent());
	S->FailStage(); TestNull(TEXT("Terminal phase has no target"), S->GetTrackedMissionTargetComponent());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionTransitionTest, "DreamCatcher.UI.Mission.TransitionSnapshotAndNextTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionTransitionTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FDCMissionHUDStyle Style;
	TestEqual(TEXT("Default completed hold"), Style.CompletedHoldDuration, .6f);
	TestEqual(TEXT("Default total transition duration"), Style.MissionTransitionDuration, .4f);
	Style.CompletedHoldDuration = 0.f; TestTrue(TEXT("Zero hold remains valid"), Style.IsValid());
	for (float Invalid : {-1.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
	{
		FDCMissionHUDStyle Bad; Bad.CompletedHoldDuration = Invalid;
		TestFalse(TEXT("Hold must be finite and nonnegative"), Bad.IsValid());
	}
	for (float Invalid : {0.f, -1.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
	{
		FDCMissionHUDStyle Bad; Bad.MissionTransitionDuration = Invalid;
		TestFalse(TEXT("Fade must be finite and positive"), Bad.IsValid());
	}
	FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>());
	Data->Missions = {Definition(TEXT("Guide"), EDCMissionProgressSource::Guidance, TEXT("Point")),
		Definition(TEXT("Count"), EDCMissionProgressSource::CurrentEncounterDefeats),
		Definition(TEXT("External"), EDCMissionProgressSource::External)};
	Data->DefaultTrackedMissionId = TEXT("Count");
	ADCR10SpawnerFixture *FirstSpawner = nullptr, *SecondSpawner = nullptr;
	ADCR11EncounterFixture* First = F.Combat(FirstSpawner); ADCR11EncounterFixture* Second = F.Combat(SecondSpawner);
	if (!TestNotNull(TEXT("First encounter"), First) || !TestNotNull(TEXT("Second encounter"), Second)
		|| !TestNotNull(TEXT("First spawner"), FirstSpawner) || !TestNotNull(TEXT("Second spawner"), SecondSpawner)) { return false; }
	ADCR11DirectorFixture* S = F.Director(Data.Get(), First); if (!TestNotNull(TEXT("Director"), S)) { return false; }
	FDCMissionTestAccess::Phases(S)[0].PhaseTitle = FText::FromString(TEXT("First fight"));
	// Copy before adding: never pass an array's own element into a reallocating Add.
	FDCStagePhaseDefinition Next = FDCMissionTestAccess::Phases(S)[0];
	Next.Encounter = Second; Next.PhaseTitle = FText::FromString(TEXT("Second fight"));
	FDCMissionTestAccess::Phases(S).Add(Next);
	FDCStagePhaseDefinition Finish; Finish.PhaseType = EDCStagePhaseType::Complete;
	FDCMissionTestAccess::Phases(S).Add(Finish);
	TArray<FDCMissionTransitionSnapshot> Seen;
	const FDelegateHandle Listener = S->OnMissionPhaseCompleted.AddLambda([&](ADCStageDirector* Source, const FDCMissionTransitionSnapshot& Snapshot)
	{
		Seen.Add(Snapshot);
		TestNull(TEXT("Callback runs after the original phase reset"), Source->GetCurrentMissionList());
		TestNull(TEXT("No old target survives the transition notification"), Source->GetTrackedMissionTargetComponent());
		if (Snapshot.PhaseIndex == 0) { TestEqual(TEXT("No next spawn inside death delegate stack"), SecondSpawner->GetSpawnedCount(), 0); }
	});
	S->StartStage(); S->UpdateExternalMissionProgress(TEXT("External"), 2);
	ADCR10EnemyFixture* Enemy = FDCMissionTestAccess::LivingEnemy(FirstSpawner);
	if (!TestNotNull(TEXT("Actual spawned GAS enemy"), Enemy)) { return false; }
	auto DeathReplay = Enemy->OnEnemyDeath; auto CompletionReplay = First->OnEncounterCompleted;
	TestTrue(TEXT("Apply real lethal Instant GE"), LethalDamage(Enemy));
	if (!TestEqual(TEXT("One final death emits once"), Seen.Num(), 1)) { return false; }
	const FDCMissionTransitionSnapshot Final = Seen[0];
	TestEqual(TEXT("Completed phase identity"), Final.PhaseIndex, 0);
	if (!TestEqual(TEXT("Whole ordered list retained"), Final.Missions.Num(), 3)) { return false; }
	TestEqual(TEXT("Guidance remains first"), Final.Missions[0].MissionId, FName(TEXT("Guide")));
	TestFalse(TEXT("Guidance completion is never invented"), Final.Missions[0].bCompleted);
	TestEqual(TEXT("Defeat row remains second"), Final.Missions[1].MissionId, FName(TEXT("Count")));
	TestTrue(TEXT("Actual final count is 1/1 completed"), Final.Missions[1].CurrentCount == 1 && Final.Missions[1].TargetCount == 1 && Final.Missions[1].bCompleted);
	TestTrue(TEXT("Tracked presentation value retained"), Final.Missions[1].bTracked);
	TestEqual(TEXT("Unrelated external runtime value retained"), Final.Missions[2].CurrentCount, 2);
	TestFalse(TEXT("Incomplete external remains incomplete"), Final.Missions[2].bCompleted);
	DeathReplay.Broadcast(Enemy); CompletionReplay.Broadcast(First); CompletionReplay.Broadcast(Second);
	TestEqual(TEXT("Duplicate and foreign callbacks suppressed"), Seen.Num(), 1);
	Data->Missions[1].DisplayText = FText::FromString(TEXT("Edited")); Data->HUDStyle.CompletedHoldDuration = 3.f;
	FDCMissionTestAccess::Phases(S)[0].PhaseTitle = FText::FromString(TEXT("Edited title"));
	TestEqual(TEXT("Snapshot text is independent"), Final.Missions[1].DisplayText.ToString(), FString(TEXT("Count")));
	TestEqual(TEXT("Snapshot phase title is independent"), Final.PhaseTitle.ToString(), FString(TEXT("First fight")));
	TestEqual(TEXT("Snapshot style is independent"), Final.HUDStyle.CompletedHoldDuration, .6f);
	TestEqual(TEXT("No synchronous phase advance"), S->GetCurrentPhaseIndex(), 0);
	if (FDCMissionTestAccess::AdvancePending(S))
	{
		F.World.TickTestWorld(.01f);
		TestEqual(TEXT("Pending next-tick activation does not advance early"), S->GetCurrentPhaseIndex(), 0);
	}
	F.World.TickTestWorld(.01f);
	TestEqual(TEXT("Original next timer tick ignores UI duration"), S->GetCurrentPhaseIndex(), 1);
	const TArray<FDCMissionProgress> Rows = S->GetMissionProgress();
	if (!TestEqual(TEXT("Same definitions reused in next phase"), Rows.Num(), 3)) { return false; }
	TestTrue(TEXT("New encounter starts at 0/1"), Rows[1].CurrentCount == 0 && Rows[1].TargetCount == 1);
	TestEqual(TEXT("External state is reset"), Rows[2].CurrentCount, 0);
	TestEqual(TEXT("Next phase reads latest definition"), Rows[1].DisplayText.ToString(), FString(TEXT("Edited")));
	CompletionReplay.Broadcast(First); TestEqual(TEXT("Old phase cannot replay into new phase"), Seen.Num(), 1);
	Enemy = FDCMissionTestAccess::LivingEnemy(SecondSpawner);
	if (!TestNotNull(TEXT("Actual next enemy"), Enemy)) { return false; }
	TestTrue(TEXT("Second real lethal GE"), LethalDamage(Enemy));
	TestEqual(TEXT("Second completion emits once"), Seen.Num(), 2);
	if (FDCMissionTestAccess::AdvancePending(S)) { F.World.TickTestWorld(.01f); }
	F.World.TickTestWorld(.01f);
	TestTrue(TEXT("Result timing is unchanged"), S->GetRunState() == EDCStageRunState::Completed);
	TestEqual(TEXT("Final copy remains intact after result"), Final.Missions[1].CurrentCount, 1);
	TestTrue(TEXT("No additional enemies spawned"), FirstSpawner->GetSpawnedCount() == 1 && SecondSpawner->GetSpawnedCount() == 1);
	S->OnMissionPhaseCompleted.Remove(Listener);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionTransitionGuardsTest, "DreamCatcher.UI.Mission.TransitionGuardsAndReentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionTransitionGuardsTest::RunTest(const FString&)
{
	using namespace DCMissionTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>());
	Data->Missions = {Definition(TEXT("Count"), EDCMissionProgressSource::CurrentEncounterDefeats)};
	for (int32 Action = 0; Action < 3; ++Action)
	{
		ADCR10SpawnerFixture* Spawner = nullptr; ADCR11EncounterFixture* E = F.Combat(Spawner);
		ADCR11DirectorFixture* S = F.Director(Data.Get(), E);
		if (!TestNotNull(TEXT("Guard stage"), S) || !TestNotNull(TEXT("Guard encounter"), E) || !TestNotNull(TEXT("Guard spawner"), Spawner)) { return false; }
		int32 Calls = 0;
		const FDelegateHandle H = S->OnMissionPhaseCompleted.AddLambda([&](ADCStageDirector*, const FDCMissionTransitionSnapshot&) { ++Calls; });
		S->StartStage(); ADCR10EnemyFixture* Enemy = FDCMissionTestAccess::LivingEnemy(Spawner);
		FDCMissionTestAccess::Completion(S, E); TestEqual(TEXT("Forged live completion ignored"), Calls, 0);
		if (Action == 0) { S->FailStage(); } else if (Action == 1) { E->AbortEncounter(); } else { E->Destroy(); }
		FDCMissionTestAccess::Completion(S, E);
		TestEqual(TEXT("Failure, abort and destruction never emit a completed view"), Calls, 0);
		S->OnMissionPhaseCompleted.Remove(H);
		if (!S->IsActorBeingDestroyed()) { S->Destroy(); }
		if (!Spawner->IsActorBeingDestroyed()) { Spawner->Destroy(); }
		if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) { Enemy->Destroy(); }
	}
	for (int32 Action = 0; Action < 2; ++Action)
	{
		ADCR10SpawnerFixture *Spawner = nullptr, *FutureSpawner = nullptr;
		ADCR11EncounterFixture* E = F.Combat(Spawner); ADCR11EncounterFixture* Future = F.Combat(FutureSpawner);
		ADCR11DirectorFixture* S = F.Director(Data.Get(), E);
		if (!TestNotNull(TEXT("Reentrant stage"), S) || !TestNotNull(TEXT("Reentrant encounter"), E) || !TestNotNull(TEXT("Reentrant spawner"), Spawner)
			|| !TestNotNull(TEXT("Future encounter"), Future) || !TestNotNull(TEXT("Future spawner"), FutureSpawner)) { return false; }
		FDCStagePhaseDefinition Next = FDCMissionTestAccess::Phases(S)[0]; Next.Encounter = Future;
		FDCMissionTestAccess::Phases(S).Add(Next);
		int32 Calls = 0; FDCMissionTransitionSnapshot Copy;
		const FDelegateHandle H = S->OnMissionPhaseCompleted.AddLambda([&](ADCStageDirector* Source, const FDCMissionTransitionSnapshot& Snapshot)
		{
			++Calls; Copy = Snapshot;
			TestNull(TEXT("Reset is committed before callback reentry"), Source->GetCurrentMissionList());
			if (Action == 0) { Source->FailStage(); } else { Source->Destroy(); }
		});
		S->StartStage(); ADCR10EnemyFixture* Enemy = FDCMissionTestAccess::LivingEnemy(Spawner);
		if (!TestNotNull(TEXT("Reentrant actual enemy"), Enemy)) { return false; }
		TestTrue(TEXT("Actual death enters receiver"), LethalDamage(Enemy));
		TestEqual(TEXT("Reentrant callback delivered once"), Calls, 1); S->OnMissionPhaseCompleted.Remove(H);
		if (!TestEqual(TEXT("Receiver owns final row values"), Copy.Missions.Num(), 1)) { return false; }
		TestEqual(TEXT("Copied completion survives FailStage/Destroy"), Copy.Missions[0].CurrentCount, 1);
		F.World.TickTestWorld(.01f); F.World.TickTestWorld(.01f);
		TestEqual(TEXT("Reentrant end cancels next spawn"), FutureSpawner->GetSpawnedCount(), 0);
		if (Action == 0) { TestTrue(TEXT("Reentrant failure stays terminal"), S->GetRunState() == EDCStageRunState::Failed); }
		else { TestTrue(TEXT("Reentrant destroy is safe"), S->IsActorBeingDestroyed()); }
		if (!S->IsActorBeingDestroyed()) { S->Destroy(); }
		if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) { Enemy->Destroy(); }
	}
	return !HasAnyErrors();
}

#endif
