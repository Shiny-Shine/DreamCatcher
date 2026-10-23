// Unsaved native regressions for the opt-in Inspector death policy. No PIE or asset saves.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Messages/DCVerbMessage.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Physics/DCLyraCollisionChannels.h"
#include "Teams/DCTeamSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCEnemyGASTestTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace DCInspectorDeathTests
{
struct FFixture
{
	FTestWorldWrapper World;

	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCInspectorDeathIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use an isolated unattended editor with -DCInspectorDeathIsolatedAutomation; no PIE or asset saves."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return Test.TestTrue(TEXT("Damage message subsystem exists"), UGameplayMessageSubsystem::HasInstance(World.GetTestWorld()));
	}

	ADCR10EnemyFixture* Spawn(uint8 Team)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ADCR10EnemyFixture* Actor = World.GetTestWorld()->SpawnActor<ADCR10EnemyFixture>(
			ADCR10EnemyFixture::StaticClass(), FTransform(FVector(0, 0, 300)), Params);
		if (!Actor || !Actor->IsGASReady()) { return nullptr; }
		Actor->SetGenericTeamId(FGenericTeamId(Team));
		return Actor;
	}
};

void ApplyDamage(ADCR10EnemyFixture* Source, ADCR10EnemyFixture* Target, float Amount, bool bUseWeaponExecution)
{
	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	TStrongObjectPtr<UGameplayEffect> Effect(NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient));
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	// Both active weapon damage GEs use this same condition. Real audio/VFX remain a user play check.
	Effect->bRequireModifierSuccessToTriggerCues = true;
	if (bUseWeaponExecution)
	{
		SourceASC->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), Amount);
		Effect->Executions.AddDefaulted_GetRef().CalculationClass = UDCLyraDamageExecution::StaticClass();
	}
	else
	{
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UDCHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Amount);
	}
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Source, Source);
	Context.AddOrigin(Source->GetActorLocation());
	FGameplayEffectSpec Spec(Effect.Get(), Context, 1.f);
	SourceASC->ApplyGameplayEffectSpecToTarget(Spec, Target->GetAbilitySystemComponent());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCInspectorFirstKillAndRejectedDamageTest,
	"DreamCatcher.AI.Inspector.Death.FirstKillAndRejectedDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCInspectorFirstKillAndRejectedDamageTest::RunTest(const FString&)
{
	using namespace DCInspectorDeathTests;
	FFixture Fixture;
	if (!Fixture.Start(*this)) { return false; }
	ADCR10EnemyFixture* Source = Fixture.Spawn(0);
	ADCR10EnemyFixture* Target = Fixture.Spawn(1);
	if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("Target"), Target)) { return false; }
	UDCLyraHealthComponent* TargetHealth = Target->GetHealthComponent();
	const UDCHealthSet* Stats = Target->GetAbilitySystemComponent()->GetSet<UDCHealthSet>();
	UDCTeamSubsystem* Teams = Fixture.World.GetTestWorld()->GetSubsystem<UDCTeamSubsystem>();
	if (!TestNotNull(TEXT("Health set"), Stats) || !TestNotNull(TEXT("Teams"), Teams)) { return false; }
	TargetHealth->bPreventDamageAfterDeath = true;

	int32 Deaths = 0, RawMessages = 0, ImmunityNotifications = 0;
	bool bInjectedZeroHPDamage = false, bGuardedBeforeDeathAbility = false;
	const FDelegateHandle DeathHandle = Stats->OnOutOfHealth.AddLambda(
		[&](AActor*, AActor*, const FGameplayEffectSpec*, float, float, float) { ++Deaths; });
	const FDelegateHandle ImmunityHandle = Stats->OnDamageBlocked.AddLambda(
		[&](const FGameplayEffectSpec&, float) { ++ImmunityNotifications; });
	FGameplayMessageListenerHandle MessageHandle = UGameplayMessageSubsystem::Get(Fixture.World.GetTestWorld())
		.RegisterListener<FDCVerbMessage>(TAG_Lyra_Damage_Message, [&](FGameplayTag, const FDCVerbMessage& Message)
		{
			if (Message.Target != Target) { return; }
			++RawMessages;
			if (TargetHealth->GetHealth() <= 0.f && !bInjectedZeroHPDamage)
			{
				bInjectedZeroHPDamage = true;
				bGuardedBeforeDeathAbility = TargetHealth->GetDeathState() == EDCLyraDeathState::NotDead
					&& TargetHealth->ShouldRejectPostDeathDamage() && !Teams->CanCauseDamage(Source, Target);
				ApplyDamage(Source, Target, 5.f, false);
			}
		});

	TestTrue(TEXT("Living enemy remains a valid hit"), Teams->CanCauseDamage(Source, Target));
	ApplyDamage(Source, Target, 12.f, true);
	TestEqual(TEXT("Normal weapon damage preserved"), TargetHealth->GetHealth(), 88.f);
	// The original reticle caches this predicate before applying the hit's GE.
	const bool bFatalHitWasSuccessful = Teams->CanCauseDamage(Source, Target);
	ApplyDamage(Source, Target, 200.f, true);
	TestTrue(TEXT("First lethal hit retains its cached success"), bFatalHitWasSuccessful);
	TestEqual(TEXT("Lethal health"), TargetHealth->GetHealth(), 0.f);
	TestEqual(TEXT("Original Death ability started"), TargetHealth->GetDeathState(), EDCLyraDeathState::DeathStarted);
	TestTrue(TEXT("HP-zero reentry is rejected before DeathState changes"), bGuardedBeforeDeathAbility);
	TestEqual(TEXT("One fatal notification"), Deaths, 1);
	TestEqual(TEXT("Only the two living-target hits emit damage messages"), RawMessages, 2);
	TestFalse(TEXT("Corpse is not a successful hit"), Teams->CanCauseDamage(Source, Target));
	TestFalse(TEXT("Self-damage exception does not re-enable the opted-in corpse"), Teams->CanCauseDamage(Target, Target));

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	TargetASC->SetNumericAttributeBase(UDCHealthSet::GetMaxArmorAttribute(), 150.f);
	TargetASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), 50.f);
	ApplyDamage(Source, Target, 200.f, true);
	ApplyDamage(Source, Target, 200.f, false);
	TestEqual(TEXT("No damage meta remains"), Stats->GetDamage(), 0.f);
	TestEqual(TEXT("No further health change"), TargetHealth->GetHealth(), 0.f);
	TestEqual(TEXT("Rejected damage does not consume armor"), TargetHealth->GetArmor(), 50.f);
	TestEqual(TEXT("No repeated damage message"), RawMessages, 2);
	TestEqual(TEXT("No repeated fatal notification"), Deaths, 1);
	TestEqual(TEXT("Corpse rejection does not emit immunity feedback"), ImmunityNotifications, 0);

	MessageHandle.Unregister();
	Stats->OnOutOfHealth.Remove(DeathHandle);
	Stats->OnDamageBlocked.Remove(ImmunityHandle);
	TWeakObjectPtr<ADCR10EnemyFixture> TargetWeak(Target);
	Fixture.World.TickTestWorld(.001f);
	Fixture.World.TickTestWorld(.06f);
	Fixture.World.TickTestWorld(.02f);
	TestTrue(TEXT("Existing Death ability finishes and removes the actor"), !TargetWeak.IsValid() || TargetWeak->IsActorBeingDestroyed());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCInspectorDeathCollisionAndDefaultPolicyTest,
	"DreamCatcher.AI.Inspector.Death.CollisionAndDefaultPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCInspectorDeathCollisionAndDefaultPolicyTest::RunTest(const FString&)
{
	using namespace DCInspectorDeathTests;
	FFixture Fixture;
	if (!Fixture.Start(*this)) { return false; }
	TestFalse(TEXT("Existing player/boss component default remains off"), GetDefault<UDCLyraHealthComponent>()->bPreventDamageAfterDeath);
	ADCR10EnemyFixture* Source = Fixture.Spawn(0);
	UDCTeamSubsystem* Teams = Fixture.World.GetTestWorld()->GetSubsystem<UDCTeamSubsystem>();
	if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("Teams"), Teams)) { return false; }
	for (bool bOptIn : {false, true})
	{
		ADCR10EnemyFixture* Target = Fixture.Spawn(1);
		if (!TestNotNull(TEXT("Policy target"), Target)) { return false; }
		UDCLyraHealthComponent* TargetHealth = Target->GetHealthComponent();
		TargetHealth->bPreventDamageAfterDeath = bOptIn;
		UBoxComponent* ExtraBody = NewObject<UBoxComponent>(Target, NAME_None, RF_Transient);
		Target->AddInstanceComponent(ExtraBody);
		ExtraBody->RegisterComponent();
		ExtraBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ExtraBody->SetCollisionResponseToAllChannels(ECR_Block);
		const ECollisionResponse OldWorldResponse = ExtraBody->GetCollisionResponseToChannel(ECC_WorldStatic);
		const ECollisionResponse OldPhysicsResponse = ExtraBody->GetCollisionResponseToChannel(ECC_PhysicsBody);
		const ECollisionResponse OldMeshResponse = Target->GetMesh()->GetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon);
		TestFalse(TEXT("Living target is not rejected"), TargetHealth->ShouldRejectPostDeathDamage());
		TargetHealth->StartDeath();
		TestEqual(TEXT("Opt-in controls dead-state rejection even before HP is changed"), TargetHealth->ShouldRejectPostDeathDamage(), bOptIn);
		TestEqual(TEXT("Only opted-in dead targets stop the existing hit predicate"), Teams->CanCauseDamage(Source, Target), !bOptIn);
		for (ECollisionChannel Channel : {DCLyra_TraceChannel_Weapon, DCLyra_TraceChannel_Weapon_Capsule, DCLyra_TraceChannel_Weapon_Multi})
		{
			TestEqual(TEXT("All weapon trace variants follow opt-in"), ExtraBody->GetCollisionResponseToChannel(Channel), bOptIn ? ECR_Ignore : ECR_Block);
		}
		TestEqual(TEXT("Character mesh follows the same opt-in"), Target->GetMesh()->GetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon), bOptIn ? ECR_Ignore : OldMeshResponse);
		TestEqual(TEXT("World collision response is retained"), ExtraBody->GetCollisionResponseToChannel(ECC_WorldStatic), OldWorldResponse);
		TestEqual(TEXT("Physics collision response is retained"), ExtraBody->GetCollisionResponseToChannel(ECC_PhysicsBody), OldPhysicsResponse);
		TargetHealth->StartDeath();
		TargetHealth->FinishDeath();
		TargetHealth->FinishDeath();
		TestEqual(TEXT("Original idempotent death transitions retained"), TargetHealth->GetDeathState(), EDCLyraDeathState::DeathFinished);
		TestEqual(TEXT("Finished-death policy remains opt-in"), TargetHealth->ShouldRejectPostDeathDamage(), bOptIn);
	}
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE
