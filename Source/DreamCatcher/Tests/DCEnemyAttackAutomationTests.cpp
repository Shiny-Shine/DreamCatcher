#include "Tests/DCEnemyAttackTestTypes.h"

#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Character/DCLyraHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCEnemyAttackTestTypes)

UDCR10AttackDamageFixture::UDCR10AttackDamageFixture()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayEffectExecutionDefinition& Execution = Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UDCLyraDamageExecution::StaticClass();
	FGameplayEffectExecutionScopedModifierInfo Modifier(FGameplayEffectAttributeCaptureDefinition(
		UDCCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true));
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = DCGameplayTags::SetByCaller_Damage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
	Execution.CalculationModifiers.Add(Modifier);
}

UDCR10AttackAbilityFixture::UDCR10AttackAbilityFixture() { DamageEffectClass = UDCR10AttackDamageFixture::StaticClass(); }
void UDCR10AttackAbilityFixture::BP_OnAttackIntent_Implementation(FVector, FVector, float)
{
	++Intents;
	if (bCancelOnIntent) { CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true); }
}
void UDCR10AttackAbilityFixture::BP_OnAttackShot_Implementation(FVector, FVector, AActor*) { ++Shots; }
void UDCR10AttackAbilityFixture::BP_OnAttackEnded_Implementation(bool bWasCancelled)
{
	++Ends;
	Cancellations += bWasCancelled ? 1 : 0;
}
UDCR10AttackSetFixture::UDCR10AttackSetFixture()
{
	GrantedGameplayAbilities.AddDefaulted_GetRef().Ability = UDCR10AttackAbilityFixture::StaticClass();
}
ADCR10AttackEnemyFixture::ADCR10AttackEnemyFixture()
{
	AbilitySets.Add(GetDefault<UDCR10AttackSetFixture>());
	RangedAttackAbilityClass = UDCR10AttackAbilityFixture::StaticClass();
}
ADCR10AutoAttackEnemyFixture::ADCR10AutoAttackEnemyFixture() { bAutoRangedAttack = true; }

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

namespace DCEnemyAttackTests
{
struct FFixture
{
	FTestWorldWrapper World;
	ADCR10AttackEnemyFixture* Enemy = nullptr;
	ADCR10EnemyFixture* Target = nullptr;
	UDCR10AttackAbilityFixture* Ability = nullptr;

	template <typename T> T* Spawn(FVector Location = FVector(0,0,300))
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform(Location), Params);
	}
	bool Start(FAutomationTestBase& Test, bool bAutoAttack = false)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCR10EnemyIsolatedAutomation")))
		{
			Test.AddError(TEXT("Run a separate editor with -DCR10EnemyIsolatedAutomation; no assets are saved."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Target = Spawn<ADCR10EnemyFixture>(FVector(500,0,300));
		if (!Test.TestNotNull(TEXT("Target"), Target)) { return false; }
		Target->SetGenericTeamId(FGenericTeamId(2));
		Target->GetCharacterMovement()->DisableMovement();
		Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (bAutoAttack)
		{
			APlayerController* PC = Spawn<APlayerController>();
			if (!Test.TestNotNull(TEXT("Target controller"), PC)) { return false; }
			PC->Possess(Target);
			Enemy = Spawn<ADCR10AutoAttackEnemyFixture>();
		}
		else { Enemy = Spawn<ADCR10AttackEnemyFixture>(); }
		if (!Test.TestNotNull(TEXT("Source"), Enemy)) { return false; }
		Enemy->GetCharacterMovement()->DisableMovement();
		UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(UDCR10AttackAbilityFixture::StaticClass());
		Ability = Spec ? Cast<UDCR10AttackAbilityFixture>(Spec->GetPrimaryInstance()) : nullptr;
		return Test.TestTrue(TEXT("Source initialized"), Enemy->IsGASReady())
			&& Test.TestNotNull(TEXT("Instanced attack"), Ability);
	}
	void Advance(float Seconds)
	{
		// Small distinct frames also promote Pending timers before measuring expiry.
		for (int32 Index = 0; Index < FMath::CeilToInt(Seconds / 0.01f); ++Index) { World.TickTestWorld(0.01f); }
	}
	float Health() const { return Target->GetHealthComponent()->GetHealth(); }
	AActor* Cover()
	{
		AActor* Wall = Spawn<AActor>(FVector(250,0,350));
		if (!Wall) { return nullptr; }
		UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
		Wall->AddInstanceComponent(Box);
		Wall->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10,150,150));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Box->RegisterComponent();
		Wall->SetActorLocation(FVector(250,0,350));
		return Wall;
	}
	void Damage(ADCGASEnemyCharacter* Victim, float Amount)
	{
		UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = GE->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UDCHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Amount);
		UAbilitySystemComponent* ASC = Victim->GetAbilitySystemComponent();
		FGameplayEffectSpec Spec(GE, ASC->MakeEffectContext(), 1.0f);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
	void Kill(ADCGASEnemyCharacter* Victim) { Damage(Victim, 1000.0f); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAttackTimingTest, "DreamCatcher.R10.Attack.WindupHitRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAttackTimingTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Attack begins"), F.Enemy->TryStartRangedAttack(F.Target));
	TestNull(TEXT("Request target not retained on NPC"), F.Enemy->GetRequestedRangedAttackTarget());
	F.Advance(.2f);
	TestEqual(TEXT("No windup damage"), F.Health(), 100.0f);
	TestFalse(TEXT("No overlapping activation"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(.4f);
	TestEqual(TEXT("Original execution receives SetByCaller damage"), F.Health(), 92.0f);
	TestEqual(TEXT("Single shot"), F.Ability->Shots, 1);
	TestTrue(TEXT("Recovery remains active"), F.Ability->GetAttackPhase() == EDCEnemyAttackPhase::Recovery);
	TestFalse(TEXT("No retrigger in recovery"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(.8f);
	TestFalse(TEXT("Ability ended"), F.Ability->IsActive());
	TestEqual(TEXT("One presentation end"), F.Ability->Ends, 1);
	TestTrue(TEXT("New attack after recovery"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(1.4f);
	TestEqual(TEXT("Exactly second damage"), F.Health(), 84.0f);
	TestEqual(TEXT("Exactly two shots"), F.Ability->Shots, 2);
	TestEqual(TEXT("No unintended cancellation"), F.Ability->Cancellations, 0);
	F.Damage(F.Target, 80.0f);
	TestTrue(TEXT("Begin lethal shot"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(.6f);
	TestTrue(TEXT("Target killed by shot does not waive recovery"), F.Ability->IsActive()
		&& F.Ability->GetAttackPhase() == EDCEnemyAttackPhase::Recovery);
	F.Advance(.8f);
	TestEqual(TEXT("Lethal shot ends normally"), F.Ability->Cancellations, 0);
	TestEqual(TEXT("Third attack finished"), F.Ability->Ends, 3);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAttackDodgeCoverTest, "DreamCatcher.R10.Attack.LockedAimAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAttackDodgeCoverTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Begin locked aim"), F.Enemy->TryStartRangedAttack(F.Target));
	const FVector Aim = F.Ability->GetLockedAimPoint();
	F.Target->SetActorLocation(FVector(500,250,300));
	F.Advance(1.4f);
	TestTrue(TEXT("Aim does not track dodging target"), F.Ability->GetLockedAimPoint().Equals(Aim));
	TestEqual(TEXT("Miss does not force damage"), F.Health(), 100.0f);
	TestEqual(TEXT("Still one fired shot"), F.Ability->Shots, 1);
	F.Target->SetActorLocation(FVector(500,0,300));
	AActor* Wall = F.Cover();
	if (!TestNotNull(TEXT("Blocking wall"), Wall)) { return false; }
	TestFalse(TEXT("Cover rejects initial attack"), F.Enemy->TryStartRangedAttack(F.Target));
	Wall->SetActorEnableCollision(false);
	TestTrue(TEXT("Attack starts when visible"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(.1f);
	Wall->SetActorEnableCollision(true);
	F.Advance(.1f);
	TestFalse(TEXT("Cover during windup cancels"), F.Ability->IsActive());
	Wall->SetActorEnableCollision(false);
	F.Advance(1.4f);
	TestEqual(TEXT("No delayed hit after cover disappears"), F.Health(), 100.0f);
	TestEqual(TEXT("Cancelled shot not emitted"), F.Ability->Shots, 1);
	TestTrue(TEXT("Begin before leaving range"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Target->SetActorLocation(FVector(2000,0,300));
	F.Advance(1.4f);
	TestFalse(TEXT("Leaving range cancels pending shot"), F.Ability->IsActive());
	TestEqual(TEXT("No out-of-range damage"), F.Health(), 100.0f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAttackCancelTest, "DreamCatcher.R10.Attack.CancelAndTargetLoss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAttackCancelTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestTrue(TEXT("Starts before cancel"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(.1f);
	// ASC::CancelAbility takes an ability CDO, not this per-actor execution instance.
	const FGameplayAbilitySpecHandle AttackHandle = F.Ability->GetCurrentAbilitySpecHandle();
	if (!TestTrue(TEXT("Active attack has a valid spec handle"), AttackHandle.IsValid())) { return false; }
	F.Enemy->GetAbilitySystemComponent()->CancelAbilityHandle(AttackHandle);
	TestFalse(TEXT("Cancel immediately deactivates the attack"), F.Ability->IsActive());
	F.Advance(1.4f);
	TestEqual(TEXT("Cancel removes delayed damage"), F.Health(), 100.0f);
	TestEqual(TEXT("Cancel ends once"), F.Ability->Ends, 1);
	F.Ability->bCancelOnIntent = true;
	F.Enemy->TryStartRangedAttack(F.Target);
	F.Advance(1.4f);
	TestEqual(TEXT("Reentrant intent cancellation cannot schedule a shot"), F.Ability->Shots, 0);
	TestEqual(TEXT("Reentrant cancel ends once"), F.Ability->Ends, 2);
	F.Ability->bCancelOnIntent = false;
	TestTrue(TEXT("Starts before target death"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Kill(F.Target);
	TestFalse(TEXT("Target death cancels immediately"), F.Ability->IsActive());
	F.Advance(1.4f);
	TestEqual(TEXT("No shot after target death"), F.Ability->Shots, 0);
	ADCR10EnemyFixture* Other = F.Spawn<ADCR10EnemyFixture>(FVector(500,0,300));
	if (!TestNotNull(TEXT("Replacement target"), Other)) { return false; }
	Other->SetGenericTeamId(FGenericTeamId(2));
	TestTrue(TEXT("Starts before target destroy"), F.Enemy->TryStartRangedAttack(Other));
	Other->Destroy();
	TestFalse(TEXT("Target EndPlay cancels immediately"), F.Ability->IsActive());
	F.Advance(1.4f);
	TestEqual(TEXT("No shot after target destruction"), F.Ability->Shots, 0);
	TestEqual(TEXT("All end notifications paired"), F.Ability->Ends, F.Ability->Intents);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAttackDeathTest, "DreamCatcher.R10.Attack.SourceDeathAndTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAttackDeathTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCR10AttackAbilityFixture> Retained(F.Ability);
	TestTrue(TEXT("Attack begins"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Kill(F.Enemy);
	TestFalse(TEXT("Original Death cancels attack"), Retained->IsActive());
	TestEqual(TEXT("Cancellation reported"), Retained->Cancellations, 1);
	F.Advance(1.4f);
	TestEqual(TEXT("No late damage from dead attacker"), F.Health(), 100.0f);
	TestEqual(TEXT("No shot after attacker death"), Retained->Shots, 0);
	ADCR10AttackEnemyFixture* Other = F.Spawn<ADCR10AttackEnemyFixture>();
	if (!TestNotNull(TEXT("Second attacker"), Other)) { return false; }
	Other->GetCharacterMovement()->DisableMovement();
	FGameplayAbilitySpec* Spec = Other->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UDCR10AttackAbilityFixture::StaticClass());
	if (!TestNotNull(TEXT("Second attack spec"), Spec)) { return false; }
	TStrongObjectPtr<UDCR10AttackAbilityFixture> Second(Cast<UDCR10AttackAbilityFixture>(Spec->GetPrimaryInstance()));
	if (!TestNotNull(TEXT("Second attack instance"), Second.Get())) { return false; }
	TestTrue(TEXT("Second starts"), Other->TryStartRangedAttack(F.Target));
	Other->Destroy();
	F.Advance(1.4f);
	TestEqual(TEXT("Destroy also clears delayed damage"), F.Health(), 100.0f);
	TestFalse(TEXT("Destroyed source ability inactive"), Second->IsActive());
	TestEqual(TEXT("Destroyed source ended once"), Second->Ends, 1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAttackRulesTest, "DreamCatcher.R10.Attack.TeamImmunityAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAttackRulesTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this)) { return false; }
	TestFalse(TEXT("Null target rejected"), F.Enemy->TryStartRangedAttack(nullptr));
	TestFalse(TEXT("Self target rejected"), F.Enemy->TryStartRangedAttack(F.Enemy));
	F.Target->SetGenericTeamId(FGenericTeamId(1));
	TestFalse(TEXT("Friendly target rejected"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Target->SetGenericTeamId(FGenericTeamId(2));
	F.Target->SetActorLocation(FVector(2000,0,300));
	TestFalse(TEXT("Out of range rejected"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Target->SetActorLocation(FVector(500,0,300));
	UAbilitySystemComponent* TargetASC = F.Target->GetAbilitySystemComponent();
	TargetASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	TestTrue(TEXT("Immune target may be shot"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(1.4f);
	TestEqual(TEXT("Original immunity blocks damage"), F.Health(), 100.0f);
	TestEqual(TEXT("Shot still emitted"), F.Ability->Shots, 1);
	TargetASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	TestTrue(TEXT("Next shot after immunity"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Advance(1.4f);
	TestEqual(TEXT("Damage resumes"), F.Health(), 92.0f);
	TestTrue(TEXT("Starts before input block"), F.Enemy->TryStartRangedAttack(F.Target));
	F.Enemy->GetAbilitySystemComponent()->AddLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	F.Advance(1.4f);
	TestFalse(TEXT("Blocking input interrupts windup"), F.Ability->IsActive());
	TestEqual(TEXT("No damage after block"), F.Health(), 92.0f);
	F.Enemy->GetAbilitySystemComponent()->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	F.Ability->RemoveEffectForTest();
	TestFalse(TEXT("No damage GE is not an active attack"), F.Enemy->TryStartRangedAttack(F.Target));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEnemyAutoAttackTest, "DreamCatcher.R10.Attack.OptInAutoRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCEnemyAutoAttackTest::RunTest(const FString&)
{
	DCEnemyAttackTests::FFixture F; if (!F.Start(*this, true)) { return false; }
	F.Advance(1.0f);
	TestEqual(TEXT("Opt-in timer requests first attack"), F.Health(), 92.0f);
	TestEqual(TEXT("One automatic shot"), F.Ability->Shots, 1);
	TStrongObjectPtr<UDCR10AttackAbilityFixture> Retained(F.Ability);
	F.Kill(F.Enemy);
	F.Advance(2.0f);
	TestEqual(TEXT("No auto requests after death"), Retained->Shots, 1);
	TestEqual(TEXT("No post-death damage"), F.Health(), 92.0f);
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
