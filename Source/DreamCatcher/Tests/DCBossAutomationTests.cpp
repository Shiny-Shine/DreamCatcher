#include "Tests/DCBossTestTypes.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCBossTestTypes)

UDCR12DataFixture::UDCR12DataFixture() { OpeningDelay = 1000.f; TransitionDuration = .2f; }
UDCR12LineFixture::UDCR12LineFixture()
{
	DamageEffectClass = UDCR10AttackDamageFixture::StaticClass();
	DamageAmount = 10.f; WindupTime = .3f; RecoveryTime = .2f;
}
UDCR12GroundFixture::UDCR12GroundFixture()
{
	DamageEffectClass = UDCR10AttackDamageFixture::StaticClass();
	DamageAmount = 10.f; WindupTime = .3f; RecoveryTime = .2f; Radius = 150.f;
}
UDCR12SetFixture::UDCR12SetFixture()
{
	GrantedGameplayAbilities.AddDefaulted_GetRef().Ability = UDCR12LineFixture::StaticClass();
	GrantedGameplayAbilities.AddDefaulted_GetRef().Ability = UDCR12GroundFixture::StaticClass();
}
ADCR12BossFixture::ADCR12BossFixture()
{
	BossData = GetDefault<UDCR12DataFixture>();
	DeathAbilityClass = UDCR10DeathFixture::StaticClass();
	RangedAttackAbilityClass = UDCR12LineFixture::StaticClass();
	GroundStrikeAbilityClass = UDCR12GroundFixture::StaticClass();
	AbilitySets.Add(GetDefault<UDCR12SetFixture>());
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AI/Boss/DCBossTelegraph.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCStageFlowTestTypes.h"

struct FDCBigBrotherTestAccess
{
	static void FacePlayer(ADCBossCharacter* Boss, bool bEnable) { Boss->bAlwaysFacePlayer = bEnable; }
	static void Origin(ADCBossCharacter* Boss, bool bEnable, FVector Offset)
	{
		Boss->bUseCustomAttackOrigin = bEnable;
		Boss->AttackOriginOffset = Offset;
	}
};

namespace DCBossTests
{
struct FFixture
{
	FTestWorldWrapper World;
	ADCR12BossFixture* Boss = nullptr;
	ADCR10EnemyFixture* Target = nullptr;
	template<class T> T* Spawn(FVector Location = FVector(0, 0, 96))
	{
		FActorSpawnParameters P; P.ObjectFlags |= RF_Transient;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform(Location), P);
	}
	AActor* Box(FVector Center, FVector Extent)
	{
		AActor* Actor = Spawn<AActor>(Center);
		UBoxComponent* Shape = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Shape); Actor->SetRootComponent(Shape);
		Shape->SetBoxExtent(Extent); Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Shape->SetCollisionObjectType(ECC_WorldStatic); Shape->SetCollisionResponseToAllChannels(ECR_Block);
		Shape->RegisterComponent(); Actor->SetActorLocation(Center);
		return Actor;
	}
	void Advance(float Seconds)
	{
		for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I) { World.TickTestWorld(.01f); }
	}
	bool Start(FAutomationTestBase& Test, bool bSpawnBoss = true)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR12BossIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate unattended editor with -DCR12BossIsolatedAutomation -DCLyraRuntimeDiagnostics.")); return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR11ModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Box(FVector(0, 0, -10), FVector(3000, 3000, 10));
		Target = Spawn<ADCR10EnemyFixture>(FVector(500, 0, 96));
		if (!Test.TestNotNull(TEXT("Target"), Target)) { return false; }
		Target->SetGenericTeamId(FGenericTeamId(1));
		Target->GetCharacterMovement()->DisableMovement();
		Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (bSpawnBoss)
		{
			Boss = Spawn<ADCR12BossFixture>();
			if (!Test.TestNotNull(TEXT("Boss"), Boss) || !Test.TestTrue(TEXT("Boss GAS ready"), Boss->IsGASReady())) { return false; }
		}
		Advance(.03f);
		return true;
	}
	void Damage(ADCGASEnemyCharacter* Actor, float Amount)
	{
		UGameplayEffect* GE = NewObject<UGameplayEffect>();
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		auto& M = GE->Modifiers.AddDefaulted_GetRef(); M.Attribute = UDCHealthSet::GetDamageAttribute();
		M.ModifierOp = EGameplayModOp::Additive; M.ModifierMagnitude = FScalableFloat(Amount);
		UAbilitySystemComponent* ASC = Actor->GetAbilitySystemComponent();
		FGameplayEffectSpec Spec(GE, ASC->MakeEffectContext(), 1.f); ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
	float Health() const { return Target->GetHealthComponent()->GetHealth(); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossGroundTimingTest, "DreamCatcher.R12.Boss.GroundWindupAndSingleDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossGroundTimingTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Ground starts"), F.Boss->TryStartGroundStrike(F.Target));
	F.Advance(.15f); TestEqual(TEXT("No telegraph damage"), F.Health(), 100.f);
	TestFalse(TEXT("No reentry during windup"), F.Boss->TryStartGroundStrike(F.Target));
	TestFalse(TEXT("Line pattern cannot overlap a ground pattern"), F.Boss->TryStartRangedAttack(F.Target));
	F.Advance(.2f); TestEqual(TEXT("One original GE damage application"), F.Health(), 90.f);
	F.Advance(.6f); TestEqual(TEXT("No repeat damage during recovery or idle"), F.Health(), 90.f);
	TestTrue(TEXT("Line starts after ground recovery"), F.Boss->TryStartRangedAttack(F.Target));
	TestFalse(TEXT("Ground cannot overlap line"), F.Boss->TryStartGroundStrike(F.Target));
	F.Advance(.15f); TestEqual(TEXT("No line telegraph damage"), F.Health(), 90.f);
	F.Advance(.25f); TestEqual(TEXT("Inherited R10 ray applies one hit"), F.Health(), 80.f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossGroundAvoidanceTest, "DreamCatcher.R12.Boss.LockedGroundCoverAndImmunity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossGroundAvoidanceTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Lock initial point"), F.Boss->TryStartGroundStrike(F.Target));
	F.Target->SetActorLocation(FVector(900, 0, 96)); F.Advance(.65f);
	TestEqual(TEXT("Warning never follows the target"), F.Health(), 100.f);
	F.Target->SetActorLocation(FVector(500, 0, 96));
	TestTrue(TEXT("Start covered attempt"), F.Boss->TryStartGroundStrike(F.Target));
	F.Target->SetActorLocation(FVector(600, 0, 96));
	AActor* Wall = F.Box(FVector(550, 0, 70), FVector(8, 100, 70));
	F.Advance(.65f); TestEqual(TEXT("Cover prevents blast damage"), F.Health(), 100.f); Wall->Destroy();
	F.Target->GetAbilitySystemComponent()->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	TestTrue(TEXT("Immune target still has a visible attack"), F.Boss->TryStartGroundStrike(F.Target));
	F.Advance(.65f); TestEqual(TEXT("Original damage immunity respected"), F.Health(), 100.f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossPhaseTest, "DreamCatcher.R12.Boss.PhaseTransitionAndLethalPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossPhaseTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	UDCBossPhaseComponent* P = F.Boss->GetBossPhaseComponent();
	TestTrue(TEXT("An attack is in windup"), F.Boss->TryStartGroundStrike(F.Target));
	F.Damage(F.Boss, 301.f);
	TestEqual(TEXT("Threshold latches second phase"), P->GetBossPhase(), 2);
	TestTrue(TEXT("Transition state"), P->GetBossRunState() == EDCBossRunState::Transitioning);
	TestFalse(TEXT("No attack during transition"), F.Boss->TryStartGroundStrike(F.Target));
	F.Damage(F.Boss, 1.f); F.Advance(.4f);
	TestEqual(TEXT("Cancelled windup did not deal damage"), F.Health(), 100.f);
	TestTrue(TEXT("Transition finishes once"), P->GetBossRunState() == EDCBossRunState::Fighting);
	F.Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 600.f);
	F.Damage(F.Boss, 301.f);
	TestTrue(TEXT("Healing/re-crossing cannot repeat transition"), P->GetBossRunState() == EDCBossRunState::Fighting);
	ADCR12BossFixture* Second = F.Spawn<ADCR12BossFixture>(FVector(-500, 0, 96));
	if (!TestNotNull(TEXT("Second boss"), Second)) { return false; }
	F.Damage(Second, 1000.f);
	TestEqual(TEXT("Lethal health drop never starts phase two"), Second->GetBossPhaseComponent()->GetBossPhase(), 1);
	TestTrue(TEXT("Lethal result stops patterns"), Second->GetBossPhaseComponent()->GetBossRunState() == EDCBossRunState::Stopped);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossCancelTest, "DreamCatcher.R12.Boss.TargetDeathAndShutdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossCancelTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Start ground"), F.Boss->TryStartGroundStrike(F.Target));
	F.Damage(F.Target, 1000.f); F.Advance(.65f);
	FGameplayAbilitySpec* Spec = F.Boss->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UDCR12GroundFixture::StaticClass());
	TestTrue(TEXT("Target death cancels ground ability"), Spec && !Spec->IsActive());
	for (TActorIterator<ADCBossTelegraph> It(F.World.GetTestWorld()); It; ++It) { AddError(TEXT("Telegraph survived target death")); }
	F.Boss->GetBossPhaseComponent()->ShutdownBoss(); F.Advance(.4f);
	TestTrue(TEXT("Terminal shutdown"), F.Boss->GetBossPhaseComponent()->GetBossRunState() == EDCBossRunState::Stopped);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossOwnershipTest, "DreamCatcher.R12.Boss.OwnedGASAbortAndDeathCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossOwnershipTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this, false)) { return false; }
	ADCR12SpawnerFixture* S = F.Spawn<ADCR12SpawnerFixture>();
	if (!TestNotNull(TEXT("Owned spawner"), S)) { return false; }
	S->ConfigureOwned(); S->ActivateSpawner();
	ADCR12BossFixture* B = nullptr;
	for (TActorIterator<ADCR12BossFixture> It(F.World.GetTestWorld()); It; ++It) { B = *It; break; }
	if (!TestNotNull(TEXT("Owned boss"), B)) { return false; }
	TestTrue(TEXT("Boss begins attack"), B->TryStartGroundStrike(F.Target));
	S->Destroy();
	TestFalse(TEXT("ASC not destroyed synchronously in the abort stack"), B->IsActorBeingDestroyed());
	F.Advance(.03f); TestTrue(TEXT("Abort removes owned boss next tick"), B->IsActorBeingDestroyed());
	F.Advance(.5f); TestEqual(TEXT("Abort cancels scheduled damage"), F.Health(), 100.f);
	S = F.Spawn<ADCR12SpawnerFixture>(); S->ConfigureOwned(); S->ActivateSpawner();
	B = nullptr;
	for (TActorIterator<ADCR12BossFixture> It(F.World.GetTestWorld()); It; ++It) { B = *It; break; }
	if (!TestNotNull(TEXT("Second owned boss"), B)) { return false; }
	F.Damage(B, 1000.f);
	TestEqual(TEXT("Death counted immediately"), S->GetAliveCount(), 0);
	TestTrue(TEXT("Single boss completes its spawner"), S->IsSpawnerCleared());
	S->Destroy(); F.Advance(.03f);
	TestTrue(TEXT("Ownership remains after counting a kill"), B->IsActorBeingDestroyed());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBigBrotherFacingTest, "DreamCatcher.R12.Boss.ContinuousFacingAndLockedAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBigBrotherFacingTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	APlayerController* PC = F.Spawn<APlayerController>();
	if (!TestNotNull(TEXT("Stage player controller"), PC)) { return false; }
	F.World.GetTestWorld()->GetAuthGameMode<ADCR11ModeFixture>()->GenericPlayerInitialization(PC);
	PC->Possess(F.Target);
	const FVector Position = F.Boss->GetActorLocation();
	F.Boss->SetActorRotation(FRotator(0, 180, 0)); F.Advance(.02f);
	TestTrue(TEXT("Existing boss keeps its idle rotation without opting in"), F.Boss->GetActorForwardVector().Equals(-FVector::ForwardVector, .001));
	FDCBigBrotherTestAccess::FacePlayer(F.Boss, true);
	FDCBigBrotherTestAccess::Origin(F.Boss, true, FVector(0, 0, 100));
	F.Advance(.02f);
	TestTrue(TEXT("Opening delay still follows the player"), F.Boss->GetActorForwardVector().Equals(FVector::ForwardVector, .001));
	const FVector Origin = F.Boss->GetRangedAttackOrigin();
	TestTrue(TEXT("Line starts from the custom origin"), F.Boss->TryStartRangedAttack(F.Target));
	FGameplayAbilitySpec* Spec = F.Boss->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UDCR12LineFixture::StaticClass());
	UDCGameplayAbility_EnemyRangedAttack* Line = Spec ? Cast<UDCGameplayAbility_EnemyRangedAttack>(Spec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull(TEXT("Instanced line ability"), Line)) { return false; }
	const FVector LockedAim = Line->GetLockedAimPoint();
	F.Target->SetActorLocation(FVector(0, 500, 96)); F.Advance(.1f);
	TestTrue(TEXT("Body faces the moving player during windup"), F.Boss->GetActorForwardVector().Equals(FVector::RightVector, .001));
	TestTrue(TEXT("Telegraphed aim remains fixed"), Line->GetLockedAimPoint().Equals(LockedAim));
	TestTrue(TEXT("An origin on the rotation axis stays fixed"), F.Boss->GetRangedAttackOrigin().Equals(Origin));
	F.Advance(.25f);
	TestEqual(TEXT("Turning the body does not retarget the warned shot"), F.Health(), 100.f);
	F.Target->SetActorLocation(FVector(-500, 0, 96)); F.Advance(.02f);
	TestTrue(TEXT("Body follows during recovery"), F.Boss->GetActorForwardVector().Equals(-FVector::ForwardVector, .001));
	F.Advance(.3f);
	F.Target->SetActorLocation(FVector(0, -500, 96)); F.Advance(.02f);
	TestTrue(TEXT("Body follows between patterns"), F.Boss->GetActorForwardVector().Equals(-FVector::RightVector, .001));
	F.Damage(F.Boss, 301.f);
	F.Target->SetActorLocation(FVector(500, 500, 96)); F.Advance(.02f);
	TestTrue(TEXT("Phase transition remains active"), F.Boss->GetBossPhaseComponent()->GetBossRunState() == EDCBossRunState::Transitioning);
	TestTrue(TEXT("Body follows during phase transition"), F.Boss->GetActorForwardVector().Equals(FVector(1, 1, 0).GetSafeNormal(), .001));
	TestTrue(TEXT("Boss never translates"), F.Boss->GetActorLocation().Equals(Position));
	const FRotator StoppedRotation = F.Boss->GetActorRotation();
	F.Boss->GetBossPhaseComponent()->ShutdownBoss();
	F.Target->SetActorLocation(FVector(500, -500, 96)); F.Advance(.02f);
	TestTrue(TEXT("Shutdown stops facing updates"), F.Boss->GetActorRotation().Equals(StoppedRotation));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBigBrotherOriginTest, "DreamCatcher.R12.Boss.CustomOriginAndLegacyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBigBrotherOriginTest::RunTest(const FString&)
{
	DCBossTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Original NPC fallback remains world-space +60"), F.Target->GetRangedAttackOrigin().Equals(F.Target->GetActorLocation() + FVector(0, 0, 60)));
	TestTrue(TEXT("Existing boss also defaults to the original origin"), F.Boss->GetRangedAttackOrigin().Equals(F.Boss->GetActorLocation() + FVector(0, 0, 60)));
	F.Boss->SetActorRotation(FRotator(0, 90, 0)); F.Boss->SetActorScale3D(FVector(2));
	FDCBigBrotherTestAccess::Origin(F.Boss, true, FVector(10, 0, 100));
	const ADCGASEnemyCharacter* NPC = F.Boss;
	TestTrue(TEXT("NPC API dispatches the configured actor-local origin"), NPC->GetRangedAttackOrigin().Equals(F.Boss->GetActorLocation() + FVector(0, 20, 200), .001));
	FDCBigBrotherTestAccess::Origin(F.Boss, false, FVector(10, 0, 100));
	TestTrue(TEXT("Disabling the option restores the unscaled legacy fallback"), NPC->GetRangedAttackOrigin().Equals(F.Boss->GetActorLocation() + FVector(0, 0, 60)));
	return !HasAnyErrors();
}

#endif
