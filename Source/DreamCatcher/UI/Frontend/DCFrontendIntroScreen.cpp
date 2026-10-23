#include "UI/Frontend/DCFrontendIntroScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "DreamCatcher.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Widgets/SWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCFrontendIntroScreen)

/** Runs before CommonUI so confirm/back cannot also operate the newly shown menu.
 * After the screen closes, only the skipping key is retained until release.
 */
class FDCFrontendIntroInputProcessor final : public IInputProcessor, public TSharedFromThis<FDCFrontendIntroInputProcessor>
{
public:
	explicit FDCFrontendIntroInputProcessor(UDCFrontendIntroScreen* Screen) : Owner(Screen)
	{
		if (const TSharedPtr<SWidget> Widget = Screen->GetCachedWidget())
		{
			Window = FSlateApplication::Get().FindWidgetWindow(Widget.ToSharedRef());
			bHadWindow = Window.IsValid();
		}
	}
	void Capture(FKey Key) { ConsumedKeys.Add(Key); }
	void Detach() { Owner.Reset(); bDetached = true; }
	virtual const TCHAR* GetDebugName() const override { return TEXT("DCFrontendIntro"); }
	virtual void Tick(float, FSlateApplication& SlateApp, TSharedRef<ICursor>) override
	{
		if (!Owner.IsValid()) { bDetached = true; }
		const TSharedPtr<SWindow> CurrentWindow = Window.Pin();
		const bool bLostWindow = bHadWindow && (!CurrentWindow || !CurrentWindow->IsActive());
		if (bDetached && (ConsumedKeys.IsEmpty() || bLostWindow))
		{
			const TSharedRef<FDCFrontendIntroInputProcessor> KeepAlive = AsShared();
			SlateApp.UnregisterInputPreProcessor(KeepAlive);
		}
	}
	virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
	{
		if (ConsumedKeys.Contains(Event.GetKey())) { return true; }
		if (UDCFrontendIntroScreen* Screen = Owner.Get(); Screen && Screen->OwnsKeyboardFocus())
		{
			// A key held while entering this map must not skip its fresh presentation.
			if (!Event.IsRepeat()) { Screen->SkipFromInput(Event.GetKey()); }
			return true;
		}
		return false;
	}
	virtual bool HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& Event) override
	{
		return ConsumedKeys.Remove(Event.GetKey()) > 0;
	}
	virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent& Event) override
	{
		if (ConsumedKeys.Contains(Event.GetEffectingButton())) { return true; }
		if (UDCFrontendIntroScreen* Screen = Owner.Get(); Screen && Screen->ContainsPointer(Event))
		{
			Screen->SkipFromInput(Event.GetEffectingButton());
			return true;
		}
		return false;
	}
	virtual bool HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent& Event) override
	{
		return ConsumedKeys.Remove(Event.GetEffectingButton()) > 0;
	}
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		return HandleMouseButtonDownEvent(SlateApp, Event);
	}
private:
	TWeakObjectPtr<UDCFrontendIntroScreen> Owner;
	TWeakPtr<SWindow> Window;
	TSet<FKey> ConsumedKeys;
	bool bDetached = false;
	bool bHadWindow = false;
};

UDCFrontendIntroScreen::UDCFrontendIntroScreen(const FObjectInitializer& Initializer) : Super(Initializer)
{
	InputConfig = EDCLyraWidgetInputMode::Menu;
	bIsBackHandler = true;
	bIsModal = true;
	SetIsFocusable(true);
}

TSharedRef<SWidget> UDCFrontendIntroScreen::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		UImage* Background = WidgetTree->ConstructWidget<UImage>();
		Background->SetColorAndOpacity(FLinearColor::Black);
		Root->AddChildToOverlay(Background);
		UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
		Fit->SetStretch(EStretch::ScaleToFit);
		UOverlaySlot* FitSlot = Root->AddChildToOverlay(Fit);
		FitSlot->SetHorizontalAlignment(HAlign_Fill);
		FitSlot->SetVerticalAlignment(VAlign_Fill);
		MovieImage = WidgetTree->ConstructWidget<UImage>();
		MovieImage->SetVisibility(ESlateVisibility::Collapsed);
		Fit->SetContent(MovieImage);
	}
	return Super::RebuildWidget();
}

void UDCFrontendIntroScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	// CommonUI pools widget instances on the persistent LocalPlayer layout.
	// Reset media on every activation; never keep a session-wide "already shown" flag.
	StopMedia();
	ReleaseInputProcessor();
	bIntroActive = true;
	bStartedPlaying = false;
	bOpenRequested = false;
	bHasPresentedFrame = false;
	LastMediaSeconds = -1.0;
	OpenStartedAt = LastProgressAt = FPlatformTime::Seconds();
	if (APlayerController* PC = GetOwningPlayer())
	{
		CursorOwner = PC;
		bSavedCursorVisibility = PC->bShowMouseCursor;
		bCursorWasSaved = true;
		PC->bShowMouseCursor = false;
	}
	InstallInputProcessor();
	SetFocus();
	// Wait until the stack's AfterPush callback; an invalid source must not
	// deactivate this widget in the middle of CommonUI adding it to the stack.
}

void UDCFrontendIntroScreen::InstallInputProcessor()
{
	if (FSlateApplication::IsInitialized() && !InputProcessor)
	{
		InputProcessor = MakeShared<FDCFrontendIntroInputProcessor>(this);
		// CommonInput can track the device first; CommonUI's analog cursor is Game/2.
		FInputPreprocessorRegistrationKey Registration;
		Registration.Type = EInputPreProcessorType::Game;
		Registration.Priority = 1;
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, Registration);
	}
}

void UDCFrontendIntroScreen::ReleaseInputProcessor()
{
	if (InputProcessor)
	{
		// Slate holds it only until the consumed key-up (or focus loss).
		StaticCastSharedPtr<FDCFrontendIntroInputProcessor>(InputProcessor)->Detach();
		InputProcessor.Reset();
	}
}

bool UDCFrontendIntroScreen::OwnsKeyboardFocus() const
{
	APlayerController* PC = GetOwningPlayer();
	return bIntroActive && IsActivated() && PC && (HasUserFocus(PC) || HasUserFocusedDescendants(PC));
}

bool UDCFrontendIntroScreen::ContainsPointer(const FPointerEvent& Event) const
{
	return bIntroActive && IsActivated() && GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition());
}

void UDCFrontendIntroScreen::SkipFromInput(FKey Key)
{
	if (!bIntroActive) { return; }
	if (InputProcessor) { StaticCastSharedPtr<FDCFrontendIntroInputProcessor>(InputProcessor)->Capture(Key); }
	SkipIntro();
}

void UDCFrontendIntroScreen::SkipIntro() { FinishIntro(); }
bool UDCFrontendIntroScreen::NativeOnHandleBackAction() { SkipFromInput(EKeys::Escape); return true; }
UWidget* UDCFrontendIntroScreen::NativeGetDesiredFocusTarget() const { return const_cast<UDCFrontendIntroScreen*>(this); }

void UDCFrontendIntroScreen::OpenIntro()
{
	if (!IntroMediaSource || !IntroMediaSource->Validate()) { FailIntro(TEXT("Missing PV media source/file")); return; }
	if (!FMath::IsFinite(MediaTimeoutSeconds) || MediaTimeoutSeconds < 1.f) { FailIntro(TEXT("Invalid media timeout")); return; }
	Player = NewObject<UMediaPlayer>(this);
	Player->PlayOnOpen = false;
	Player->SetLooping(true);
	Player->OnMediaOpened.AddUniqueDynamic(this, &ThisClass::MediaOpened);
	Player->OnMediaOpenFailed.AddUniqueDynamic(this, &ThisClass::MediaOpenFailed);
	Player->OnEndReached.AddUniqueDynamic(this, &ThisClass::MediaEnded);
	Texture = NewObject<UMediaTexture>(this);
	Texture->NewStyleOutput = true;
	Texture->SetMediaPlayer(Player);
	Texture->UpdateResource();
	if (APlayerController* PC = GetOwningPlayer())
	{
		Sound = NewObject<UMediaSoundComponent>(PC);
		Sound->bIsUISound = true;
		Sound->bAllowSpatialization = false;
		Sound->SetMediaPlayer(Player);
		Sound->RegisterComponentWithWorld(GetWorld());
		Sound->Start();
		Sound->SetVolumeMultiplier(0.25f); // 기존 볼륨의 25%
	}
	UE_LOG(LogDreamCatcher, Log, TEXT("[FrontendIntro] Opening PV: %s"), *GetWorld()->GetMapName());
	if (!Player->OpenSource(IntroMediaSource)) { FailIntro(TEXT("OpenSource rejected PV")); }
}

void UDCFrontendIntroScreen::MediaOpened(FString)
{
	if (!bIntroActive) { return; }
	bMediaOpened = true;
	StartMovieIfReady();
}

void UDCFrontendIntroScreen::StartMovieIfReady()
{
	if (!bIntroActive || !bMediaOpened || bStartedPlaying || !Player) { return; }
	const FIntPoint Size = Player->GetVideoTrackDimensions(INDEX_NONE, INDEX_NONE);
	if (Size.X <= 0 || Size.Y <= 0) { return; }
	if (!Player->SetLooping(true)) { FailIntro(TEXT("PV backend does not support looping")); return; }
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(Size.X, Size.Y);
	if (MovieImage) { MovieImage->SetBrush(Brush); MovieImage->SetVisibility(ESlateVisibility::HitTestInvisible); }
	bStartedPlaying = true;
	LastProgressAt = FPlatformTime::Seconds();
	if (!Player->Play()) { FailIntro(TEXT("PV playback could not start")); return; }
	UE_LOG(LogDreamCatcher, Log, TEXT("[FrontendIntro] Playing/looping: %dx%d"), Size.X, Size.Y);
}

void UDCFrontendIntroScreen::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (!bIntroActive) { return; }
	if (APlayerController* PC = GetOwningPlayer()) { PC->bShowMouseCursor = false; }
	const double Now = FPlatformTime::Seconds();
	if (!bOpenRequested)
	{
		bOpenRequested = true;
		OpenIntro();
		if (!bIntroActive) { return; }
	}
	if (!bStartedPlaying)
	{
		StartMovieIfReady();
		if (bIntroActive && !bStartedPlaying && Now - OpenStartedAt > MediaTimeoutSeconds) { FailIntro(TEXT("PV open timed out")); }
	}
	else if (Player)
	{
		const double Seconds = Player->GetTime().GetTotalSeconds();
		if (Texture && Texture->CurrentAspectRatio > 0.f && Seconds > 0.0) { bHasPresentedFrame = true; }
		// A loop's backward time jump is progress too.
		if (FMath::Abs(Seconds - LastMediaSeconds) > .001) { LastMediaSeconds = Seconds; LastProgressAt = Now; }
		if (Player->HasError() || Now - LastProgressAt > MediaTimeoutSeconds) { FailIntro(TEXT("PV playback stopped making progress")); }
	}
}

void UDCFrontendIntroScreen::MediaOpenFailed(FString) { if (bIntroActive) { FailIntro(TEXT("PV media open failed")); } }
void UDCFrontendIntroScreen::MediaEnded()
{
	if (bIntroActive && (!bHasPresentedFrame || !Player || !Player->IsLooping()))
	{
		FailIntro(TEXT("PV ended without decoded frames or looping"));
	}
}

void UDCFrontendIntroScreen::StopMedia()
{
	bMediaOpened = false;
	bStartedPlaying = false;
	if (MovieImage) { MovieImage->SetVisibility(ESlateVisibility::Collapsed); MovieImage->SetBrush(FSlateBrush()); }
	if (Sound) { Sound->Stop(); Sound->SetMediaPlayer(nullptr); Sound->DestroyComponent(); Sound = nullptr; }
	if (Player)
	{
		Player->OnMediaOpened.RemoveDynamic(this, &ThisClass::MediaOpened);
		Player->OnMediaOpenFailed.RemoveDynamic(this, &ThisClass::MediaOpenFailed);
		Player->OnEndReached.RemoveDynamic(this, &ThisClass::MediaEnded);
		Player->Close();
		Player = nullptr;
	}
	if (Texture) { Texture->SetMediaPlayer(nullptr); Texture = nullptr; }
}

void UDCFrontendIntroScreen::RestoreCursor()
{
	if (bCursorWasSaved && CursorOwner.IsValid()) { CursorOwner->bShowMouseCursor = bSavedCursorVisibility; }
	bCursorWasSaved = false;
	CursorOwner.Reset();
}

void UDCFrontendIntroScreen::FinishIntro()
{
	if (!bIntroActive) { return; }
	bIntroActive = false;
	StopMedia();
	ReleaseInputProcessor();
	RestoreCursor();
	UE_LOG(LogDreamCatcher, Log, TEXT("[FrontendIntro] Finished; showing mode selection"));
	DeactivateWidget();
}

void UDCFrontendIntroScreen::FailIntro(const TCHAR* Reason)
{
	UE_LOG(LogDreamCatcher, Warning, TEXT("[FrontendIntro] %s; continuing to mode selection"), Reason);
	FinishIntro();
}

void UDCFrontendIntroScreen::NativeOnDeactivated()
{
	bIntroActive = false;
	StopMedia();
	ReleaseInputProcessor();
	RestoreCursor();
	Super::NativeOnDeactivated();
}

void UDCFrontendIntroScreen::NativeDestruct()
{
	bIntroActive = false;
	StopMedia();
	ReleaseInputProcessor();
	RestoreCursor();
	Super::NativeDestruct();
}
