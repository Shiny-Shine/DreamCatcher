// Unsaved GAS regressions. Human-experience setup is tested separately from NPC fixtures.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayEffectContext.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Character/DCLyraHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameplayEffect.h"
#include "Messages/DCVerbMessage.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Player/Lyra/DCLyraExperiencePlayerState.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCEnemyGASTestTypes.h"
#include "Tests/DCStageFlowTestTypes.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Weapon/Lyra/DCLyraRangedWeaponInstance.h"
#include <limits>

struct FDCArmorTestAccess
{
	static bool Initialize(ADCLyraExperiencePlayerState* State, const UDCLyraExperienceDefinition* Data)
	{ return State->InitializePlayerStatus(Data); }
	static bool Initialized(const ADCLyraExperiencePlayerState* State) { return State->bPlayerStatusInitialized; }
};

namespace DCArmorTests
{
struct FFixture
{
	FTestWorldWrapper World;
	bool Start(FAutomationTestBase& Test, bool bExperienceWorld = false)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCArmorIsolatedAutomation")))
		{ Test.AddError(TEXT("Use an isolated unattended editor with -DCArmorIsolatedAutomation; no assets are saved.")); return false; }
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		// Experience PlayerState PostInitializeComponents requires the real manager;
		// reuse the already-tested R10 assembly world without selecting any asset.
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = bExperienceWorld
			? ADCR10AssemblyGameModeFixture::StaticClass() : ADCR11ModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return Test.TestTrue(TEXT("Message subsystem available"), UGameplayMessageSubsystem::HasInstance(World.GetTestWorld()));
	}
	template<class T> T* Spawn(AActor* Owner = nullptr)
	{
		FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
		Params.Owner = Owner;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform(FVector(0, 0, 300)), Params);
	}
	ADCR10EnemyFixture* Target(float Armor = 150.f)
	{
		ADCR10EnemyFixture* Actor = Spawn<ADCR10EnemyFixture>();
		if (!Actor || !Actor->IsGASReady()) { return nullptr; }
		UAbilitySystemComponent* ASC = Actor->GetAbilitySystemComponent();
		ASC->SetNumericAttributeBase(UDCHealthSet::GetMaxArmorAttribute(), 150.f);
		ASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), Armor);
		return Actor;
	}
};

const UDCHealthSet* Stats(ADCR10EnemyFixture* Actor)
{ return Actor->GetAbilitySystemComponent()->GetSet<UDCHealthSet>(); }

UGameplayEffect* Modifier(const FGameplayAttribute& Attribute, float Amount)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Mod = Effect->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = Attribute; Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FScalableFloat(Amount);
	return Effect;
}

bool Apply(AActor* Source, AActor* Target, UGameplayEffect* Effect,
	UDCLyraRangedWeaponInstance* Weapon = nullptr, bool bSelfDestruct = false)
{
	if (!IsValid(Source) || !IsValid(Target) || !Effect) { return false; }
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC) { return false; }
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Source, Source); Context.AddOrigin(Source->GetActorLocation());
	FDCGameplayEffectContext* Typed = FDCGameplayEffectContext::ExtractEffectContext(Context);
	if (!Typed) { return false; }
	Typed->SetAbilitySource(Weapon, 1.f);
	FGameplayEffectSpec Spec(Effect, Context, 1.f);
	if (bSelfDestruct) { Spec.AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct); }
	SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
	return true;
}

void Check(FAutomationTestBase& Test, const UDCHealthSet* Set, float Armor, float Health)
{
	Test.TestTrue(TEXT("Actual armor after GE"), FMath::IsNearlyEqual(Set->GetArmor(), Armor, .001f));
	Test.TestTrue(TEXT("Actual HP after GE"), FMath::IsNearlyEqual(Set->GetHealth(), Health, .001f));
	Test.TestEqual(TEXT("Damage meta consumed exactly once"), Set->GetDamage(), 0.f);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCArmorAllocationTest, "DreamCatcher.UI.PlayerStatus.Armor.AllocationAndWeaponSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCArmorAllocationTest::RunTest(const FString&)
{
	using namespace DCArmorTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	UClass* RifleClass = LoadClass<UDCLyraRangedWeaponInstance>(nullptr,
		TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/B_WeaponInstance_Rifle.B_WeaponInstance_Rifle_C"));
	if (!TestNotNull(TEXT("Existing actual rifle instance BP"), RifleClass)) { return false; }
	FFloatProperty* RatioProperty = FindFProperty<FFloatProperty>(RifleClass, TEXT("DamageToArmorRatio"));
	if (!TestNotNull(TEXT("Editable ratio property"), RatioProperty)) { return false; }
	const float OriginalCDO = RatioProperty->GetPropertyValue_InContainer(RifleClass->GetDefaultObject());
	TestEqual(TEXT("Native default ratio"), GetDefault<UDCLyraRangedWeaponInstance>()->GetDamageToArmorRatio(), .8f);
	TestEqual(TEXT("Shared NPC default armor remains zero"), GetDefault<UDCHealthSet>()->GetArmor(), 0.f);
	TestEqual(TEXT("Shared NPC default max armor remains zero"), GetDefault<UDCHealthSet>()->GetMaxArmor(), 0.f);
	struct FCase { float Armor, Ratio, AfterArmor, AfterHP; };
	const FCase Cases[] = {{150, .8f, 70, 80}, {50, .8f, 0, 80}, {0, .8f, 0, 0},
		{150, 0, 150, 0}, {150, 1, 50, 100}, {150, .25f, 125, 25}, {150, .75f, 75, 75}};
	TStrongObjectPtr<UGameplayEffect> Hit(Modifier(UDCHealthSet::GetDamageAttribute(), 100.f));
	for (const FCase& C : Cases)
	{
		ADCR10EnemyFixture* Target = F.Target(C.Armor); if (!TestNotNull(TEXT("Actual GAS target"), Target)) { return false; }
		TStrongObjectPtr<UDCLyraRangedWeaponInstance> Weapon(NewObject<UDCLyraRangedWeaponInstance>(Target, RifleClass, NAME_None, RF_Transient));
		RatioProperty->SetPropertyValue_InContainer(Weapon.Get(), C.Ratio);
		TestTrue(TEXT("Actual GE uses context weapon instance"), Apply(Target, Target, Hit.Get(), Weapon.Get()));
		Check(*this, Stats(Target), C.AfterArmor, C.AfterHP);
		if (C.Armor == 50.f && C.Ratio == .8f)
		{
			TestTrue(TEXT("Next hit after armor breaks applies full HP damage"), Apply(Target, Target, Hit.Get(), Weapon.Get()));
			Check(*this, Stats(Target), 0, 0);
		}
	}
	for (float Invalid : {-1.f, 1.1f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
	{
		ADCR10EnemyFixture* Target = F.Target(); if (!TestNotNull(TEXT("Invalid ratio target"), Target)) { return false; }
		TStrongObjectPtr<UDCLyraRangedWeaponInstance> Weapon(NewObject<UDCLyraRangedWeaponInstance>(Target, RifleClass, NAME_None, RF_Transient));
		RatioProperty->SetPropertyValue_InContainer(Weapon.Get(), Invalid);
		TestEqual(TEXT("Invalid authored ratio has safe runtime fallback"), Weapon->GetDamageToArmorRatio(), .8f);
		FDataValidationContext Validation;
		TestTrue(TEXT("Invalid authoring is diagnosed without mutation"), Weapon->IsDataValid(Validation) == EDataValidationResult::Invalid);
		TestTrue(TEXT("Invalid source value cannot poison resources"), Apply(Target, Target, Hit.Get(), Weapon.Get()));
		Check(*this, Stats(Target), 70, 80);
	}
	// The unchanged original execution still captures BaseDamage and emits the same D.
	ADCR10EnemyFixture* Target = F.Target(); if (!TestNotNull(TEXT("Original execution target"), Target)) { return false; }
	Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), 100.f);
	TStrongObjectPtr<UGameplayEffect> Execution(NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient));
	Execution->DurationPolicy = EGameplayEffectDurationType::Instant;
	Execution->Executions.AddDefaulted_GetRef().CalculationClass = UDCLyraDamageExecution::StaticClass();
	TestTrue(TEXT("No weapon source uses default ratio through original execution"), Apply(Target, Target, Execution.Get()));
	Check(*this, Stats(Target), 70, 80);
	TestEqual(TEXT("Only transient instances changed; original weapon CDO preserved"), RatioProperty->GetPropertyValue_InContainer(RifleClass->GetDefaultObject()), OriginalCDO);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCArmorBindingTest, "DreamCatcher.UI.PlayerStatus.Armor.ClampsAndPawnBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCArmorBindingTest::RunTest(const FString&)
{
	using namespace DCArmorTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10EnemyFixture* Target = F.Target(); if (!TestNotNull(TEXT("Clamp target"), Target)) { return false; }
	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent(); const UDCHealthSet* Set = Stats(Target);
	int32 ArmorEvents = 0, MaxEvents = 0;
	const FDelegateHandle A = Set->OnArmorChanged.AddLambda([&](AActor*, AActor*, const FGameplayEffectSpec*, float, float, float)
	{ ++ArmorEvents; TestTrue(TEXT("Armor observer sees fully clamped pair"), Set->GetArmor() >= 0 && Set->GetArmor() <= Set->GetMaxArmor()); });
	const FDelegateHandle M = Set->OnMaxArmorChanged.AddLambda([&](AActor*, AActor*, const FGameplayEffectSpec*, float, float, float) { ++MaxEvents; });
	ASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), 999.f);
	TestEqual(TEXT("Upper clamp is a no-op at full armor"), ArmorEvents, 0);
	ASC->SetNumericAttributeBase(UDCHealthSet::GetMaxArmorAttribute(), 50.f);
	TestEqual(TEXT("Max clamp emits exactly one armor event"), ArmorEvents, 1);
	TestEqual(TEXT("Max clamp emits exactly one max event"), MaxEvents, 1);
	Check(*this, Set, 50, 100);
	ASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), -10.f); Check(*this, Set, 0, 100);
	ASC->SetNumericAttributeBase(UDCHealthSet::GetMaxArmorAttribute(), std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("Invalid maximum cannot produce NaN"), Set->GetMaxArmor(), 0.f);
	Set->OnArmorChanged.Remove(A); Set->OnMaxArmorChanged.Remove(M);

	ADCPlayerState* PS = F.Spawn<ADCPlayerState>(); APawn* First = F.Spawn<APawn>(); APawn* Next = F.Spawn<APawn>();
	if (!TestNotNull(TEXT("Persistent PlayerState"), PS) || !TestNotNull(TEXT("First pawn"), First) || !TestNotNull(TEXT("Replacement pawn"), Next)) { return false; }
	UDCAbilitySystemComponent* PlayerASC = PS->GetDCAbilitySystemComponent();
	PlayerASC->InitAbilityActorInfo(PS, First);
	PlayerASC->SetNumericAttributeBase(UDCHealthSet::GetMaxArmorAttribute(), 150);
	PlayerASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), 75);
	UDCLyraHealthComponent* Old = NewObject<UDCLyraHealthComponent>(First, NAME_None, RF_Transient);
	First->AddInstanceComponent(Old); Old->RegisterComponent(); Old->InitializeWithAbilitySystem(PlayerASC);
	TestEqual(TEXT("Pawn binding never refills armor"), Old->GetArmor(), 75.f);
	TestEqual(TEXT("Normalized armor"), Old->GetArmorNormalized(), .5f);
	TestTrue(TEXT("Original armor callback is bound"), PS->GetHealthSet()->OnArmorChanged.IsBoundToObject(Old));
	Old->UninitializeFromAbilitySystem(); TestEqual(TEXT("Detached component is safe"), Old->GetArmor(), 0.f);
	TestFalse(TEXT("Old armor callback removed"), PS->GetHealthSet()->OnArmorChanged.IsBoundToObject(Old));
	TestFalse(TEXT("Old max armor callback removed"), PS->GetHealthSet()->OnMaxArmorChanged.IsBoundToObject(Old));
	PlayerASC->InitAbilityActorInfo(PS, Next);
	UDCLyraHealthComponent* New = NewObject<UDCLyraHealthComponent>(Next, NAME_None, RF_Transient);
	Next->AddInstanceComponent(New); New->RegisterComponent(); New->InitializeWithAbilitySystem(PlayerASC);
	TestEqual(TEXT("Replacement pawn inherits existing ASC armor"), New->GetArmor(), 75.f);
	TestEqual(TEXT("Replacement pawn inherits max armor"), New->GetMaxArmor(), 150.f);
	PlayerASC->SetNumericAttributeBase(UDCHealthSet::GetArmorAttribute(), 25);
	TestEqual(TEXT("New binding reads current value"), New->GetArmor(), 25.f);
	TestEqual(TEXT("Old component remains detached"), Old->GetArmor(), 0.f);
	New->UninitializeFromAbilitySystem();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCArmorGuardsTest, "DreamCatcher.UI.PlayerStatus.Armor.BlockingHealingAndSelfDestruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCArmorGuardsTest::RunTest(const FString&)
{
	using namespace DCArmorTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10EnemyFixture* Target = F.Target(); if (!TestNotNull(TEXT("Guard target"), Target)) { return false; }
	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent(); const UDCHealthSet* Set = Stats(Target);
	TStrongObjectPtr<UGameplayEffect> Hit(Modifier(UDCHealthSet::GetDamageAttribute(), 100));
	int32 Blocked = 0, Changed = 0;
	const FDelegateHandle B = Set->OnDamageBlocked.AddLambda([&](const FGameplayEffectSpec&, float Amount)
	{ ++Blocked; TestEqual(TEXT("Original blocked raw amount"), Amount, 100.f); Check(*this, Set, 150, 100); });
	const FDelegateHandle A = Set->OnArmorChanged.AddLambda([&](AActor*, AActor*, const FGameplayEffectSpec*, float, float, float) { ++Changed; });
	ASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
	TestTrue(TEXT("Real immune GE"), Apply(Target, Target, Hit.Get())); Check(*this, Set, 150, 100);
	TestEqual(TEXT("One original blocked notification"), Blocked, 1);
	TestEqual(TEXT("Blocked hit consumes no armor"), Changed, 0);
	ASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
#if !UE_BUILD_SHIPPING
	ASC->AddLooseGameplayTag(DCGameplayTags::Cheat_GodMode);
	TestTrue(TEXT("GodMode blocks before armor"), Apply(Target, Target, Hit.Get())); Check(*this, Set, 150, 100);
	ASC->RemoveLooseGameplayTag(DCGameplayTags::Cheat_GodMode);
#endif
	ASC->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 50);
	TStrongObjectPtr<UGameplayEffect> Negative(Modifier(UDCHealthSet::GetDamageAttribute(), -10));
	TStrongObjectPtr<UGameplayEffect> Zero(Modifier(UDCHealthSet::GetDamageAttribute(), 0));
	TStrongObjectPtr<UGameplayEffect> Heal(Modifier(UDCHealthSet::GetHealingAttribute(), 20));
	TestTrue(TEXT("Negative damage retains legacy HP behavior"), Apply(Target, Target, Negative.Get())); Check(*this, Set, 150, 60);
	TestTrue(TEXT("Zero damage is not armor consumption"), Apply(Target, Target, Zero.Get())); Check(*this, Set, 150, 60);
	TestTrue(TEXT("Healing changes HP only"), Apply(Target, Target, Heal.Get())); Check(*this, Set, 150, 80);
	TestEqual(TEXT("No armor notification for HP-only operations"), Changed, 0);
	TestEqual(TEXT("Healing meta consumed"), Set->GetHealing(), 0.f);
	TestTrue(TEXT("Consume armor before testing recovery"), Apply(Target, Target, Hit.Get())); Check(*this, Set, 70, 60);
	TestTrue(TEXT("Healing cannot refill depleted armor"), Apply(Target, Target, Heal.Get())); Check(*this, Set, 70, 80);
	TestEqual(TEXT("Only the actual damaging hit changed armor"), Changed, 1);
	Set->OnDamageBlocked.Remove(B); Set->OnArmorChanged.Remove(A);
#if !UE_BUILD_SHIPPING
	ADCR10EnemyFixture* Unlimited = F.Target(); if (!TestNotNull(TEXT("Unlimited-health target"), Unlimited)) { return false; }
	Unlimited->GetAbilitySystemComponent()->AddLooseGameplayTag(DCGameplayTags::Cheat_UnlimitedHealth);
	TStrongObjectPtr<UGameplayEffect> LargeHit(Modifier(UDCHealthSet::GetDamageAttribute(), 1000));
	TestTrue(TEXT("Unlimited health preserves the original minimum HP"), Apply(Unlimited, Unlimited, LargeHit.Get()));
	Check(*this, Stats(Unlimited), 0, 1);
#endif
	ADCR10EnemyFixture* Self = F.Target(); if (!TestNotNull(TEXT("Self destruct target"), Self)) { return false; }
	Self->GetAbilitySystemComponent()->AddLooseGameplayTag(DCGameplayTags::Gameplay_DamageImmunity);
#if !UE_BUILD_SHIPPING
	Self->GetAbilitySystemComponent()->AddLooseGameplayTag(DCGameplayTags::Cheat_GodMode);
#endif
	TestTrue(TEXT("Tagged self destruct bypasses armor and immunity"), Apply(Self, Self, Hit.Get(), nullptr, true));
	Check(*this, Stats(Self), 150, 0);
	TestTrue(TEXT("Original Death ability still starts"), Self->IsDeadOrDying());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCArmorReentryTest, "DreamCatcher.UI.PlayerStatus.Armor.ReentryAndFatalCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCArmorReentryTest::RunTest(const FString&)
{
	using namespace DCArmorTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	ADCR10EnemyFixture* OuterSource = F.Spawn<ADCR10EnemyFixture>(); ADCR10EnemyFixture* InnerSource = F.Spawn<ADCR10EnemyFixture>();
	if (!TestNotNull(TEXT("Outer instigator"), OuterSource) || !TestNotNull(TEXT("Different inner instigator"), InnerSource)) { return false; }
	TStrongObjectPtr<UGameplayEffect> OuterHit(Modifier(UDCHealthSet::GetDamageAttribute(), 100));
	TStrongObjectPtr<UGameplayEffect> InnerHit(Modifier(UDCHealthSet::GetDamageAttribute(), 500));
	TStrongObjectPtr<UGameplayEffect> Heal(Modifier(UDCHealthSet::GetHealingAttribute(), 100));
	UGameplayMessageSubsystem& Messages = UGameplayMessageSubsystem::Get(F.World.GetTestWorld());
	// Modes: raw-message lethal reentry; armor-event lethal reentry; outer lethal
	// with extra HP-zero damage; outer lethal healed then killed by another cause.
	for (int32 Mode = 0; Mode < 4; ++Mode)
	{
		ADCR10EnemyFixture* Target = F.Target(Mode >= 2 ? 0.f : 150.f);
		if (!TestNotNull(TEXT("Reentry target"), Target)) { return false; }
		const UDCHealthSet* Set = Stats(Target);
		int32 Deaths = 0, RawMessages = 0, ArmorEvents = 0; bool Injected = false, ExtraAfterDeath = false;
		const FDelegateHandle D = Set->OnOutOfHealth.AddLambda([&](AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float, float)
		{
			++Deaths;
			const bool bOuterOwns = Mode == 2;
			TestTrue(TEXT("First actual fatal GE owns the death spec"), Spec && Spec->Def == (bOuterOwns ? OuterHit.Get() : InnerHit.Get()));
			TestTrue(TEXT("Fatal instigator is preserved"), Instigator == (bOuterOwns ? OuterSource : InnerSource));
			TestTrue(TEXT("Fatal causer is preserved"), Causer == (bOuterOwns ? OuterSource : InnerSource));
			TestEqual(TEXT("Fatal raw magnitude is preserved"), Magnitude, bOuterOwns ? 100.f : 500.f);
			TestEqual(TEXT("Death listener sees consumed meta"), Set->GetDamage(), 0.f);
			if (!ExtraAfterDeath) { ExtraAfterDeath = true; TestTrue(TEXT("HP-zero reentry does not notify death twice"), Apply(InnerSource, Target, InnerHit.Get())); }
		});
		const FDelegateHandle A = Set->OnArmorChanged.AddLambda([&](AActor*, AActor*, const FGameplayEffectSpec*, float, float, float)
		{
			++ArmorEvents; TestEqual(TEXT("Armor listener sees consumed meta"), Set->GetDamage(), 0.f);
			if (Mode == 1 && !Injected)
			{
				Injected = true; Check(*this, Set, 70, 80);
				TestTrue(TEXT("New armor event may apply a real second GE"), Apply(InnerSource, Target, InnerHit.Get()));
			}
		});
		FGameplayMessageListenerHandle L = Messages.RegisterListener<FDCVerbMessage>(TAG_Lyra_Damage_Message,
			[&](FGameplayTag, const FDCVerbMessage& Message)
			{
				if (Message.Target != Target) { return; }
				++RawMessages; TestEqual(TEXT("Raw message sees empty meta"), Set->GetDamage(), 0.f);
				if (Mode != 1 && !Injected && Message.Instigator == OuterSource)
				{
					Injected = true;
					if (Mode == 0) { Check(*this, Set, 70, 80); }
					if (Mode == 3) { TestTrue(TEXT("Message-time recovery cancels the older fatal reservation"), Apply(InnerSource, Target, Heal.Get())); }
					TestTrue(TEXT("Raw message may apply another source's real GE"), Apply(InnerSource, Target, InnerHit.Get()));
				}
			});
		TestTrue(TEXT("Outer real damage GE"), Apply(OuterSource, Target, OuterHit.Get()));
		Check(*this, Set, 0, 0);
		TestEqual(TEXT("One fatal notification despite nested GE"), Deaths, 1);
		TestEqual(TEXT("Each actual GE emits one original raw message"), RawMessages, 3);
		TestEqual(TEXT("Only actual armor changes notify"), ArmorEvents, Mode < 2 ? 2 : 0);
		L.Unregister(); Set->OnOutOfHealth.Remove(D); Set->OnArmorChanged.Remove(A);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCArmorExperienceTest, "DreamCatcher.UI.PlayerStatus.Armor.HumanExperienceInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCArmorExperienceTest::RunTest(const FString&)
{
	using namespace DCArmorTests;
	FFixture F; if (!F.Start(*this, true)) { return false; }
	APlayerController* Human = F.Spawn<APlayerController>();
	ADCLyraPlayerBotController* Bot = F.Spawn<ADCLyraPlayerBotController>();
	AAIController* Other = F.Spawn<AAIController>();
	if (!TestNotNull(TEXT("Human controller"), Human) || !TestNotNull(TEXT("Bot controller"), Bot) || !TestNotNull(TEXT("Other AI"), Other)) { return false; }
	auto NewState = [&](AController* Owner)
	{
		ADCLyraExperiencePlayerState* State = F.Spawn<ADCLyraExperiencePlayerState>(Owner);
		if (State) { State->GetDCAbilitySystemComponent()->InitAbilityActorInfo(State, State); }
		return State;
	};
	TStrongObjectPtr<UDCLyraExperienceDefinition> Data(NewObject<UDCLyraExperienceDefinition>());
	ADCLyraExperiencePlayerState* State = NewState(Human);
	if (!TestNotNull(TEXT("Human Experience PlayerState"), State)) { return false; }
	TestFalse(TEXT("Every other Experience is opt-out by default"), Data->bUsePlayerStatusDefaults);
	State->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 73);
	TestTrue(TEXT("Opt-out leaves the shared defaults alone"), FDCArmorTestAccess::Initialize(State, Data.Get()));
	TestEqual(TEXT("Opt-out HP is untouched"), State->GetHealthSet()->GetHealth(), 73.f);
	TestEqual(TEXT("Opt-out armor is untouched"), State->GetHealthSet()->GetArmor(), 0.f);
	TestFalse(TEXT("Opt-out does not lock initialization"), FDCArmorTestAccess::Initialized(State));
	Data->bUsePlayerStatusDefaults = true;
	State->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), 33);
	TestTrue(TEXT("Human opted-in initialization"), FDCArmorTestAccess::Initialize(State, Data.Get()));
	TestEqual(TEXT("Approved human HP"), State->GetHealthSet()->GetHealth(), 100.f);
	TestEqual(TEXT("Approved human max HP"), State->GetHealthSet()->GetMaxHealth(), 100.f);
	TestEqual(TEXT("Approved single armor pool"), State->GetHealthSet()->GetArmor(), 150.f);
	TestEqual(TEXT("Approved max armor"), State->GetHealthSet()->GetMaxArmor(), 150.f);
	TestEqual(TEXT("Existing BaseDamage is not changed"), State->GetCombatSet()->GetBaseDamage(), 33.f);
	TStrongObjectPtr<UGameplayEffect> Hit(Modifier(UDCHealthSet::GetDamageAttribute(), 100));
	TestTrue(TEXT("Actually damage the initialized ASC"), Apply(State, State, Hit.Get()));
	Check(*this, State->GetHealthSet(), 70, 80);
	Data->PlayerMaxHealth = 200; Data->PlayerMaxArmor = 300; Data->PlayerInitialArmor = 250;
	TestTrue(TEXT("Repeated callback after Editor data edit is a no-op"), FDCArmorTestAccess::Initialize(State, Data.Get()));
	TestTrue(TEXT("Initialized stale callback does not refill"), FDCArmorTestAccess::Initialize(State, nullptr));
	Check(*this, State->GetHealthSet(), 70, 80);
	TestEqual(TEXT("Existing run retains its max HP"), State->GetHealthSet()->GetMaxHealth(), 100.f);
	TestEqual(TEXT("Existing run retains its max armor"), State->GetHealthSet()->GetMaxArmor(), 150.f);
	Data->PlayerMaxHealth = 100; Data->PlayerMaxArmor = 150; Data->PlayerInitialArmor = 150;
	ADCLyraExperiencePlayerState* Fresh = NewState(Human);
	if (!TestNotNull(TEXT("Fresh run PlayerState"), Fresh)) { return false; }
	TestTrue(TEXT("New run restores approved initial values"), FDCArmorTestAccess::Initialize(Fresh, Data.Get()));
	Check(*this, Fresh->GetHealthSet(), 150, 100);
	for (AController* Owner : {static_cast<AController*>(Bot), static_cast<AController*>(Other)})
	{
		ADCLyraExperiencePlayerState* Skipped = NewState(Owner); if (!TestNotNull(TEXT("Nonhuman state"), Skipped)) { return false; }
		Skipped->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 61);
		TestTrue(TEXT("Bots and AI remain unchanged"), FDCArmorTestAccess::Initialize(Skipped, Data.Get()));
		TestEqual(TEXT("Nonhuman HP is preserved"), Skipped->GetHealthSet()->GetHealth(), 61.f);
		TestEqual(TEXT("Nonhuman armor remains zero"), Skipped->GetHealthSet()->GetArmor(), 0.f);
		TestFalse(TEXT("Nonhuman path never locks human initialization"), FDCArmorTestAccess::Initialized(Skipped));
	}
	ADCLyraExperiencePlayerState* Invalid = NewState(Human);
	if (!TestNotNull(TEXT("Invalid-settings state"), Invalid)) { return false; }
	const float NaN = std::numeric_limits<float>::quiet_NaN(), Inf = std::numeric_limits<float>::infinity();
	struct FInvalid { float Health, MaxArmor, InitialArmor; };
	const FInvalid BadValues[] = {{0, 150, 150}, {NaN, 150, 150}, {Inf, 150, 150},
		{100, -1, 0}, {100, NaN, 0}, {100, Inf, 150}, {100, 150, -1}, {100, 150, NaN}, {100, 150, Inf}};
	for (const FInvalid& Value : BadValues)
	{
		Data->PlayerMaxHealth = Value.Health; Data->PlayerMaxArmor = Value.MaxArmor; Data->PlayerInitialArmor = Value.InitialArmor;
		FDataValidationContext Validation; TestTrue(TEXT("Invalid status data diagnosed"), Data->IsDataValid(Validation) == EDataValidationResult::Invalid);
		AddExpectedError(TEXT("requires finite MaxHealth >= 1"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("Invalid settings rejected before mutation"), FDCArmorTestAccess::Initialize(Invalid, Data.Get()));
		TestFalse(TEXT("Invalid setting does not lock initialization"), FDCArmorTestAccess::Initialized(Invalid));
		Check(*this, Invalid->GetHealthSet(), 0, 100);
	}
	Data->PlayerMaxHealth = 100; Data->PlayerMaxArmor = 150; Data->PlayerInitialArmor = 151;
	AddExpectedError(TEXT("requires finite MaxHealth >= 1"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Initial armor above max is rejected"), FDCArmorTestAccess::Initialize(Invalid, Data.Get()));
	TestFalse(TEXT("Range failure leaves the flag unlocked"), FDCArmorTestAccess::Initialized(Invalid));
	Data->PlayerInitialArmor = 150;
	Invalid->SetRole(ROLE_SimulatedProxy);
	AddExpectedError(TEXT("on non-authority"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Client role cannot initialize"), FDCArmorTestAccess::Initialize(Invalid, Data.Get()));
	TestFalse(TEXT("Client role cannot consume the flag"), FDCArmorTestAccess::Initialized(Invalid));
	Invalid->SetRole(ROLE_Authority);
	TestTrue(TEXT("Corrected authority state may initialize"), FDCArmorTestAccess::Initialize(Invalid, Data.Get()));
	Check(*this, Invalid->GetHealthSet(), 150, 100);
	AddInfo(TEXT("Native initialization and ASC Pawn rebinding only; real Experience travel and network replication remain PIE checks."));
	return !HasAnyErrors();
}

#endif
