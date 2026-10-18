// Unsaved radar tests. Reuse the existing Lyra GAS, stage and boss fixtures.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "UI/Radar/DCRadarWidget.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Teams/DCTeamAgentInterface.h"
#include "Teams/DCTeamSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCBossTestTypes.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "Tests/DCStageUITestTypes.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

struct FDCRadarTestAccess
{
	static bool Project(const UDCRadarWidget* Radar, FVector Delta, double Yaw, EDCRadarMarkerKind Kind, FDCRadarMarker& Out)
	{ return Radar->ProjectOffset(Delta, Yaw, Kind, Out); }
	static void Construct(UDCRadarWidget* Radar) { Radar->NativeConstruct(); }
	static void Destruct(UDCRadarWidget* Radar) { Radar->NativeDestruct(); }
	static void Sync(UDCRadarWidget* Radar) { Radar->SynchronizeContext(); }
	static void Refresh(UDCRadarWidget* Radar) { Radar->RefreshCandidates(); }
	static bool Enemy(const UDCRadarWidget* Radar, APawn* Pawn) { return Radar->IsEnemy(Pawn); }
	static bool HasTimer(const UDCRadarWidget* Radar)
	{ return Radar->TrackedWorld.IsValid() && Radar->TrackedWorld->GetTimerManager().TimerExists(Radar->RefreshTimer); }
	static int32 CandidateCount(const UDCRadarWidget* Radar) { return Radar->Candidates.Num(); }
	static bool HasTrackedWorld(const UDCRadarWidget* Radar) { return Radar->TrackedWorld.IsValid(); }
	static FTimerHandle Timer(const UDCRadarWidget* Radar) { return Radar->RefreshTimer; }
};

namespace DCRadarTests
{
bool Context(FAutomationTestBase& Test)
{
	// EditorContext registration and this guard agree: no commandlet-only demand.
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCRadarIsolatedAutomation"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")))
	{
		Test.AddError(TEXT("Use a separate unattended editor with -DCRadarIsolatedAutomation -DCLyraRuntimeDiagnostics."));
		return false;
	}
	return true;
}

struct FFixture
{
	FTestWorldWrapper World;
	TStrongObjectPtr<ULocalPlayer> LocalPlayer;
	TStrongObjectPtr<UDCRadarWidget> Radar;
	ADCR10TeamPlayerControllerFixture* PC = nullptr;
	APawn* Observer = nullptr;
	ADCR13StageModeFixture* Mode = nullptr;

	~FFixture()
	{
		if (Radar.IsValid()) { FDCRadarTestAccess::Destruct(Radar.Get()); Radar.Reset(); }
		if (LocalPlayer.IsValid()) { LocalPlayer->PlayerController = nullptr; }
		if (IsValid(PC)) { PC->Player = nullptr; }
	}
	template<class T> T* Spawn(FVector Location = FVector::ZeroVector)
	{
		FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform(Location), Params);
	}
	USceneComponent* Root(AActor* Actor, FVector Location)
	{
		USceneComponent* Component = NewObject<USceneComponent>(Actor, NAME_None, RF_Transient);
		Actor->AddInstanceComponent(Component); Actor->SetRootComponent(Component);
		Component->RegisterComponent(); Actor->SetActorLocation(Location);
		return Component;
	}
	APawn* PlainPawn(int32 Team, FVector Location = FVector::ZeroVector, bool bInitializeHealth = true)
	{
		APawn* Pawn = Spawn<APawn>();
		Root(Pawn, Location);
		ADCPlayerState* State = Spawn<ADCPlayerState>();
		State->SetGenericTeamId(IntegerToGenericTeamId(Team));
		Pawn->SetPlayerState(State);
		UDCLyraHealthComponent* Health = NewObject<UDCLyraHealthComponent>(Pawn, NAME_None, RF_Transient);
		Pawn->AddInstanceComponent(Health); Health->RegisterComponent();
		if (bInitializeHealth)
		{
			UDCAbilitySystemComponent* ASC = NewObject<UDCAbilitySystemComponent>(Pawn, NAME_None, RF_Transient);
			Pawn->AddInstanceComponent(ASC); ASC->RegisterComponent();
			ASC->AddAttributeSetSubobject(NewObject<UDCHealthSet>(Pawn, NAME_None, RF_Transient));
			ASC->InitAbilityActorInfo(Pawn, Pawn);
			Health->InitializeWithAbilitySystem(ASC);
		}
		return Pawn;
	}
	bool Start(FAutomationTestBase& Test)
	{
		if (!Context(Test)) { return false; }
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR13StageModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Mode = World.GetTestWorld()->GetAuthGameMode<ADCR13StageModeFixture>();
		PC = Spawn<ADCR10TeamPlayerControllerFixture>();
		if (!Test.TestNotNull(TEXT("Stage fixture"), Mode) || !Test.TestNotNull(TEXT("Controller fixture"), PC)) { return false; }
		ADCPlayerState* State = Spawn<ADCPlayerState>(); State->SetGenericTeamId(FGenericTeamId(0));
		PC->AssignFixturePlayerState(State);
		Observer = PlainPawn(0);
		PC->Possess(Observer);
		// Context only: no PlayerAdded/subsystem startup, viewport or user settings.
		LocalPlayer.Reset(NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient));
		LocalPlayer->PlayerController = PC; PC->Player = LocalPlayer.Get();
		Radar.Reset(NewObject<UDCRadarWidget>(World.GetTestWorld()->GetGameInstance(), NAME_None, RF_Transient));
		Radar->SetOwningPlayer(PC);
		if (!Test.TestNotNull(TEXT("Actual camera manager"), PC->PlayerCameraManager.Get())) { return false; }
		Radar->Initialize();
		FDCRadarTestAccess::Construct(Radar.Get());
		return Test.TestTrue(TEXT("Passive radar has a live view"), Radar->IsRadarActive());
	}
};

bool HasActor(const TArray<FDCRadarMarker>& Markers, const AActor* Actor, EDCRadarMarkerKind Kind)
{
	return Markers.ContainsByPredicate([Actor, Kind](const FDCRadarMarker& Marker)
	{ return Marker.Kind == Kind && Marker.ActorPath == Actor->GetPathName(); });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRadarProjectionTest, "DreamCatcher.UI.Radar.ProjectionAndBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRadarProjectionTest::RunTest(const FString&)
{
	if (!DCRadarTests::Context(*this)) { return false; }
	TStrongObjectPtr<UDCRadarWidget> Radar(NewObject<UDCRadarWidget>());
	struct FCase { FVector Delta; double Yaw; FVector2D Expected; };
	// Independently tabulated 50m scale: (120 - 4 - half diagonal of 24px).
	const FCase Cases[] = {
		{FVector(2500, 0, 0), 0, FVector2D(120, 70.4852813742)},
		{FVector(-2500, 0, 0), 0, FVector2D(120, 169.5147186258)},
		{FVector(0, 2500, 0), 0, FVector2D(169.5147186258, 120)},
		{FVector(0, -2500, 0), 0, FVector2D(70.4852813742, 120)},
		{FVector(1000, 0, 0), 45, FVector2D(105.9951226765, 105.9951226765)},
		{FVector(1000, 0, 0), 90, FVector2D(100.1941125497, 120)},
		{FVector(1000, 0, 0), 180, FVector2D(120, 139.8058874503)},
		{FVector(0, 2500, 1000000000), 0, FVector2D(169.5147186258, 120)}
	};
	for (const FCase& Case : Cases)
	{
		FDCRadarMarker Enemy, Objective;
		TestTrue(TEXT("Valid horizontal projection"), FDCRadarTestAccess::Project(Radar.Get(), Case.Delta, Case.Yaw, EDCRadarMarkerKind::Enemy, Enemy));
		TestTrue(TEXT("Independent expected direction/distance"), Enemy.Center.Equals(Case.Expected, .001));
		TestTrue(TEXT("Same world position also projects as objective"), FDCRadarTestAccess::Project(Radar.Get(), Case.Delta, Case.Yaw, EDCRadarMarkerKind::Objective, Objective));
		TestTrue(TEXT("Marker types share one world scale"), Enemy.Center.Equals(Objective.Center, .0001));
		TestEqual(TEXT("Enemy icon size"), Enemy.SizePixels, 12.f);
		TestEqual(TEXT("Objective icon size"), Objective.SizePixels, 24.f);
	}
	FDCRadarMarker Marker;
	TestTrue(TEXT("50m minus 1cm visible"), FDCRadarTestAccess::Project(Radar.Get(), FVector(4999, 0, 0), 0, EDCRadarMarkerKind::Enemy, Marker));
	TestTrue(TEXT("50m boundary included"), FDCRadarTestAccess::Project(Radar.Get(), FVector(5000, 0, 0), 0, EDCRadarMarkerKind::Enemy, Marker));
	TestFalse(TEXT("50m plus 1cm hidden"), FDCRadarTestAccess::Project(Radar.Get(), FVector(5001, 0, 0), 0, EDCRadarMarkerKind::Enemy, Marker));
	TestTrue(TEXT("XY diagonal boundary included"), FDCRadarTestAccess::Project(Radar.Get(), FVector(3000, 4000, 1000000), 0, EDCRadarMarkerKind::Enemy, Marker));
	TestTrue(TEXT("Distant objective retained"), FDCRadarTestAccess::Project(Radar.Get(), FVector(100000, 100000, 0), 0, EDCRadarMarkerKind::Objective, Marker));
	TestTrue(TEXT("Objective marks edge clamp"), Marker.bClamped);
	TestTrue(TEXT("Clamp retains NE direction"), FMath::IsNearlyEqual(Marker.Center.X - 120, 120 - Marker.Center.Y, .001));
	for (const FVector2D Corner : {FVector2D(-12, -12), FVector2D(-12, 12), FVector2D(12, -12), FVector2D(12, 12)})
	{
		TestTrue(TEXT("Every icon corner fits circle and padding"), (Marker.Center + Corner - FVector2D(120)).Length() <= 116.001);
	}
	TestTrue(TEXT("Zero-distance objective is valid"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 90, EDCRadarMarkerKind::Objective, Marker));
	TestTrue(TEXT("Zero-distance objective centered"), Marker.Center.Equals(FVector2D(120), .001) && !Marker.bClamped);
	Radar->PlayerIconSize = 20.f; Radar->EnemyIconSize = 18.f; Radar->ObjectiveIconSize = 40.f;
	FDCRadarMarker LargeObjective, ChangedEnemy;
	TestTrue(TEXT("Configured objective size projects"), FDCRadarTestAccess::Project(Radar.Get(), FVector(2500, 0, 0), 0, EDCRadarMarkerKind::Objective, LargeObjective));
	TestTrue(TEXT("Configured enemy size projects"), FDCRadarTestAccess::Project(Radar.Get(), FVector(2500, 0, 0), 0, EDCRadarMarkerKind::Enemy, ChangedEnemy));
	TestEqual(TEXT("Configured objective size returned"), LargeObjective.SizePixels, 40.f);
	TestEqual(TEXT("Configured enemy size returned"), ChangedEnemy.SizePixels, 18.f);
	TestTrue(TEXT("Largest icon controls shared scale"), LargeObjective.Center.Equals(FVector2D(120, 76.1421356237), .001)
		&& ChangedEnemy.Center.Equals(LargeObjective.Center, .001));
	Radar->EnemyIconSize = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("NaN icon size rejected"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 0, EDCRadarMarkerKind::Enemy, Marker));
	Radar->EnemyIconSize = 12.f; Radar->ObjectiveIconSize = 200.f;
	TestFalse(TEXT("Icon too large for circle rejected"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 0, EDCRadarMarkerKind::Objective, Marker));
	Radar->PlayerIconSize = 24.f; Radar->ObjectiveIconSize = 24.f;
	Radar->RangeMeters = 0;
	TestFalse(TEXT("Zero range hides safely"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 0, EDCRadarMarkerKind::Enemy, Marker));
	Radar->RangeMeters = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("NaN setting hides safely"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 0, EDCRadarMarkerKind::Enemy, Marker));
	Radar->RangeMeters = 50;
	TestFalse(TEXT("NaN yaw hides safely"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, std::numeric_limits<double>::quiet_NaN(), EDCRadarMarkerKind::Enemy, Marker));
	TestFalse(TEXT("NaN target hides safely"), FDCRadarTestAccess::Project(Radar.Get(), FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), 0, EDCRadarMarkerKind::Enemy, Marker));
	Radar->InnerPadding = 120;
	TestFalse(TEXT("Impossible circle padding hides safely"), FDCRadarTestAccess::Project(Radar.Get(), FVector::ZeroVector, 0, EDCRadarMarkerKind::Enemy, Marker));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRadarTeamHealthTest, "DreamCatcher.UI.Radar.LyraTeamAndHealthFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRadarTeamHealthTest::RunTest(const FString&)
{
	using namespace DCRadarTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	F.Radar->PlayerIconSize = 20.f;
	TestEqual(TEXT("Player marker uses configured size"), F.Radar->GetRadarMarkers()[0].SizePixels, 20.f);
	APawn* Enemy = F.PlainPawn(1, FVector(1000, 0, 0));
	APawn* Friend = F.PlainPawn(0);
	APawn* NoTeam = F.PlainPawn(INDEX_NONE);
	APawn* Uninitialized = F.PlainPawn(1, FVector::ZeroVector, false);
	APawn* Dying = F.PlainPawn(1);
	UDCLyraHealthComponent::FindHealthComponent(Dying)->StartDeath();
	APawn* ZeroHealth = F.PlainPawn(1);
	ZeroHealth->FindComponentByClass<UDCAbilitySystemComponent>()->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 0.f);
	ADCR10EnemyFixture* NPC = F.Spawn<ADCR10EnemyFixture>(FVector(0, 1000, 96));
	ADCR12BossFixture* Boss = F.Spawn<ADCR12BossFixture>(FVector(0, -1000, 96));
	if (!TestNotNull(TEXT("NPC"), NPC) || !TestNotNull(TEXT("Boss"), Boss)) { return false; }
	TestTrue(TEXT("Existing NPC health initialized"), NPC->IsGASReady());
	TestTrue(TEXT("Existing boss health initialized"), Boss->IsGASReady());
	TestTrue(TEXT("Plain Pawn uses actual Lyra team subsystem"), F.World.GetTestWorld()->GetSubsystem<UDCTeamSubsystem>()->CompareTeams(F.Observer, Enemy) == EDCTeamComparison::DifferentTeams);
	TestTrue(TEXT("Plain Pawn enemy; no BotController type requirement"), FDCRadarTestAccess::Enemy(F.Radar.Get(), Enemy));
	TestFalse(TEXT("Self excluded"), FDCRadarTestAccess::Enemy(F.Radar.Get(), F.Observer));
	for (APawn* Hidden : {Friend, NoTeam, Uninitialized, Dying, ZeroHealth})
	{
		TestFalse(TEXT("Team/health excluded"), FDCRadarTestAccess::Enemy(F.Radar.Get(), Hidden));
	}
	// SetTimer starts pending outside TimerManager::Tick; the first tick activates
	// it relative to InternalTime. Then allow the actual 0.2-second interval.
	F.World.TickTestWorld(.01f);
	TestEqual(TEXT("Pending timer does not discover early"), FDCRadarTestAccess::CandidateCount(F.Radar.Get()), 0);
	F.World.TickTestWorld(.25f);
	TestTrue(TEXT("Timer discovers real Pawns"), FDCRadarTestAccess::CandidateCount(F.Radar.Get()) >= 3);
	const TArray<FDCRadarMarker> Markers = F.Radar->GetRadarMarkers();
	TestTrue(TEXT("Ordinary Pawn included"), HasActor(Markers, Enemy, EDCRadarMarkerKind::Enemy));
	TestTrue(TEXT("NPC included"), HasActor(Markers, NPC, EDCRadarMarkerKind::Enemy));
	TestTrue(TEXT("Boss included without a BotController"), HasActor(Markers, Boss, EDCRadarMarkerKind::Enemy));
	Enemy->GetPlayerState<ADCPlayerState>()->SetGenericTeamId(FGenericTeamId(0));
	TestFalse(TEXT("Team change hidden before next world scan"), HasActor(F.Radar->GetRadarMarkers(), Enemy, EDCRadarMarkerKind::Enemy));
	Enemy->GetPlayerState<ADCPlayerState>()->SetGenericTeamId(FGenericTeamId(1));
	Enemy->Destroy();
	TestFalse(TEXT("Destroyed Pawn removed from cached display"), HasActor(F.Radar->GetRadarMarkers(), Enemy, EDCRadarMarkerKind::Enemy));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRadarLifetimeTest, "DreamCatcher.UI.Radar.ObjectiveAndContextLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRadarLifetimeTest::RunTest(const FString&)
{
	using namespace DCRadarTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	TestNull(TEXT("Default has no objective"), F.Radar->GetObjectiveComponent());
	AActor* Goal = F.Spawn<AActor>();
	USceneComponent* GoalComponent = F.Root(Goal, FVector::ZeroVector);
	F.Radar->SetObjectiveComponent(GoalComponent);
	TestTrue(TEXT("Real objective at world origin accepted"), F.Radar->GetObjectiveComponent() == GoalComponent);
	TestTrue(TEXT("Origin marker is present"), HasActor(F.Radar->GetRadarMarkers(), Goal, EDCRadarMarkerKind::Objective));
	F.Radar->ClearObjective();
	TestNull(TEXT("Explicit clear"), F.Radar->GetObjectiveComponent());
	F.Radar->SetObjectiveComponent(NewObject<USceneComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	TestNull(TEXT("Ownerless component rejected"), F.Radar->GetObjectiveComponent());
	F.Radar->SetObjectiveComponent(nullptr);
	TestNull(TEXT("Null component stays hidden"), F.Radar->GetObjectiveComponent());
	F.Radar->SetObjectiveComponent(GoalComponent);
	GoalComponent->DestroyComponent();
	TestNull(TEXT("Destroyed component hidden immediately"), F.Radar->GetObjectiveComponent());
	TestFalse(TEXT("Destroyed component has no marker"), HasActor(F.Radar->GetRadarMarkers(), Goal, EDCRadarMarkerKind::Objective));
	AActor* NextGoal = F.Spawn<AActor>();
	USceneComponent* NextComponent = F.Root(NextGoal, FVector(10000, 10000, 0));
	F.Radar->SetObjectiveComponent(NextComponent);
	TestTrue(TEXT("Own timer established"), FDCRadarTestAccess::HasTimer(F.Radar.Get()));
	APawn* NewPawn = F.PlainPawn(0);
	F.PC->Possess(NewPawn);
	TestFalse(TEXT("Pawn replacement hides stale data before synchronization"), F.Radar->IsRadarActive());
	FDCRadarTestAccess::Sync(F.Radar.Get());
	TestNull(TEXT("Pawn replacement clears old objective"), F.Radar->GetObjectiveComponent());
	TestEqual(TEXT("Pawn replacement clears old candidates"), FDCRadarTestAccess::CandidateCount(F.Radar.Get()), 0);
	TestTrue(TEXT("New Pawn can display"), F.Radar->IsRadarActive());
	F.Radar->SetObjectiveComponent(NextComponent);
	UDCLyraHealthComponent::FindHealthComponent(NewPawn)->StartDeath();
	TestTrue(TEXT("Death hides all markers immediately"), F.Radar->GetRadarMarkers().IsEmpty());
	FDCRadarTestAccess::Sync(F.Radar.Get());
	TestFalse(TEXT("Death stops own timer"), FDCRadarTestAccess::HasTimer(F.Radar.Get()));
	F.PC->Possess(F.Observer); FDCRadarTestAccess::Sync(F.Radar.Get());
	F.Radar->SetObjectiveComponent(NextComponent);
	FFixture Other; if (!Other.Start(*this)) { return false; }
	AActor* ForeignGoal = Other.Spawn<AActor>();
	USceneComponent* ForeignComponent = Other.Root(ForeignGoal, FVector::ZeroVector);
	F.Radar->SetObjectiveComponent(ForeignComponent);
	TestNull(TEXT("Foreign world target rejected"), F.Radar->GetObjectiveComponent());
	TestFalse(TEXT("Foreign world Pawn rejected"), FDCRadarTestAccess::Enemy(F.Radar.Get(), Other.Observer));
	F.Radar->SetObjectiveComponent(NextComponent);
	const FTimerHandle PriorWorldTimer = FDCRadarTestAccess::Timer(F.Radar.Get());
	F.Radar->SetOwningPlayer(Other.PC); FDCRadarTestAccess::Sync(F.Radar.Get());
	TestFalse(TEXT("World switch removed old world's timer"), F.World.GetTestWorld()->GetTimerManager().TimerExists(PriorWorldTimer));
	TestNull(TEXT("World/controller change clears prior objective"), F.Radar->GetObjectiveComponent());
	TestEqual(TEXT("World/controller change clears prior cache"), FDCRadarTestAccess::CandidateCount(F.Radar.Get()), 0);
	TestTrue(TEXT("New world has a live view"), F.Radar->IsRadarActive());
	F.Radar->SetObjectiveComponent(ForeignComponent);
	ForeignGoal->Destroy();
	TestNull(TEXT("Destroying objective owner hides target"), F.Radar->GetObjectiveComponent());
	const FTimerHandle LastTimer = FDCRadarTestAccess::Timer(F.Radar.Get());
	Other.Mode->UseExperienceUI(true);
	TestTrue(TEXT("Actual stage exit request"), Other.Mode->RequestExitStageFromMenu());
	TestTrue(TEXT("Ended stage/travel hides all data"), F.Radar->GetRadarMarkers().IsEmpty());
	FDCRadarTestAccess::Destruct(F.Radar.Get());
	TestFalse(TEXT("Destruct removed actual timer from its world"), Other.World.GetTestWorld()->GetTimerManager().TimerExists(LastTimer));
	TestFalse(TEXT("Destruct removes its timer"), FDCRadarTestAccess::HasTimer(F.Radar.Get()));
	TestFalse(TEXT("Destruct releases world reference"), FDCRadarTestAccess::HasTrackedWorld(F.Radar.Get()));
	TestEqual(TEXT("Destruct clears candidates"), FDCRadarTestAccess::CandidateCount(F.Radar.Get()), 0);
	return !HasAnyErrors();
}

#endif
