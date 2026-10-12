#include "Tests/DCEnemyGASTestTypes.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Character/DCLyraHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCEnemyGASTestTypes)

namespace DCEnemyGASTests
{
// A real Instant GE modifier, not direct Health subtraction.
void Damage(ADCGASEnemyCharacter* Target, float Amount)
{
	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UDCHealthSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(Amount);
	FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
}

void UDCR10DeathFixture::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (IsActive())
	{
		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, 0.05f);
		Wait->OnFinish.AddDynamic(this, &ThisClass::FinishFixtureDeath);
		Wait->ReadyForActivation();
	}
}
void UDCR10DeathFixture::FinishFixtureDeath() { K2_EndAbility(); }
UDCR10MarkerEffect::UDCR10MarkerEffect() { DurationPolicy = EGameplayEffectDurationType::Infinite; }
UDCR10ExtraSet::UDCR10ExtraSet()
{
	FDCAbilitySet_GameplayAbility& Ability = GrantedGameplayAbilities.AddDefaulted_GetRef();
	Ability.Ability = UDCR10MarkerAbility::StaticClass();
	FDCAbilitySet_GameplayEffect& Effect = GrantedGameplayEffects.AddDefaulted_GetRef();
	Effect.GameplayEffect = UDCR10MarkerEffect::StaticClass();
}
ADCR10EnemyFixture::ADCR10EnemyFixture()
{
	DeathAbilityClass = UDCR10DeathFixture::StaticClass();
	AbilitySets.Add(GetDefault<UDCR10ExtraSet>());
	AutoPossessAI = EAutoPossessAI::Disabled; // No navigation/AI dependencies in the isolated test world.
}
ADCR10UnconfiguredEnemyFixture::ADCR10UnconfiguredEnemyFixture() { DeathAbilityClass = nullptr; }
void ADCR10ImmediateEnemyFixture::BeginPlay()
{
	Super::BeginPlay();
	if (IsGASReady()) { DCEnemyGASTests::Damage(this, 1000.0f); }
}
ADCR10LegacyEnemyFixture::ADCR10LegacyEnemyFixture() { AutoPossessAI = EAutoPossessAI::Disabled; }
void ADCR10SpawnerFixture::ConfigureGAS(TSubclassOf<ADCGASEnemyCharacter> Class, int32 Count, int32 Concurrent)
{
	GASEnemyClass = Class;
	// Deliberately retained to verify the opt-in GAS class wins without deleting the legacy setting.
	EnemyClass = ADCR10LegacyEnemyFixture::StaticClass();
	SpawnCount = Count;
	MaxAliveAtOnce = Concurrent;
	SpawnInterval = 0.01f;
}
void ADCR10SpawnerFixture::ConfigureLegacy()
{
	GASEnemyClass = nullptr;
	EnemyClass = ADCR10LegacyEnemyFixture::StaticClass();
	SpawnCount = 1;
	MaxAliveAtOnce = 1;
}
void ADCR10EncounterFixture::Configure(ADCEnemySpawner* First, ADCEnemySpawner* Second)
{
	Spawners.Reset();
	if (First) { Spawners.Add(First); }
	if (Second) { Spawners.Add(Second); }
}
void UDCR10Observer::EnemyDied(ADCGASEnemyCharacter*) { ++Deaths; }
void UDCR10Observer::SpawnerCleared(ADCEnemySpawner*) { ++Clears; }
void UDCR10Observer::EncounterCompleted(ADCEncounterController*) { ++Encounters; }

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Components/DCHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Teams/DCTeamSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

namespace DCEnemyGASTests
{
struct FFixture
{
	FTestWorldWrapper World;
	TStrongObjectPtr<UDCR10Observer> Observer;
	bool Start(FAutomationTestBase& Test)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCR10EnemyIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate editor with -DCR10EnemyIsolatedAutomation; no assets are saved."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Observer.Reset(NewObject<UDCR10Observer>());
		return true;
	}
	template <typename T> T* Spawn()
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform(FVector(0,0,300)), Params);
	}
	TArray<ADCGASEnemyCharacter*> Living() const
	{
		TArray<ADCGASEnemyCharacter*> Result;
		for (TActorIterator<ADCGASEnemyCharacter> It(World.GetTestWorld()); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->IsGASReady() && !It->IsDeadOrDying()) { Result.Add(*It); }
		}
		return Result;
	}
	ADCR10SpawnerFixture* Spawner(int32 Count = 1, int32 Concurrent = 1)
	{
		ADCR10SpawnerFixture* Result = Spawn<ADCR10SpawnerFixture>();
		if (Result)
		{
			Result->ConfigureGAS(ADCR10EnemyFixture::StaticClass(), Count, Concurrent);
			Result->OnSpawnerCleared.AddDynamic(Observer.Get(), &UDCR10Observer::SpawnerCleared);
		}
		return Result;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyOriginalDamageTest, "DreamCatcher.R10.Enemy.OriginalDamageAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyOriginalDamageTest::RunTest(const FString&)
{
	using namespace DCEnemyGASTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10EnemyFixture* Target = F.Spawn<ADCR10EnemyFixture>();
	ADCPlayerState* Source = F.Spawn<ADCPlayerState>();
	if (!TestNotNull(TEXT("GAS target"), Target) || !TestNotNull(TEXT("PlayerState source"), Source)) { return false; }
	TestTrue(TEXT("Initialized before BeginPlay"), Target->IsGASReady());
	TestNull(TEXT("No second legacy HealthComponent"), Target->FindComponentByClass<UDCHealthComponent>());
	Target->OnEnemyDeath.AddDynamic(F.Observer.Get(), &UDCR10Observer::EnemyDied);
	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	TestEqual(TEXT("Death plus extra set ability"), TargetASC->GetActivatableAbilities().Num(), 2);
	TestEqual(TEXT("Exactly native Health and Combat sets"), TargetASC->GetSpawnedAttributes().Num(), 2);
	TestEqual(TEXT("Extra-set persistent effect"), TargetASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 1);
	Source->SetGenericTeamId(FGenericTeamId(1));
	UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
	DamageEffect->Executions.AddDefaulted_GetRef().CalculationClass = UDCLyraDamageExecution::StaticClass();
	auto Hit = [&](float Amount)
	{
		SourceASC->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), Amount);
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Source, Source);
		// This PlayerState-only fixture has no source Pawn and does not test distance attenuation.
		Context.AddOrigin(FVector::ZeroVector);
		FGameplayEffectSpec Spec(DamageEffect, Context, 1.0f);
		SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
	};
	Hit(12.0f);
	TestEqual(TEXT("Same-team damage blocked by original execution"), Target->GetHealthComponent()->GetHealth(), 100.0f);
	Source->SetGenericTeamId(FGenericTeamId(0));
	Hit(12.0f);
	TestEqual(TEXT("Enemy damage through original execution"), Target->GetHealthComponent()->GetHealth(), 88.0f);
	TargetASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	Hit(12.0f);
	TestEqual(TEXT("Original health immunity retained"), Target->GetHealthComponent()->GetHealth(), 88.0f);
	TargetASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	Hit(200.0f);
	TestEqual(TEXT("Lethal damage"), Target->GetHealthComponent()->GetHealth(), 0.0f);
	TestTrue(TEXT("Original Death starts"), Target->IsDeadOrDying());
	TestEqual(TEXT("One death event"), F.Observer->Deaths, 1);
	Hit(200.0f);
	TestEqual(TEXT("No duplicate death"), F.Observer->Deaths, 1);
	TStrongObjectPtr<UAbilitySystemComponent> RetainedASC(TargetASC);
	TWeakObjectPtr<ADCGASEnemyCharacter> TargetWeak(Target);
	// The first world tick activates pending timers; measure the delay after that tick.
	F.World.TickTestWorld(.001f);
	F.World.TickTestWorld(.06f);
	F.World.TickTestWorld(.02f);
	TestTrue(TEXT("Death finishes and destroys actor"), !TargetWeak.IsValid() || TargetWeak->IsActorBeingDestroyed());
	TestNull(TEXT("Avatar cleared on teardown"), RetainedASC->GetAvatarActor());
	TestEqual(TEXT("Grants removed on teardown"), RetainedASC->GetActivatableAbilities().Num(), 0);
	TestEqual(TEXT("Extra-set effects removed"), RetainedASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyTeamIdentityTest, "DreamCatcher.R10.Enemy.TeamIdentityWithoutPlayerState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyTeamIdentityTest::RunTest(const FString&)
{
	using namespace DCEnemyGASTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10EnemyFixture* Source = F.Spawn<ADCR10EnemyFixture>();
	ADCR10EnemyFixture* Target = F.Spawn<ADCR10EnemyFixture>();
	UDCTeamSubsystem* Teams = F.World.GetTestWorld()->GetSubsystem<UDCTeamSubsystem>();
	if (!TestNotNull(TEXT("Source NPC"), Source) || !TestNotNull(TEXT("Target NPC"), Target)
		|| !TestNotNull(TEXT("Original team subsystem"), Teams)) { return false; }
	TestNull(TEXT("Source has no PlayerState"), Source->GetPlayerState());
	TestNull(TEXT("Target has no PlayerState"), Target->GetPlayerState());
	TestFalse(TEXT("Two missing PlayerStates are not self damage"), Teams->CanCauseDamage(Source,Target));
	TestTrue(TEXT("Actual self damage remains allowed"), Teams->CanCauseDamage(Source,Source));
	TestFalse(TEXT("Self damage can still be disabled"), Teams->CanCauseDamage(Source,Source,false));
	TestFalse(TEXT("No source is not a self actor"), Teams->CanCauseDamage(nullptr,Target));
	TestFalse(TEXT("Two null objects are not self damage"), Teams->CanCauseDamage(nullptr,nullptr));
	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	SourceASC->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(),8.0f);
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(),NAME_None,RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect->Executions.AddDefaulted_GetRef().CalculationClass = UDCLyraDamageExecution::StaticClass();
	auto Hit = [&]()
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddOrigin(Source->GetActorLocation());
		FGameplayEffectSpec Spec(Effect,Context,1.0f);
		SourceASC->ApplyGameplayEffectSpecToTarget(Spec,Target->GetAbilitySystemComponent());
	};
	Hit();
	TestEqual(TEXT("Friendly NPC GE damage is blocked"), Target->GetHealthComponent()->GetHealth(),100.0f);
	Source->SetGenericTeamId(FGenericTeamId(2));
	TestTrue(TEXT("Opposing NPCs can damage"), Teams->CanCauseDamage(Source,Target));
	Hit();
	TestEqual(TEXT("Opposing NPC GE damage executes"), Target->GetHealthComponent()->GetHealth(),92.0f);
	ADCPlayerState* PS = F.Spawn<ADCPlayerState>();
	APawn* Pawn = F.Spawn<APawn>();
	if (!TestNotNull(TEXT("PlayerState fixture"), PS) || !TestNotNull(TEXT("Pawn fixture"), Pawn)) { return false; }
	PS->SetGenericTeamId(FGenericTeamId(0));
	Pawn->SetPlayerState(PS);
	TestTrue(TEXT("Real shared PlayerState self damage retained"), Teams->CanCauseDamage(PS,Pawn));
	TestFalse(TEXT("Shared PlayerState without self exception remains same-team"), Teams->CanCauseDamage(PS,Pawn,false));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemySpawnerLifecycleTest, "DreamCatcher.R10.Enemy.SpawnerAndEncounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemySpawnerLifecycleTest::RunTest(const FString&)
{
	using namespace DCEnemyGASTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10SpawnerFixture* Spawner = F.Spawner(2);
	ADCR10EncounterFixture* Encounter = F.Spawn<ADCR10EncounterFixture>();
	if (!TestNotNull(TEXT("Spawner"), Spawner) || !TestNotNull(TEXT("Encounter"), Encounter)) { return false; }
	Encounter->Configure(Spawner);
	Encounter->OnEncounterCompleted.AddDynamic(F.Observer.Get(), &UDCR10Observer::EncounterCompleted);
	Encounter->StartEncounter();
	TArray<ADCGASEnemyCharacter*> Enemies = F.Living();
	if (!TestEqual(TEXT("First spawned enemy"), Enemies.Num(), 1)) { return false; }
	TestEqual(TEXT("One alive"), Spawner->GetAliveCount(), 1);
	// Snapshot a binding before it is detached to simulate an already queued duplicate callback.
	FDCGASEnemyDeathSignature Replay = Enemies[0]->OnEnemyDeath;
	Damage(Enemies[0], 1000);
	Replay.Broadcast(Enemies[0]);
	Replay.Broadcast(nullptr);
	TestEqual(TEXT("Death consumed once"), Spawner->GetAliveCount(), 0);
	TestFalse(TEXT("Still more to spawn"), Spawner->IsSpawnerCleared());
	// Activate the pending spawn timer before advancing past its interval.
	F.World.TickTestWorld(.001f);
	F.World.TickTestWorld(.02f);
	Enemies = F.Living();
	if (!TestEqual(TEXT("Replacement spawned"), Enemies.Num(), 1)) { return false; }
	TestEqual(TEXT("Total spawn count"), Spawner->GetSpawnedCount(), 2);
	Replay.Broadcast(Enemies[0]); // Living foreign payload must not be counted as a death.
	TestEqual(TEXT("Forged notification ignored"), Spawner->GetAliveCount(), 1);
	Damage(Enemies[0], 1000);
	TestTrue(TEXT("Cleared"), Spawner->IsSpawnerCleared());
	TestEqual(TEXT("One clear"), F.Observer->Clears, 1);
	TestEqual(TEXT("One encounter completion"), F.Observer->Encounters, 1);
	Spawner->ActivateSpawner();
	F.World.TickTestWorld(.10f);
	F.World.TickTestWorld(.02f);
	TestEqual(TEXT("No respawn after clear"), Spawner->GetSpawnedCount(), 2);
	TestEqual(TEXT("Finish/Destroy do not clear twice"), F.Observer->Clears, 1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyEarlyDeathTest, "DreamCatcher.R10.Enemy.ImmediateDeathAndInvalidSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyEarlyDeathTest::RunTest(const FString&)
{
	using namespace DCEnemyGASTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10SpawnerFixture* Early = F.Spawner();
	ADCR10SpawnerFixture* Normal = F.Spawner();
	ADCR10EncounterFixture* Encounter = F.Spawn<ADCR10EncounterFixture>();
	if (!TestNotNull(TEXT("Early spawner"), Early) || !TestNotNull(TEXT("Normal spawner"), Normal)
		|| !TestNotNull(TEXT("Encounter"), Encounter)) { return false; }
	Early->ConfigureGAS(ADCR10ImmediateEnemyFixture::StaticClass());
	Encounter->Configure(Early, Normal);
	Encounter->OnEncounterCompleted.AddDynamic(F.Observer.Get(), &UDCR10Observer::EncounterCompleted);
	Encounter->StartEncounter();
	TestEqual(TEXT("BeginPlay death already consumed"), Early->GetAliveCount(), 0);
	TestEqual(TEXT("No premature encounter completion"), F.Observer->Encounters, 0);
	F.World.TickTestWorld(.02f);
	TestTrue(TEXT("Early spawner clears outside initial call stack"), Early->IsSpawnerCleared());
	TestEqual(TEXT("Other spawner still pending"), F.Observer->Encounters, 0);
	const TArray<ADCGASEnemyCharacter*> Enemies = F.Living();
	if (!TestEqual(TEXT("One normal enemy remains"), Enemies.Num(), 1)) { return false; }
	Damage(Enemies[0], 1000);
	TestEqual(TEXT("Encounter completes once"), F.Observer->Encounters, 1);
	ADCR10SpawnerFixture* Invalid = F.Spawner();
	if (!TestNotNull(TEXT("Invalid fixture spawner"), Invalid)) { return false; }
	Invalid->ConfigureGAS(ADCR10UnconfiguredEnemyFixture::StaticClass());
	AddExpectedError(TEXT("GAS enemy initialization failed"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("GAS enemy did not initialize"), EAutomationExpectedErrorFlags::Contains, 1);
	Invalid->ActivateSpawner();
	TestTrue(TEXT("Invalid setup is a failure"), Invalid->HasSpawnFailure());
	TestFalse(TEXT("Invalid setup is not a successful clear"), Invalid->IsSpawnerCleared());
	TestEqual(TEXT("Unusable actor removed from count"), Invalid->GetAliveCount(), 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyLegacyAndCleanupTest, "DreamCatcher.R10.Enemy.LegacyAndTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyLegacyAndCleanupTest::RunTest(const FString&)
{
	using namespace DCEnemyGASTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10SpawnerFixture* Legacy = F.Spawner();
	if (!TestNotNull(TEXT("Legacy spawner"), Legacy)) { return false; }
	Legacy->ConfigureLegacy(); Legacy->ActivateSpawner();
	ADCEnemyCharacter* LegacyEnemy = nullptr;
	for (TActorIterator<ADCEnemyCharacter> It(F.World.GetTestWorld()); It; ++It) { LegacyEnemy = *It; break; }
	if (!TestNotNull(TEXT("Legacy class still spawns"), LegacyEnemy)) { return false; }
	FDCEnemyDeathSignature Replay = LegacyEnemy->OnEnemyDeath;
	LegacyEnemy->GetHealthComponent()->ApplyDamage(1000,nullptr);
	Replay.Broadcast(LegacyEnemy);
	TestTrue(TEXT("Legacy death still clears"), Legacy->IsSpawnerCleared());
	TestEqual(TEXT("Legacy clear once"), F.Observer->Clears, 1);
	ADCR10SpawnerFixture* Removed = F.Spawner(2);
	if (!TestNotNull(TEXT("Teardown spawner"), Removed)) { return false; }
	Removed->ActivateSpawner();
	TArray<ADCGASEnemyCharacter*> Enemies = F.Living();
	if (!TestEqual(TEXT("Teardown enemy"), Enemies.Num(), 1)) { return false; }
	Removed->Destroy();
	Damage(Enemies[0],1000);
	F.World.TickTestWorld(.1f);
	TestEqual(TEXT("Dead spawner does not spawn again"), F.Living().Num(), 0);
	TestEqual(TEXT("No clear emitted by EndPlay"), F.Observer->Clears, 1);
	ADCR10SpawnerFixture* Unexpected = F.Spawner();
	if (!TestNotNull(TEXT("Unexpected removal spawner"), Unexpected)) { return false; }
	Unexpected->ActivateSpawner();
	Enemies = F.Living();
	if (!TestEqual(TEXT("Unexpected removal enemy"), Enemies.Num(), 1)) { return false; }
	AddExpectedError(TEXT("tracked enemy destroyed without a death notification"), EAutomationExpectedErrorFlags::Contains, 1);
	Enemies[0]->Destroy();
	TestTrue(TEXT("Unexpected removal reported"), Unexpected->HasSpawnFailure());
	TestFalse(TEXT("Destroy is not a fake kill/clear"), Unexpected->IsSpawnerCleared());
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
