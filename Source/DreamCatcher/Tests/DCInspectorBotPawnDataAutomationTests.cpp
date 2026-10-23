// Unsaved tests of the real GameMode selection policy. No saved Experience,
// fabricated Experience-loaded state, Inspector assets, or PIE are involved.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && WITH_SERVER_CODE

#include "CoreMinimal.h"
#include "AIController.h"
#include "Character/DCPawnData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "Tests/DCLyraAIFoundationTestTypes.h"
#include "UObject/StrongObjectPtr.h"

struct FDCInspectorBotPawnDataTestAccess
{
	static const UDCPawnData* Select(const AController* Controller,
		TFunctionRef<const UDCLyraExperienceDefinition*()> LoadedExperience,
		TFunctionRef<const UDCPawnData*()> Fallback)
	{
		return ADCLyraGameMode::SelectPawnDataForController(Controller, LoadedExperience, Fallback);
	}
};

namespace DCInspectorBotPawnDataTests
{
bool Authorized(FAutomationTestBase& Test)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCInspectorBotPawnDataTests")))
	{
		Test.AddError(TEXT("Use a separate unattended Editor with -DCInspectorBotPawnDataTests; no assets are saved."));
		return false;
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCInspectorBotPawnSelectionTest,
	"DreamCatcher.Editor.InspectorBotPawnData.SelectionPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCInspectorBotPawnSelectionTest::RunTest(const FString&)
{
	if (!DCInspectorBotPawnDataTests::Authorized(*this)) { return false; }
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	// Existing fixture has the real GameState/Experience component, but never
	// selects or loads a saved Experience. Its PlayerStates are the native base.
	World->GetWorldSettings()->DefaultGameMode = ADCR10AssemblyGameModeFixture::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	ADCLyraGameMode* GameMode = World->GetAuthGameMode<ADCLyraGameMode>();
	ADCR10BotControllerFixture* Bot = World->SpawnActor<ADCR10BotControllerFixture>();
	ADCR10TeamPlayerControllerFixture* Human = World->SpawnActor<ADCR10TeamPlayerControllerFixture>();
	AAIController* OtherAI = World->SpawnActor<AAIController>();
	if (!TestNotNull(TEXT("Existing GameMode fixture"), GameMode)
		|| !TestNotNull(TEXT("Existing Lyra-bot subclass fixture"), Bot)
		|| !TestNotNull(TEXT("Existing human-controller fixture"), Human)
		|| !TestNotNull(TEXT("Ordinary AI controller"), OtherAI)) { return false; }

	TStrongObjectPtr<UDCLyraExperienceDefinition> Experience(NewObject<UDCLyraExperienceDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> DefaultData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> BotData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> FallbackData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> ExplicitData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));
	DefaultData->PawnClass = APawn::StaticClass();
	BotData->PawnClass = ACharacter::StaticClass();
	FallbackData->PawnClass = APawn::StaticClass();
	ExplicitData->PawnClass = ACharacter::StaticClass();
	Experience->DefaultPawnData = DefaultData.Get();
	Experience->BotPawnData = BotData.Get();
	int32 ExperienceQueries = 0;
	int32 FallbackQueries = 0;
	auto Select = [&](const AController* Controller, const UDCLyraExperienceDefinition* Loaded, const UDCPawnData* Fallback)
	{
		ExperienceQueries = 0;
		FallbackQueries = 0;
		return FDCInspectorBotPawnDataTestAccess::Select(Controller,
			[&]() -> const UDCLyraExperienceDefinition* { ++ExperienceQueries; return Loaded; },
			[&]() -> const UDCPawnData* { ++FallbackQueries; return Fallback; });
	};
	TestNull(TEXT("Public resolver preserves unloaded Experience for bots"), GameMode->GetPawnDataForController(Bot));
	TestNull(TEXT("Public resolver preserves unloaded Experience for humans"), GameMode->GetPawnDataForController(Human));
	TestTrue(TEXT("Lyra bot subclass selects the optional bot data"), Select(Bot, Experience.Get(), FallbackData.Get()) == BotData.Get());
	TestEqual(TEXT("Selecting bot data does not query the global fallback"), FallbackQueries, 0);
	TestTrue(TEXT("Human keeps the Experience default"), Select(Human, Experience.Get(), FallbackData.Get()) == DefaultData.Get());
	TestTrue(TEXT("Other AI keeps the Experience default"), Select(OtherAI, Experience.Get(), FallbackData.Get()) == DefaultData.Get());
	TestTrue(TEXT("Null controller keeps the Experience default"), Select(nullptr, Experience.Get(), FallbackData.Get()) == DefaultData.Get());
	TestTrue(TEXT("Bot selection preserves the human default reference"), Experience->DefaultPawnData == DefaultData.Get());
	Experience->BotPawnData = nullptr;
	TestTrue(TEXT("Unconfigured bot override preserves the old default path"), Select(Bot, Experience.Get(), FallbackData.Get()) == DefaultData.Get());
	Experience->BotPawnData = BotData.Get();
	Experience->DefaultPawnData = nullptr;
	TestTrue(TEXT("Bot override does not require a shared default"), Select(Bot, Experience.Get(), FallbackData.Get()) == BotData.Get());
	TestTrue(TEXT("Human uses the original AssetManager-provider fallback"), Select(Human, Experience.Get(), FallbackData.Get()) == FallbackData.Get());
	TestEqual(TEXT("Fallback is queried once when actually needed"), FallbackQueries, 1);
	TestTrue(TEXT("Other AI uses the original fallback"), Select(OtherAI, Experience.Get(), FallbackData.Get()) == FallbackData.Get());
	TestTrue(TEXT("Null controller uses the original fallback"), Select(nullptr, Experience.Get(), FallbackData.Get()) == FallbackData.Get());
	Experience->BotPawnData = nullptr;
	TestTrue(TEXT("Unconfigured bot uses the original fallback"), Select(Bot, Experience.Get(), FallbackData.Get()) == FallbackData.Get());
	TestNull(TEXT("Absent global default stays null"), Select(Bot, Experience.Get(), nullptr));
	TestNull(TEXT("Unloaded Experience never uses an available fallback"), Select(Bot, nullptr, FallbackData.Get()));
	TestEqual(TEXT("Unloaded Experience does not load the global fallback"), FallbackQueries, 0);
	Experience->DefaultPawnData = DefaultData.Get();
	Experience->BotPawnData = BotData.Get();

	ADCPlayerState* BotState = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* HumanState = Human->GetPlayerState<ADCPlayerState>();
	if (!HumanState) { HumanState = World->SpawnActor<ADCPlayerState>(); }
	if (!TestNotNull(TEXT("Transient bot PlayerState"), BotState) || !TestNotNull(TEXT("Transient human PlayerState"), HumanState)) { return false; }
	BotState->SetOwner(Bot);
	Bot->AssignFixturePlayerState(BotState);
	HumanState->SetOwner(Human);
	Human->AssignFixturePlayerState(HumanState);
	BotState->SetPawnData(ExplicitData.Get());
	HumanState->SetPawnData(ExplicitData.Get());
	TestTrue(TEXT("Explicit bot PlayerState data beats the bot override"), Select(Bot, Experience.Get(), FallbackData.Get()) == ExplicitData.Get());
	TestEqual(TEXT("Explicit bot data skips Experience/GameState lookup"), ExperienceQueries, 0);
	TestEqual(TEXT("Explicit bot data skips global fallback lookup"), FallbackQueries, 0);
	TestTrue(TEXT("Explicit human data works even before Experience readiness"), Select(Human, nullptr, FallbackData.Get()) == ExplicitData.Get());
	TestEqual(TEXT("Explicit human data skips Experience/GameState lookup"), ExperienceQueries, 0);
	TestEqual(TEXT("Explicit human data skips global fallback lookup"), FallbackQueries, 0);
	TestTrue(TEXT("Public resolver uses explicit bot PlayerState data"), GameMode->GetPawnDataForController(Bot) == ExplicitData.Get());
	TestTrue(TEXT("Public resolver uses explicit human PlayerState data"), GameMode->GetPawnDataForController(Human) == ExplicitData.Get());
	TestTrue(TEXT("Selection never rewrites immutable PlayerState data"), BotState->GetPawnData<UDCPawnData>() == ExplicitData.Get()
		&& HumanState->GetPawnData<UDCPawnData>() == ExplicitData.Get());
	AddInfo(TEXT("Shared production policy and real transient controllers/PlayerStates only. Saved Experience loading, Inspector mesh, spawn/animation, replication and PIE remain separate checks."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCInspectorBotPawnValidationTest,
	"DreamCatcher.Editor.InspectorBotPawnData.DataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCInspectorBotPawnValidationTest::RunTest(const FString&)
{
	if (!DCInspectorBotPawnDataTests::Authorized(*this)) { return false; }
	TStrongObjectPtr<UDCLyraExperienceDefinition> Experience(NewObject<UDCLyraExperienceDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> BotData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));
	TestNull(TEXT("Existing Experiences default to no bot override"), Experience->BotPawnData.Get());
	FDataValidationContext Unset;
	TestTrue(TEXT("An unset bot override preserves existing validation"), Experience->IsDataValid(Unset) == EDataValidationResult::Valid);
	Experience->BotPawnData = BotData.Get();
	FDataValidationContext MissingClass;
	TestTrue(TEXT("Configured bot data requires a PawnClass"), Experience->IsDataValid(MissingClass) == EDataValidationResult::Invalid);
	TestEqual(TEXT("Missing bot class produces one authoring error"), MissingClass.GetNumErrors(), uint32(1));
	BotData->PawnClass = APawn::StaticClass();
	FDataValidationContext ConcreteClass;
	TestTrue(TEXT("A concrete APawn class is valid"), Experience->IsDataValid(ConcreteClass) == EDataValidationResult::Valid);
	// TSubclassOf also rejects invalid class metadata on Get(); validate the
	// incoming data without changing any existing class or production object.
	BotData->PawnClass = UObject::StaticClass();
	FDataValidationContext NonPawnClass;
	TestTrue(TEXT("Non-Pawn class metadata is rejected"), Experience->IsDataValid(NonPawnClass) == EDataValidationResult::Invalid);
	TStrongObjectPtr<UClass> AbstractPawnClass(NewObject<UClass>(GetTransientPackage(), NAME_None, RF_Transient));
	AbstractPawnClass->SetSuperStruct(APawn::StaticClass());
	AbstractPawnClass->ClassFlags |= CLASS_Abstract;
	if (!TestTrue(TEXT("Unsaved abstract metadata has Pawn ancestry"), AbstractPawnClass->IsChildOf(APawn::StaticClass()))) { return false; }
	BotData->PawnClass = AbstractPawnClass.Get();
	FDataValidationContext AbstractClass;
	TestTrue(TEXT("Abstract Pawn class metadata is rejected"), Experience->IsDataValid(AbstractClass) == EDataValidationResult::Invalid);
	Experience->BotPawnData = nullptr;
	FDataValidationContext Disabled;
	TestTrue(TEXT("Unused invalid bot data does not invalidate another Experience"), Experience->IsDataValid(Disabled) == EDataValidationResult::Valid);
	AddInfo(TEXT("Validation uses transient data/class metadata only; no Blueprint compilation, saved data or existing class flags are changed."));
	return !HasAnyErrors();
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && WITH_SERVER_CODE
