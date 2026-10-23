#include "UI/Stage/DCMissionClearWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/DCStageRunGameMode.h"
#include "HAL/PlatformTime.h"
#include "Input/CommonUIInputTypes.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "NativeGameplayTags.h"
#include "DreamCatcher.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCMissionClearWidget)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DCMissionClearEscape, "UI.Action.Escape");

UDCMissionClearWidget::UDCMissionClearWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
	InputConfig = EDCLyraWidgetInputMode::Menu;
	bIsBackHandler = true;
	bIsModal = true;
	SetIsFocusable(true);
}

void UDCMissionClearWidget::BindStage(ADCStageRunGameMode* Stage)
{
	// CommonUI pools activatable instances on the persistent LocalPlayer layout.
	// A new stage must not inherit the prior instance's Stopped/Failed state.
	StopPresentation();
	Mode = Stage;
	Presentation = EPresentation::Idle;
	bPlaybackFinished = false;
	bHasPresentedVideoFrame = false;
	PhaseStartedAt = LastProgressAt = 0.0;
	LastMediaSeconds = -1.0;
	if (Stage && Stage->GetStagePlayerController()) { SetOwningPlayer(Stage->GetStagePlayerController()); }
	if (Blackout) { Blackout->SetRenderOpacity(0.f); }
	if (ErrorPanel) { ErrorPanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (ErrorText) { ErrorText->SetText(FText::GetEmpty()); }
	if (RetryButton) { RetryButton->SetIsEnabled(true); }
	if (ReturnButton) { ReturnButton->SetIsEnabled(true); }
}

bool UDCMissionClearWidget::IsShowingFailure() const
{
	return Presentation == EPresentation::Failed && ErrorPanel && ErrorPanel->GetVisibility() == ESlateVisibility::Visible;
}

float UDCMissionClearWidget::GetBlackoutOpacity() const { return Blackout ? Blackout->GetRenderOpacity() : 0.f; }

TSharedRef<SWidget> UDCMissionClearWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		Blackout = WidgetTree->ConstructWidget<UImage>();
		Blackout->SetColorAndOpacity(FLinearColor::Black);
		Blackout->SetRenderOpacity(0.f);
		Root->AddChildToOverlay(Blackout);
		UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
		Fit->SetStretch(EStretch::ScaleToFit);
		Root->AddChildToOverlay(Fit);
		MovieImage = WidgetTree->ConstructWidget<UImage>();
		MovieImage->SetVisibility(ESlateVisibility::Collapsed);
		Fit->SetContent(MovieImage);
		ErrorPanel = WidgetTree->ConstructWidget<UVerticalBox>();
		UOverlaySlot* ErrorSlot = Root->AddChildToOverlay(ErrorPanel);
		ErrorSlot->SetHorizontalAlignment(HAlign_Center); ErrorSlot->SetVerticalAlignment(VAlign_Center);
		ErrorText = WidgetTree->ConstructWidget<UTextBlock>();
		ErrorText->SetAutoWrapText(true); ErrorText->SetWrapTextAt(720.f);
		ErrorPanel->AddChildToVerticalBox(ErrorText)->SetPadding(FMargin(20.f));
		RetryButton = WidgetTree->ConstructWidget<UButton>();
		ReturnButton = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* RetryLabel = WidgetTree->ConstructWidget<UTextBlock>();
		RetryLabel->SetText(NSLOCTEXT("DCMissionClear", "Retry", "레벨 다시 시작"));
		RetryButton->SetContent(RetryLabel);
		UTextBlock* ReturnLabel = WidgetTree->ConstructWidget<UTextBlock>();
		ReturnLabel->SetText(NSLOCTEXT("DCMissionClear", "Return", "모드 선택"));
		ReturnButton->SetContent(ReturnLabel);
		ErrorPanel->AddChildToVerticalBox(RetryButton)->SetPadding(FMargin(12.f));
		ErrorPanel->AddChildToVerticalBox(ReturnButton)->SetPadding(FMargin(12.f));
		RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RestartLevel);
		ReturnButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ReturnToModes);
		ErrorPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UDCMissionClearWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_DCMissionClearEscape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::ConsumeEscape)));
}

void UDCMissionClearWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (Presentation != EPresentation::Idle) { return; }
	ADCStageRunGameMode* Stage = Mode.Get();
	if (!Stage || !Stage->BeginMissionClearPresentation(this))
	{
		ShowFailure(NSLOCTEXT("DCMissionClear", "StageUnavailable", "클리어 화면을 시작하지 못했습니다.")); return;
	}
	FadeDuration = Stage->GetMissionClearFadeDuration();
	if (!FMath::IsFinite(FadeDuration) || FadeDuration < 0.f)
	{
		ShowFailure(NSLOCTEXT("DCMissionClear", "InvalidFade", "클리어 화면의 암전 시간 설정을 확인해 주세요.")); return;
	}
	if (APlayerController* PC = GetOwningPlayer()) { PC->bShowMouseCursor = false; }
	Presentation = EPresentation::Fading;
	PhaseStartedAt = FPlatformTime::Seconds();
	UE_LOG(LogDreamCatcher, Log, TEXT("[MissionClear] Fade started: %s"), *GetWorld()->GetMapName());
}

void UDCMissionClearWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	const double Now = FPlatformTime::Seconds();
	if (Presentation == EPresentation::Fading || Presentation == EPresentation::Opening || Presentation == EPresentation::Playing)
	{
		if (APlayerController* PC = GetOwningPlayer()) { PC->bShowMouseCursor = false; }
	}
	if (Presentation == EPresentation::Fading)
	{
		const float Alpha = FadeDuration > 0.f ? FMath::Clamp(float((Now - PhaseStartedAt) / FadeDuration), 0.f, 1.f) : 1.f;
		if (Blackout) { Blackout->SetRenderOpacity(Alpha); }
		if (Alpha >= 1.f) { OpenMovie(); }
	}
	else if (Presentation == EPresentation::Opening)
	{
		if (bMediaOpened) { StartMovieIfReady(); }
		if (Presentation == EPresentation::Opening && Now - PhaseStartedAt > 15.0)
		{
			ShowFailure(NSLOCTEXT("DCMissionClear", "OpenTimeout", "클리어 영상을 여는 시간이 초과되었습니다."));
		}
	}
	else if (Presentation == EPresentation::Playing && Player)
	{
		const double Seconds = Player->GetTime().GetTotalSeconds();
		if (Texture && Texture->CurrentAspectRatio > 0.f && Seconds > 0.0) { bHasPresentedVideoFrame = true; }
		if (Seconds > LastMediaSeconds + .001) { LastMediaSeconds = Seconds; LastProgressAt = Now; }
		if (Now - LastProgressAt > 15.0)
		{
			ShowFailure(NSLOCTEXT("DCMissionClear", "PlaybackStopped", "클리어 영상 재생이 중단되었습니다."));
		}
	}
}

void UDCMissionClearWidget::OpenMovie()
{
	if (Presentation != EPresentation::Fading) { return; }
	UMediaSource* Source = Mode.IsValid() ? Mode->GetMissionClearSource() : nullptr;
	if (!Source || !Source->Validate())
	{
		ShowFailure(NSLOCTEXT("DCMissionClear", "MissingMovie", "클리어 영상 파일을 찾을 수 없습니다.")); return;
	}
	Player = NewObject<UMediaPlayer>(this);
	Player->PlayOnOpen = false; Player->SetLooping(false);
	Player->OnMediaOpened.AddUniqueDynamic(this, &ThisClass::MediaOpened);
	Player->OnMediaOpenFailed.AddUniqueDynamic(this, &ThisClass::MediaOpenFailed);
	Player->OnEndReached.AddUniqueDynamic(this, &ThisClass::MediaEnded);
	Texture = NewObject<UMediaTexture>(this);
	// Slate consumes a regular 2D texture, rather than an external-texture material.
	Texture->NewStyleOutput = true;
	Texture->SetMediaPlayer(Player); Texture->UpdateResource();
	if (APlayerController* PC = GetOwningPlayer())
	{
		Sound = NewObject<UMediaSoundComponent>(PC);
		Sound->bIsUISound = true; Sound->bAllowSpatialization = false;
		Sound->SetMediaPlayer(Player); Sound->RegisterComponentWithWorld(GetWorld()); Sound->Start();
	}
	Presentation = EPresentation::Opening;
	PhaseStartedAt = FPlatformTime::Seconds();
	if (!Player->OpenSource(Source)) { MediaOpenFailed(FString()); }
}

void UDCMissionClearWidget::MediaOpened(FString)
{
	if (Presentation != EPresentation::Opening || !Player) { return; }
	bMediaOpened = true;
	StartMovieIfReady();
}

void UDCMissionClearWidget::StartMovieIfReady()
{
	if (Presentation != EPresentation::Opening || !bMediaOpened || !Player) { return; }
	const FIntPoint Size = Player->GetVideoTrackDimensions(INDEX_NONE, INDEX_NONE);
	// Some backends announce the open before their selected track is ready.
	if (Size.X <= 0 || Size.Y <= 0) { return; }
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture); Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(Size.X, Size.Y);
	MovieImage->SetBrush(Brush); MovieImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	Presentation = EPresentation::Playing;
	LastProgressAt = FPlatformTime::Seconds(); LastMediaSeconds = -1.0;
	if (!Player->Play()) { ShowFailure(NSLOCTEXT("DCMissionClear", "PlayFailed", "클리어 영상을 재생하지 못했습니다.")); return; }
	UE_LOG(LogDreamCatcher, Log, TEXT("[MissionClear] Movie playing: %dx%d"), Size.X, Size.Y);
}

void UDCMissionClearWidget::MediaOpenFailed(FString)
{
	ShowFailure(NSLOCTEXT("DCMissionClear", "OpenFailed", "클리어 영상을 열지 못했습니다."));
}

void UDCMissionClearWidget::MediaEnded()
{
	if (Presentation != EPresentation::Playing || !IsActivated() || !Mode.IsValid()) { return; }
	// WMF can report end-of-stream for an unsupported H.264 profile without
	// decoding a single frame. That is a playback failure, never a completed movie.
	if (!bHasPresentedVideoFrame)
	{
		ShowFailure(NSLOCTEXT("DCMissionClear", "NoDecodedFrames", "클리어 영상의 화면을 재생하지 못했습니다. 영상 형식을 확인하거나 다시 시도해 주세요."));
		return;
	}
	Presentation = EPresentation::Finished;
	bPlaybackFinished = true;
	UE_LOG(LogDreamCatcher, Log, TEXT("[MissionClear] Movie ended; requesting configured next screen."));
	StopMedia();
	if (!Mode->RequestMissionClearTravel(this))
	{
		ShowFailure(NSLOCTEXT("DCMissionClear", "DestinationUnavailable", "이동할 다음 화면을 확인할 수 없습니다."));
	}
}

void UDCMissionClearWidget::StopMedia()
{
	bMediaOpened = false;
	if (MovieImage) { MovieImage->SetVisibility(ESlateVisibility::Collapsed); MovieImage->SetBrush(FSlateBrush()); }
	if (Sound) { Sound->Stop(); Sound->SetMediaPlayer(nullptr); Sound->DestroyComponent(); Sound = nullptr; }
	if (Player)
	{
		Player->OnMediaOpened.RemoveDynamic(this, &ThisClass::MediaOpened);
		Player->OnMediaOpenFailed.RemoveDynamic(this, &ThisClass::MediaOpenFailed);
		Player->OnEndReached.RemoveDynamic(this, &ThisClass::MediaEnded);
		Player->Close(); Player = nullptr;
	}
	if (Texture) { Texture->SetMediaPlayer(nullptr); Texture = nullptr; }
}

void UDCMissionClearWidget::ShowFailure(const FText& Message)
{
	if (Presentation == EPresentation::Stopped) { return; }
	Presentation = EPresentation::Failed;
	bPlaybackFinished = false;
	StopMedia();
	if (Mode.IsValid()) { Mode->ReportMissionClearFailure(this); }
	if (Blackout) { Blackout->SetRenderOpacity(1.f); }
	if (ErrorText) { ErrorText->SetText(Message); }
	if (ErrorPanel) { ErrorPanel->SetVisibility(ESlateVisibility::Visible); }
	if (RetryButton) { RetryButton->SetIsEnabled(true); RetryButton->SetFocus(); }
	if (ReturnButton) { ReturnButton->SetIsEnabled(true); }
	UE_LOG(LogDreamCatcher, Warning, TEXT("[MissionClear] %s"), *Message.ToString());
}

void UDCMissionClearWidget::RestartLevel()
{
	if (Presentation == EPresentation::Failed && Mode.IsValid() && Mode->RequestRestartStage())
	{
		RetryButton->SetIsEnabled(false); ReturnButton->SetIsEnabled(false);
	}
}

void UDCMissionClearWidget::ReturnToModes()
{
	if (Presentation == EPresentation::Failed && Mode.IsValid() && Mode->RequestExitStage())
	{
		RetryButton->SetIsEnabled(false); ReturnButton->SetIsEnabled(false);
	}
}

UWidget* UDCMissionClearWidget::NativeGetDesiredFocusTarget() const
{
	return Presentation == EPresentation::Failed ? static_cast<UWidget*>(RetryButton.Get()) : const_cast<UDCMissionClearWidget*>(this);
}

void UDCMissionClearWidget::StopPresentation()
{
	Presentation = EPresentation::Stopped;
	bPlaybackFinished = false;
	StopMedia();
}

void UDCMissionClearWidget::NativeOnDeactivated() { StopPresentation(); Super::NativeOnDeactivated(); }
void UDCMissionClearWidget::NativeDestruct() { StopPresentation(); Mode.Reset(); Super::NativeDestruct(); }
