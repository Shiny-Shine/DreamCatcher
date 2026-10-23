#include "Tests/DCStageUITestTypes.h"
#include "UI/Stage/DCStageUIComponent.h"
#include "UI/Stage/DCMissionClearWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCStageUITestTypes)

ADCR13StageModeFixture::ADCR13StageModeFixture()
{
	ReturnMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.Level_1")));
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "UObject/StrongObjectPtr.h"

struct FDCStageUITestAccess
{
	static void Block(UDCStageUIComponent* UI, bool bBlock) { UI->SetMenuInputBlocked(bBlock); }
	static UMediaPlayer* SeedMovie(UDCMissionClearWidget* Widget)
	{
		Widget->Player = NewObject<UMediaPlayer>(Widget);
		Widget->Texture = NewObject<UMediaTexture>(Widget);
		Widget->Texture->SetMediaPlayer(Widget->Player);
		Widget->Player->OnEndReached.AddUniqueDynamic(Widget, &UDCMissionClearWidget::MediaEnded);
		Widget->Player->OnMediaOpened.AddUniqueDynamic(Widget, &UDCMissionClearWidget::MediaOpened);
		Widget->Player->OnMediaOpenFailed.AddUniqueDynamic(Widget, &UDCMissionClearWidget::MediaOpenFailed);
		Widget->Presentation = UDCMissionClearWidget::EPresentation::Playing;
		return Widget->Player;
	}
	static bool Released(const UDCMissionClearWidget* Widget) { return !Widget->Player && !Widget->Texture && !Widget->Sound; }
	static void SeedCompletedMovie(UDCMissionClearWidget* Widget)
	{
		Widget->Presentation = UDCMissionClearWidget::EPresentation::Stopped;
		Widget->bPlaybackFinished = true;
		Widget->bHasPresentedVideoFrame = true;
		Widget->LastMediaSeconds = 6.9;
	}
	static bool ReadyForAnotherStage(const UDCMissionClearWidget* Widget, const ADCStageRunGameMode* Mode)
	{
		return Widget->Presentation == UDCMissionClearWidget::EPresentation::Idle && Widget->Mode.Get() == Mode
			&& !Widget->bPlaybackFinished && !Widget->bHasPresentedVideoFrame && !Widget->bMediaOpened
			&& Widget->LastMediaSeconds == -1.0 && Released(Widget);
	}
};

namespace DCStageUITests
{
struct FFixture
{
	FTestWorldWrapper World;
	ADCR13StageModeFixture* Mode = nullptr;
	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR13UIIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate unattended editor with -DCR13UIIsolatedAutomation -DCLyraRuntimeDiagnostics.")); return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR13StageModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Mode = World.GetTestWorld()->GetAuthGameMode<ADCR13StageModeFixture>();
		return Test.TestNotNull(TEXT("Mode"), Mode);
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageUIMenuTravelTest, "DreamCatcher.R13.UI.MenuReturnIsOptIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageUIMenuTravelTest::RunTest(const FString&)
{
	DCStageUITests::FFixture F; if (!F.Start(*this)) { return false; }
	TestFalse(TEXT("Existing R11/R12 default remains disabled"), F.Mode->UsesExperienceUI());
	TestFalse(TEXT("Old run cannot be aborted by the new menu API"), F.Mode->RequestExitStageFromMenu());
	TestFalse(TEXT("Rejected request did not end the run"), F.Mode->HasRunEnded());
	F.Mode->UseExperienceUI(true);
	TestTrue(TEXT("Opt-in menu may abort and return"), F.Mode->RequestExitStageFromMenu());
	TestTrue(TEXT("Run ends and travel is latched"), F.Mode->HasRunEnded() && F.Mode->IsStageTravelPending());
	TestFalse(TEXT("Exit is not victory"), F.Mode->WasRunSuccessful());
	TestFalse(TEXT("Double request rejected"), F.Mode->RequestExitStageFromMenu());
	TestEqual(TEXT("No immediate travel inside UI callback"), F.Mode->TravelCalls, 0);
	F.World.TickTestWorld(.02f); F.World.TickTestWorld(.02f);
	TestEqual(TEXT("One queued request"), F.Mode->TravelCalls, 1);
	TestEqual(TEXT("Existing return destination"), F.Mode->LastTravel,
		FName(TEXT("/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageUIInputLifetimeTest, "DreamCatcher.R13.UI.MenuInputOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageUIInputLifetimeTest::RunTest(const FString&)
{
	DCStageUITests::FFixture F; if (!F.Start(*this)) { return false; }
	APlayerController* PC = F.World.GetTestWorld()->SpawnActor<ADCR10TeamPlayerControllerFixture>();
	if (!TestNotNull(TEXT("PC"), PC)) { return false; }
	F.Mode->GenericPlayerInitialization(PC);
	ADCPlayerState* PS = PC->GetPlayerState<ADCPlayerState>();
	if (!TestNotNull(TEXT("PlayerState"), PS)) { return false; }
	UDCAbilitySystemComponent* ASC = PS->GetDCAbilitySystemComponent();
	UDCStageUIComponent* UI = NewObject<UDCStageUIComponent>(PC);
	PC->AddInstanceComponent(UI); UI->RegisterComponent();
	ASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
	FDCStageUITestAccess::Block(UI, true); FDCStageUITestAccess::Block(UI, true);
	TestEqual(TEXT("One UI contribution despite nested notifications"), ASC->GetTagCount(DCGameplayTags::Gameplay_AbilityInputBlocked), 2);
	FDCStageUITestAccess::Block(UI, false); FDCStageUITestAccess::Block(UI, false);
	TestEqual(TEXT("External tag retained"), ASC->GetTagCount(DCGameplayTags::Gameplay_AbilityInputBlocked), 1);
	TestTrue(TEXT("External move/look blocks retained"), PC->IsMoveInputIgnored() && PC->IsLookInputIgnored());
	FDCStageUITestAccess::Block(UI, true); UI->DestroyComponent();
	TestEqual(TEXT("Component teardown releases exactly its own tag"), ASC->GetTagCount(DCGameplayTags::Gameplay_AbilityInputBlocked), 1);
	PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false);
	ASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	TestFalse(TEXT("No residual UI move ignore"), PC->IsMoveInputIgnored());
	TestFalse(TEXT("No residual UI look ignore"), PC->IsLookInputIgnored());
	AddInfo(TEXT("Native ownership/request contracts only; real CommonUI menus/settings/focus require the R13 asset PIE."));
	return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageUIMovieLifetimeTest, "DreamCatcher.R13.UI.MissionClearMediaLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageUIMovieLifetimeTest::RunTest(const FString&)
{
	DCStageUITests::FFixture F; if (!F.Start(*this)) { return false; }
	TStrongObjectPtr<UDCMissionClearWidget> Movie { NewObject<UDCMissionClearWidget>(F.World.GetTestWorld()) };
	TStrongObjectPtr<UMediaPlayer> Player { FDCStageUITestAccess::SeedMovie(Movie.Get()) };
	TestTrue(TEXT("Media end delegate initially owned"), Player->OnEndReached.IsBound());
	Movie->StopPresentation(); Movie->StopPresentation();
	TestFalse(TEXT("Teardown is never movie completion"), Movie->HasPlaybackFinished());
	TestFalse(TEXT("End callback removed"), Player->OnEndReached.IsBound());
	TestFalse(TEXT("Open callback removed"), Player->OnMediaOpened.IsBound());
	TestFalse(TEXT("Failure callback removed"), Player->OnMediaOpenFailed.IsBound());
	TestTrue(TEXT("Texture/player/audio ownership released idempotently"), FDCStageUITestAccess::Released(Movie.Get()));
	FDCStageUITestAccess::SeedCompletedMovie(Movie.Get());
	Movie->BindStage(F.Mode);
	TestTrue(TEXT("Pooled instance is reset for the next stage, including playback authorization"),
		FDCStageUITestAccess::ReadyForAnotherStage(Movie.Get(), F.Mode));
	return !HasAnyErrors();
}
#endif
