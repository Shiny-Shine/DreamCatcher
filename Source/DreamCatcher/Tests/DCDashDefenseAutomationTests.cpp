#include "Tests/DCDashDefenseTestTypes.h"

#include "AbilitySystem/DCGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCDashDefenseTestTypes)

UDCDashDefenseAutomationAbility::UDCDashDefenseAutomationAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UDCDashDefenseAutomationAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData*)
{
	bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
}
void UDCDashDefenseAutomationAbility::RecordOpened(const FGameplayEventData&) { ++Opened; }
void UDCDashDefenseAutomationAbility::RecordClosed(const FGameplayEventData&) { ++Closed; }
void UDCDashDefenseAutomationAbility::RecordAvoided(const FGameplayEventData&) { ++Avoided; }

UDCDashDefenseAutomationEffect::UDCDashDefenseAutomationEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.25f);
	UTargetTagsGameplayEffectComponent* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(Tags);
	FInheritedTagContainer Granted;
	Granted.Added.AddTag(DCGameplayTags::Gameplay_DamageImmunity);
	Tags->SetAndApplyTargetTagChanges(Granted);
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Tasks/DCAbilityTask_DashDefenseWindow.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

namespace DCDashDefenseTests
{
struct FFixture
{
	FTestWorldWrapper World;
	APawn* Pawn = nullptr;
	AActor* Attacker = nullptr;
	UDCAbilitySystemComponent* ASC = nullptr;
	UDCHealthSet* Health = nullptr;
	UDCDashDefenseAutomationAbility* Ability = nullptr;
	FDelegateHandle EventBinding;
	FGameplayEventData LastSuccess;
	TArray<TStrongObjectPtr<UDCAbilityTask_DashDefenseWindow>> Tasks;
	int32 Successes = 0;

	~FFixture()
	{
		if (IsValid(ASC))
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(DCGameplayTags::GameplayEvent_Dodge_Success).Remove(EventBinding);
			ASC->CancelAllAbilities();
		}
	}

	bool Start(FAutomationTestBase& Test)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCR9DefenseIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate editor process with -DCR9DefenseIsolatedAutomation; no production assets are changed."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game))
		{
			World.ForwardErrorMessages(&Test);
			return false;
		}
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		if (!World.BeginPlayInTestWorld())
		{
			World.ForwardErrorMessages(&Test);
			return false;
		}
		Pawn = World.GetTestWorld()->SpawnActor<APawn>();
		Attacker = World.GetTestWorld()->SpawnActor<AActor>();
		if (!Test.TestNotNull(TEXT("Pawn"), Pawn) || !Test.TestNotNull(TEXT("Attacker"), Attacker)) return false;
		ASC = NewObject<UDCAbilitySystemComponent>(Pawn, NAME_None, RF_Transient);
		Pawn->AddInstanceComponent(ASC);
		ASC->RegisterComponent();
		Health = NewObject<UDCHealthSet>(Pawn, NAME_None, RF_Transient);
		ASC->AddAttributeSetSubobject(Health);
		ASC->InitAbilityActorInfo(Pawn, Pawn);
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UDCDashDefenseAutomationAbility::StaticClass(), 1));
		if (!Test.TestTrue(TEXT("Native fixture activates"), ASC->TryActivateAbility(Handle))) return false;
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		Ability = Spec ? Cast<UDCDashDefenseAutomationAbility>(Spec->GetPrimaryInstance()) : nullptr;
		if (!Test.TestNotNull(TEXT("Instanced fixture ability"), Ability) || !Test.TestTrue(TEXT("Commit succeeded"), Ability->bCommitted)) return false;
		EventBinding = ASC->GenericGameplayEventCallbacks.FindOrAdd(DCGameplayTags::GameplayEvent_Dodge_Success).AddLambda(
			[this](const FGameplayEventData* Event) { ++Successes; LastSuccess = *Event; });
		return true;
	}

	UDCAbilityTask_DashDefenseWindow* Open(bool bCommitted = true, float Duration = 0.25f)
	{
		UDCAbilityTask_DashDefenseWindow* Task = UDCAbilityTask_DashDefenseWindow::DashDefenseWindow(
			Ability, bCommitted, UDCDashDefenseAutomationEffect::StaticClass(), Duration);
		Tasks.Emplace(Task);
		Task->OnOpened.AddDynamic(Ability, &UDCDashDefenseAutomationAbility::RecordOpened);
		Task->OnClosed.AddDynamic(Ability, &UDCDashDefenseAutomationAbility::RecordClosed);
		Task->OnDamageAvoided.AddDynamic(Ability, &UDCDashDefenseAutomationAbility::RecordAvoided);
		Task->ReadyForActivation();
		return Task;
	}

	void Apply(float Amount, bool bSelfDestruct = false, bool bHealing = false)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = bHealing ? UDCHealthSet::GetHealingAttribute() : UDCHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Amount);
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(Attacker, Attacker);
		FGameplayEffectSpec Spec(Effect, Context, 1.0f);
		if (bSelfDestruct) Spec.AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
	int32 ImmunityCount() const { return ASC->GetGameplayTagCount(DCGameplayTags::Gameplay_DamageImmunity); }
	FActiveGameplayEffectHandle AddOtherImmunity()
	{
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UDCDashDefenseAutomationEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
		Spec.Data->SetDuration(2.0f, true);
		Spec.Data->GetContext().AddSourceObject(Attacker);
		return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDashDefenseDamageWindowTest, "DreamCatcher.R9.DashDefense.DamageWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDashDefenseDamageWindowTest::RunTest(const FString&)
{
	DCDashDefenseTests::FFixture F; if (!F.Start(*this)) return false;
	F.Apply(25.0f); TestEqual(TEXT("Damage before window"), F.Health->GetHealth(), 75.0f);
	UDCAbilityTask_DashDefenseWindow* Task = F.Open();
	TestTrue(TEXT("Window opened"), Task->IsDefenseWindowActive());
	TestEqual(TEXT("Only own immunity granted"), F.ImmunityCount(), 1);
	F.Apply(0.0f); F.Apply(10.0f, false, true);
	TestEqual(TEXT("Healing is not blocked"), F.Health->GetHealth(), 85.0f);
	TestEqual(TEXT("No false success on zero/healing"), F.Successes, 0);
	const FDelegateHandle Reentrant = F.ASC->GenericGameplayEventCallbacks.FindOrAdd(DCGameplayTags::GameplayEvent_Dodge_Success).AddLambda(
		[&F](const FGameplayEventData*) { F.Apply(3.0f); });
	F.Apply(20.0f);
	F.ASC->GenericGameplayEventCallbacks.FindOrAdd(DCGameplayTags::GameplayEvent_Dodge_Success).Remove(Reentrant);
	F.Apply(25.0f);
	TestEqual(TEXT("Real Damage modifiers blocked"), F.Health->GetHealth(), 85.0f);
	TestEqual(TEXT("One gameplay event per window"), F.Successes, 1);
	TestEqual(TEXT("One BP success delegate"), F.Ability->Avoided, 1);
	TestEqual(TEXT("First blocked magnitude"), F.LastSuccess.EventMagnitude, 20.0f);
	TestTrue(TEXT("Attacker and avatar copied"), F.LastSuccess.Instigator == F.Attacker && F.LastSuccess.Target == F.Pawn);
	TestTrue(TEXT("Effect context preserved"), F.LastSuccess.ContextHandle.GetOriginalInstigator() == F.Attacker);
	F.World.TickTestWorld(0.30f);
	TestTrue(TEXT("Task expires"), Task->IsFinished());
	TestEqual(TEXT("No residual immunity"), F.ImmunityCount(), 0);
	TestEqual(TEXT("One opened/closed notification"), F.Ability->Opened, 1);
	TestEqual(TEXT("Closed once"), F.Ability->Closed, 1);
	F.Apply(25.0f); TestEqual(TEXT("Damage after window"), F.Health->GetHealth(), 60.0f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDashDefenseOverlapTest, "DreamCatcher.R9.DashDefense.OverlapAndCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDashDefenseOverlapTest::RunTest(const FString&)
{
	DCDashDefenseTests::FFixture F; if (!F.Start(*this)) return false;
	UDCAbilityTask_DashDefenseWindow* Task = F.Open();
	UDCAbilityTask_DashDefenseWindow* Duplicate = F.Open();
	TestTrue(TEXT("Duplicate execution refused"), Duplicate->IsFinished());
	TestEqual(TEXT("No duplicate effect"), F.ImmunityCount(), 1);
	F.ASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	F.Apply(25.0f);
	TestEqual(TEXT("Other immunity still blocks"), F.Health->GetHealth(), 100.0f);
	TestEqual(TEXT("Overlapping immunity not credited to Dash"), F.Successes, 0);
	F.ASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	F.ASC->AddLooseGameplayTag(DCGameplayTags::Cheat_GodMode);
	F.Apply(25.0f); TestEqual(TEXT("God mode not credited"), F.Successes, 0);
	F.ASC->RemoveLooseGameplayTag(DCGameplayTags::Cheat_GodMode);
	F.ASC->AddLooseGameplayTag(DCGameplayTags::Cheat_UnlimitedHealth);
	F.Apply(25.0f); TestEqual(TEXT("Unlimited health not credited"), F.Successes, 0);
	F.ASC->RemoveLooseGameplayTag(DCGameplayTags::Cheat_UnlimitedHealth);
	F.Apply(25.0f); TestEqual(TEXT("Sole Dash protection is credited"), F.Successes, 1);
	const FActiveGameplayEffectHandle Other = F.AddOtherImmunity();
	TestTrue(TEXT("Other GE applied"), Other.IsValid());
	F.ASC->CancelAllAbilities();
	TestTrue(TEXT("Cancel destroys task"), Task->IsFinished());
	TestEqual(TEXT("Unrelated immunity survives"), F.ImmunityCount(), 1);
	TestNotNull(TEXT("Other GE handle still exists"), F.ASC->GetActiveGameplayEffect(Other));
	F.World.TickTestWorld(0.40f);
	TestEqual(TEXT("No delayed second decrement"), F.ImmunityCount(), 1);
	TestEqual(TEXT("Closed only once"), F.Ability->Closed, 1);
	F.ASC->RemoveActiveGameplayEffect(Other);
	F.Apply(25.0f); TestEqual(TEXT("Normal damage restored"), F.Health->GetHealth(), 75.0f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDashDefenseFailureAndDeathTest, "DreamCatcher.R9.DashDefense.FailureAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDashDefenseFailureAndDeathTest::RunTest(const FString&)
{
	DCDashDefenseTests::FFixture F; if (!F.Start(*this)) return false;
	TestTrue(TEXT("Commit failure grants no window"), F.Open(false)->IsFinished());
	TestTrue(TEXT("Zero duration refused"), F.Open(true, 0.0f)->IsFinished());
	TestTrue(TEXT("Negative duration refused"), F.Open(true, -1.0f)->IsFinished());
	F.Pawn->SetRole(ROLE_SimulatedProxy);
	TestTrue(TEXT("Nonauthority owner cannot grant immunity"), F.Open()->IsFinished());
	F.Pawn->SetRole(ROLE_Authority);
	TestEqual(TEXT("No rejection tags or opened event"), F.ImmunityCount(), 0);
	TestEqual(TEXT("Opened none"), F.Ability->Opened, 0);
	UDCAbilityTask_DashDefenseWindow* Task = F.Open();
	F.Apply(200.0f, true);
	TestEqual(TEXT("Original SelfDestruct exception preserved"), F.Health->GetHealth(), 0.0f);
	TestTrue(TEXT("Out-of-health ends window immediately"), Task->IsFinished());
	TestEqual(TEXT("Death leaves no immunity"), F.ImmunityCount(), 0);
	TestEqual(TEXT("No success for SelfDestruct"), F.Successes, 0);
	TestTrue(TEXT("Dead avatar cannot reopen"), F.Open()->IsFinished());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDashDefenseAvatarTest, "DreamCatcher.R9.DashDefense.AvatarAndExternalRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDashDefenseAvatarTest::RunTest(const FString&)
{
	DCDashDefenseTests::FFixture F; if (!F.Start(*this)) return false;
	UDCAbilityTask_DashDefenseWindow* Task = F.Open();
	F.ASC->RemoveActiveGameplayEffect(Task->GetDefenseEffectHandle());
	TestTrue(TEXT("External removal ends task"), Task->IsFinished());
	TestEqual(TEXT("Removal has no residual immunity"), F.ImmunityCount(), 0);
	Task = F.Open();
	APawn* NextAvatar = F.World.GetTestWorld()->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Replacement avatar"), NextAvatar)) return false;
	F.ASC->InitAbilityActorInfo(F.Pawn, NextAvatar);
	F.World.TickTestWorld(0.01f);
	TestTrue(TEXT("Direct avatar replacement ends old window by next tick"), Task->IsFinished());
	TestEqual(TEXT("New avatar does not inherit immunity"), F.ImmunityCount(), 0);
	Task = F.Open();
	F.ASC->ClearActorInfo();
	F.World.TickTestWorld(0.01f);
	TestTrue(TEXT("Cleared actor info ends window"), Task->IsFinished());
	TestEqual(TEXT("Cleared ASC keeps no immunity"), F.ImmunityCount(), 0);
	F.ASC->InitAbilityActorInfo(F.Pawn, NextAvatar);
	Task = F.Open();
	NextAvatar->Destroy();
	TestTrue(TEXT("Avatar EndPlay ends window"), Task->IsFinished());
	TestEqual(TEXT("Destroyed avatar leaves no immunity"), F.ImmunityCount(), 0);
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
