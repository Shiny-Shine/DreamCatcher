// Isolated role initialization and the actual Rifle GE/execution. No saved assets are modified.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "AIController.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Player/Lyra/DCLyraExperiencePlayerState.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

struct FDCBaseDamageTestAccess
{
	static bool Initialize(ADCLyraExperiencePlayerState* State, const UDCLyraExperienceDefinition* Experience)
	{
		return State->InitializeRoleBaseDamage(Experience);
	}
	static bool WasInitialized(const ADCLyraExperiencePlayerState* State)
	{
		return State->bRoleBaseDamageInitialized;
	}
};

namespace DCBaseDamageTests
{
FGameplayTag IgnoreBonusTag()
{
	return FGameplayTag::RequestGameplayTag(FName(TEXT("Gameplay.Damage.IgnoreWeaponBonus")));
}

struct FFixture
{
	FTestWorldWrapper World;
	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCBaseDamageIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate unattended editor with -DCBaseDamageIsolatedAutomation."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		// Existing fixture supplies the real Experience component but deliberately selects no saved Experience.
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR10AssemblyGameModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return true;
	}

	template <class T> T* Spawn(AActor* Owner = nullptr)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World.GetTestWorld()->SpawnActor<T>(T::StaticClass(), FTransform::Identity, Params);
	}

	ADCLyraExperiencePlayerState* State(FAutomationTestBase& Test, AController* Controller)
	{
		ADCLyraExperiencePlayerState* Result = Spawn<ADCLyraExperiencePlayerState>(Controller);
		if (!Test.TestNotNull(TEXT("Transient Experience PlayerState"), Result)) { return nullptr; }
		UDCAbilitySystemComponent* ASC = Result->GetDCAbilitySystemComponent();
		if (!Test.TestNotNull(TEXT("Original ASC"), ASC)
			|| !Test.TestTrue(TEXT("Original CombatSet registered"), ASC->GetSet<UDCCombatSet>() == Result->GetCombatSet()))
		{
			return nullptr;
		}
		// No Pawn/weapon fixture is needed for these initialization and execution checks.
		ASC->InitAbilityActorInfo(Result, Result);
		return Result;
	}
};

float Base(const ADCLyraExperiencePlayerState* State) { return State->GetCombatSet()->GetBaseDamage(); }
int32 TagCount(const ADCLyraExperiencePlayerState* State)
{
	return State->GetDCAbilitySystemComponent()->GetTagCount(IgnoreBonusTag());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBaseDamageDataTest, "DreamCatcher.Combat.BaseDamage.DataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBaseDamageDataTest::RunTest(const FString&)
{
	TStrongObjectPtr<UDCLyraExperienceDefinition> Data(NewObject<UDCLyraExperienceDefinition>());
	TestFalse(TEXT("Other Experiences remain opt-out"), Data->bUseRoleBasedBaseDamage);
	TestEqual(TEXT("Player default"), Data->PlayerBaseDamage, 0.0f);
	TestEqual(TEXT("Enemy default"), Data->EnemyBaseDamage, 12.0f);
	Data->bUseRoleBasedBaseDamage = true;
	FDataValidationContext Valid;
	TestTrue(TEXT("Approved defaults valid"), Data->IsDataValid(Valid) == EDataValidationResult::Valid);
	const float InvalidValues[] = {-1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()};
	for (float Value : InvalidValues)
	{
		Data->PlayerBaseDamage = Value;
		Data->EnemyBaseDamage = 12.0f;
		FDataValidationContext PlayerInvalid;
		TestTrue(TEXT("Invalid player value rejected"), Data->IsDataValid(PlayerInvalid) == EDataValidationResult::Invalid);
		TestEqual(TEXT("One player field error"), PlayerInvalid.GetNumErrors(), uint32(1));
		Data->PlayerBaseDamage = 0.0f;
		Data->EnemyBaseDamage = Value;
		FDataValidationContext EnemyInvalid;
		TestTrue(TEXT("Invalid enemy value rejected"), Data->IsDataValid(EnemyInvalid) == EDataValidationResult::Invalid);
		TestEqual(TEXT("One enemy field error"), EnemyInvalid.GetNumErrors(), uint32(1));
	}
	Data->bUseRoleBasedBaseDamage = false;
	FDataValidationContext Disabled;
	TestTrue(TEXT("Disabled settings do not invalidate another Experience"), Data->IsDataValid(Disabled) == EDataValidationResult::Valid);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBaseDamageInitializationTest, "DreamCatcher.Combat.BaseDamage.Initialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBaseDamageInitializationTest::RunTest(const FString&)
{
	using namespace DCBaseDamageTests;
	FFixture Fixture;
	if (!Fixture.Start(*this)) { return false; }
	APlayerController* Player = Fixture.Spawn<APlayerController>();
	ADCLyraPlayerBotController* Bot = Fixture.Spawn<ADCLyraPlayerBotController>();
	AAIController* Other = Fixture.Spawn<AAIController>();
	if (!TestNotNull(TEXT("Player owner"), Player) || !TestNotNull(TEXT("Bot owner"), Bot)
		|| !TestNotNull(TEXT("Unrelated AI owner"), Other)) { return false; }
	ADCLyraExperiencePlayerState* PlayerState = Fixture.State(*this, Player);
	ADCLyraExperiencePlayerState* BotState = Fixture.State(*this, Bot);
	ADCLyraExperiencePlayerState* OtherState = Fixture.State(*this, Other);
	if (!PlayerState || !BotState || !OtherState) { return false; }
	TStrongObjectPtr<UDCLyraExperienceDefinition> Data(NewObject<UDCLyraExperienceDefinition>());
	BotState->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), 7.0f);
	TestTrue(TEXT("Disabled initialization is a no-op"), FDCBaseDamageTestAccess::Initialize(BotState, Data.Get()));
	TestEqual(TEXT("Disabled preserves existing attribute"), Base(BotState), 7.0f);
	TestEqual(TEXT("Disabled adds no source tag"), TagCount(BotState), 0);
	TestFalse(TEXT("Disabled does not consume initialization"), FDCBaseDamageTestAccess::WasInitialized(BotState));
	Data->bUseRoleBasedBaseDamage = true;
	TestTrue(TEXT("Player initializes"), FDCBaseDamageTestAccess::Initialize(PlayerState, Data.Get()));
	TestTrue(TEXT("Bot initializes"), FDCBaseDamageTestAccess::Initialize(BotState, Data.Get()));
	TestEqual(TEXT("Player default base"), Base(PlayerState), 0.0f);
	TestEqual(TEXT("Bot default base replaces old value"), Base(BotState), 12.0f);
	TestEqual(TEXT("Player retains rifle bonus"), TagCount(PlayerState), 0);
	TestEqual(TEXT("Bot has one ignore tag"), TagCount(BotState), 1);
	// A later attribute change must survive a repeated Experience-ready callback.
	BotState->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), 5.0f);
	Data->PlayerBaseDamage = 8.0f;
	Data->EnemyBaseDamage = 8.0f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("Repeated player initialization accepted"), FDCBaseDamageTestAccess::Initialize(PlayerState, Data.Get()));
		TestTrue(TEXT("Repeated bot initialization accepted"), FDCBaseDamageTestAccess::Initialize(BotState, Data.Get()));
	}
	TestEqual(TEXT("Existing player not reset to changed Experience"), Base(PlayerState), 0.0f);
	TestEqual(TEXT("Runtime bot attribute change preserved"), Base(BotState), 5.0f);
	TestEqual(TEXT("Repeated initialization does not stack tag"), TagCount(BotState), 1);
	ADCLyraExperiencePlayerState* NewPlayer = Fixture.State(*this, Player);
	ADCLyraExperiencePlayerState* NewBot = Fixture.State(*this, Bot);
	if (!NewPlayer || !NewBot) { return false; }
	TestTrue(TEXT("New player gets current setting"), FDCBaseDamageTestAccess::Initialize(NewPlayer, Data.Get()));
	TestTrue(TEXT("New bot gets current setting"), FDCBaseDamageTestAccess::Initialize(NewBot, Data.Get()));
	TestEqual(TEXT("New player base"), Base(NewPlayer), 8.0f);
	TestEqual(TEXT("New bot base"), Base(NewBot), 8.0f);
	TestEqual(TEXT("New bot source tag"), TagCount(NewBot), 1);
	OtherState->GetDCAbilitySystemComponent()->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), 23.0f);
	TestTrue(TEXT("Unrelated AI skipped"), FDCBaseDamageTestAccess::Initialize(OtherState, Data.Get()));
	TestEqual(TEXT("Unrelated AI base preserved"), Base(OtherState), 23.0f);
	TestEqual(TEXT("Unrelated AI tag preserved"), TagCount(OtherState), 0);
	TestFalse(TEXT("Unrelated AI not marked initialized"), FDCBaseDamageTestAccess::WasInitialized(OtherState));

	ADCLyraExperiencePlayerState* Invalid = Fixture.State(*this, Bot);
	if (!Invalid) { return false; }
	Data->EnemyBaseDamage = -1.0f;
	AddExpectedError(TEXT("requires finite, non-negative player and enemy values"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Runtime invalid settings rejected"), FDCBaseDamageTestAccess::Initialize(Invalid, Data.Get()));
	TestEqual(TEXT("Invalid value leaves attribute untouched"), Base(Invalid), 0.0f);
	TestEqual(TEXT("Invalid value adds no tag"), TagCount(Invalid), 0);
	TestFalse(TEXT("Invalid settings do not consume initialization"), FDCBaseDamageTestAccess::WasInitialized(Invalid));
	Data->EnemyBaseDamage = 8.0f;
	TestTrue(TEXT("Corrected setting can initialize"), FDCBaseDamageTestAccess::Initialize(Invalid, Data.Get()));
	TestEqual(TEXT("Corrected setting applied"), Base(Invalid), 8.0f);
	AddInfo(TEXT("Native authority initialization only; real Experience-load timing, travel and replication require PIE checks."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBaseDamageRifleExecutionTest, "DreamCatcher.Combat.BaseDamage.SavedRifleExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBaseDamageRifleExecutionTest::RunTest(const FString&)
{
	using namespace DCBaseDamageTests;
	FFixture Fixture;
	if (!Fixture.Start(*this)) { return false; }
	// Run AFTER base_damage_setup.py apply + fresh verify. Do not patch the asset in a test.
	UClass* RifleClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Weapons/Rifle/GE_Damage_RifleAuto.GE_Damage_RifleAuto_C"));
	if (!TestNotNull(TEXT("Saved real Rifle GE"), RifleClass)) { return false; }
	const UGameplayEffect* Rifle = GetDefault<UGameplayEffect>(RifleClass);
	if (!TestEqual(TEXT("Original single execution retained"), Rifle->Executions.Num(), 1)) { return false; }
	const FGameplayEffectExecutionDefinition& Definition = Rifle->Executions[0];
	if (!TestTrue(TEXT("Original Lyra damage execution retained"), Definition.CalculationClass == UDCLyraDamageExecution::StaticClass())
		|| !TestEqual(TEXT("Original one scoped modifier retained"), Definition.CalculationModifiers.Num(), 1)) { return false; }
	const FGameplayEffectExecutionScopedModifierInfo& Bonus = Definition.CalculationModifiers[0];
	if (!TestTrue(TEXT("Approved source exclusion saved on the actual GE"), Bonus.SourceTags.IgnoreTags.HasTagExact(IgnoreBonusTag()))) { return false; }
	TestTrue(TEXT("BaseDamage source capture"), Bonus.CapturedAttribute == FGameplayEffectAttributeCaptureDefinition(
		UDCCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true));
	TestTrue(TEXT("Original additive operation"), Bonus.ModifierOp == EGameplayModOp::Additive);

	ADCPlayerState* Target = Fixture.Spawn<ADCPlayerState>();
	if (!TestNotNull(TEXT("Independent target"), Target)) { return false; }
	Target->SetGenericTeamId(FGenericTeamId(2));
	UDCAbilitySystemComponent* TargetASC = Target->GetDCAbilitySystemComponent();
	TargetASC->InitAbilityActorInfo(Target, Target);
	struct FCase { bool bBot; float PlayerBase; float EnemyBase; float Expected; };
	const FCase Cases[] = {{false,0,12,12},{true,0,12,12},{false,8,8,20},{true,8,8,8},{true,8,0,0}};
	for (const FCase& Case : Cases)
	{
		AController* Controller = Case.bBot ? static_cast<AController*>(Fixture.Spawn<ADCLyraPlayerBotController>())
			: static_cast<AController*>(Fixture.Spawn<APlayerController>());
		if (!TestNotNull(TEXT("Case controller"), Controller)) { return false; }
		ADCLyraExperiencePlayerState* Source = Fixture.State(*this, Controller);
		if (!Source) { return false; }
		Source->SetGenericTeamId(FGenericTeamId(1));
		TStrongObjectPtr<UDCLyraExperienceDefinition> Data(NewObject<UDCLyraExperienceDefinition>());
		Data->bUseRoleBasedBaseDamage = true;
		Data->PlayerBaseDamage = Case.PlayerBase;
		Data->EnemyBaseDamage = Case.EnemyBase;
		if (!TestTrue(TEXT("Case initializes via production helper"), FDCBaseDamageTestAccess::Initialize(Source, Data.Get()))) { return false; }
		UDCAbilitySystemComponent* SourceASC = Source->GetDCAbilitySystemComponent();
		FGameplayEffectSpecHandle Handle = SourceASC->MakeOutgoingSpec(RifleClass, 1.0f, SourceASC->MakeEffectContext());
		if (!TestTrue(TEXT("Actual Rifle spec"), Handle.IsValid())) { return false; }
		FGameplayEffectSpec& Spec = *Handle.Data.Get();
		Spec.CaptureAttributeDataFromTarget(TargetASC);
		FGameplayEffectCustomExecutionParameters Execution(Spec, Definition.CalculationModifiers, TargetASC, FGameplayTagContainer(), FPredictionKey());
		FGameplayEffectCustomExecutionOutput Output;
		GetDefault<UDCLyraDamageExecution>()->Execute(Execution, Output);
		const TArray<FGameplayModifierEvaluatedData>& Modifiers = Output.GetOutputModifiers();
		if (Case.Expected == 0.0f)
		{
			TestEqual(TEXT("Zero enemy base emits no damage modifier"), Modifiers.Num(), 0);
		}
		else if (TestEqual(TEXT("Actual damage output count"), Modifiers.Num(), 1))
		{
			TestTrue(TEXT("Output targets real Damage attribute"), Modifiers[0].Attribute == UDCHealthSet::GetDamageAttribute());
			TestEqual(FString::Printf(TEXT("%s base %.0f: actual Rifle execution"), Case.bBot ? TEXT("Enemy") : TEXT("Player"), Base(Source)), Modifiers[0].Magnitude, Case.Expected);
		}
	}
	AddInfo(TEXT("Runs the saved GE and original native execution with live ASC captures. No manufactured HitResult, alternate formula, Cue, health application, distance/weakspot or actual shots; those remain PIE checks."));
	return !HasAnyErrors();
}

#endif
