#include "Tests/DCFrontendTestTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCFrontendTestTypes)

void ADCFrontendModeFixture::InitGame(const FString& Map, const FString& Options, FString& Error)
{
	AGameModeBase::InitGame(Map, Options, Error);
}
void ADCFrontendModeFixture::InitGameState() { AGameModeBase::InitGameState(); }
void ADCFrontendModeFixture::SetStoryDestination(const FString& Path) { StoryMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(Path)); }
void ADCFrontendModeFixture::SimulateTravelFailure() { HandleTravelFailure(GetWorld(), ETravelFailure::LoadMapFailure, TEXT("Fixture failure")); }
void ADCFrontendModeFixture::PerformStoryTravel(FName Destination) { ++TravelCalls; LastDestination = Destination; }

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE
#include "CommonPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFrontendTravelTest, "DreamCatcher.Frontend.DeferredTravelAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFrontendTravelTest::RunTest(const FString&)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCFrontendIsolatedAutomation")))
	{
		AddError(TEXT("Run in a separate unattended editor with -DCFrontendIsolatedAutomation -DCLyraRuntimeDiagnostics.")); return false;
	}
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	Fixture.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCFrontendModeFixture::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	auto* Mode = Fixture.GetTestWorld()->GetAuthGameMode<ADCFrontendModeFixture>();
	if (!TestNotNull(TEXT("Menu mode"), Mode)) { return false; }
	TestFalse(TEXT("Unset destination does not begin travel"), Mode->RequestStartStory());
	Mode->SetStoryDestination(TEXT("/Game/DreamCatcher/Maps/Missing_Frontend_Test.Missing_Frontend_Test"));
	TestFalse(TEXT("Missing map does not disable menu"), Mode->RequestStartStory());
	TestFalse(TEXT("Rejected request does not latch"), Mode->IsStoryTravelPending());
	Mode->SetStoryDestination(TEXT("/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.Level_1"));
	TestTrue(TEXT("Existing story map accepted without loading its world"), Mode->RequestStartStory());
	TestFalse(TEXT("Double click rejected"), Mode->RequestStartStory());
	TestEqual(TEXT("UI callback never changes the world synchronously"), Mode->TravelCalls, 0);
	Fixture.TickTestWorld(.02f); Fixture.TickTestWorld(.02f);
	TestEqual(TEXT("Exactly one travel request"), Mode->TravelCalls, 1);
	TestEqual(TEXT("Full production package destination"), Mode->LastDestination,
		FName(TEXT("/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1")));
	int32 FailureNotifications = 0;
	Mode->OnStoryTravelFailed.AddLambda([&FailureNotifications](const FText&) { ++FailureNotifications; });
	Mode->SimulateTravelFailure();
	Mode->SimulateTravelFailure();
	TestFalse(TEXT("Failure releases input latch"), Mode->IsStoryTravelPending());
	TestEqual(TEXT("One failure notification"), FailureNotifications, 1);
	TestTrue(TEXT("Retry accepted"), Mode->RequestStartStory());
	Mode->SimulateTravelFailure();
	Fixture.TickTestWorld(.02f); Fixture.TickTestWorld(.02f);
	TestEqual(TEXT("Canceled queued request does not open a map"), Mode->TravelCalls, 1);
	Mode->OnStoryTravelFailed.Clear();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFrontendIsolationTest, "DreamCatcher.Frontend.NoCombatPawnDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFrontendIsolationTest::RunTest(const FString&)
{
	ADCFrontendGameMode* Mode = GetMutableDefault<ADCFrontendGameMode>();
	TestFalse(TEXT("Frontend rejects the gameplay restart path"), Mode->ControllerCanRestart(nullptr));
	TestNull(TEXT("Frontend never falls back to AssetManager combat PawnData"), Mode->GetDefaultPawnClassForController_Implementation(nullptr));
	TestNull(TEXT("No spectator pawn"), Mode->SpectatorClass.Get());
	TestEqual(TEXT("CommonUI controller without loadout/stage components"), Mode->PlayerControllerClass.Get(), ACommonPlayerController::StaticClass());
	TestEqual(TEXT("PlayerState without Experience combat grant callback"), Mode->PlayerStateClass.Get(), APlayerState::StaticClass());
	return !HasAnyErrors();
}
#endif
