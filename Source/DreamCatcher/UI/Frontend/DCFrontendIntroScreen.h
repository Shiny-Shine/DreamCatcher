#pragma once

#include "UI/Lyra/DCLyraActivatableWidget.h"
#include "InputCoreTypes.h"
#include "DCFrontendIntroScreen.generated.h"

class FDCFrontendIntroInputProcessor;
class APlayerController;
class IInputProcessor;
class UImage;
class UMediaPlayer;
class UMediaSoundComponent;
class UMediaSource;
class UMediaTexture;
struct FPointerEvent;

/** A fresh, skippable PV presentation for each frontend-world visit. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCFrontendIntroScreen : public UDCLyraActivatableWidget
{
	GENERATED_BODY()
public:
	UDCFrontendIntroScreen(const FObjectInitializer& Initializer);
	UFUNCTION(BlueprintCallable, Category="Frontend|Intro")
	void SkipIntro();
	UFUNCTION(BlueprintPure, Category="Frontend|Intro")
	UMediaPlayer* GetIntroPlayer() const { return Player; }
	UFUNCTION(BlueprintPure, Category="Frontend|Intro")
	bool IsIntroPlaying() const { return bIntroActive && bStartedPlaying; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, Category="Frontend|Intro")
	TObjectPtr<UMediaSource> IntroMediaSource;
	UPROPERTY(EditDefaultsOnly, Category="Frontend|Intro", meta=(ClampMin="1.0"))
	float MediaTimeoutSeconds = 15.f;

private:
	friend class FDCFrontendIntroInputProcessor;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCFrontendIntroTestAccess;
#endif
	void InstallInputProcessor();
	void ReleaseInputProcessor();
	void SkipFromInput(FKey Key);
	bool OwnsKeyboardFocus() const;
	bool ContainsPointer(const FPointerEvent& Event) const;
	void OpenIntro();
	void StartMovieIfReady();
	void StopMedia();
	void RestoreCursor();
	void FinishIntro();
	void FailIntro(const TCHAR* Reason);
	UFUNCTION() void MediaOpened(FString Url);
	UFUNCTION() void MediaOpenFailed(FString Url);
	UFUNCTION() void MediaEnded();

	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
	UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
	UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
	UPROPERTY(Transient) TObjectPtr<UImage> MovieImage;
	TSharedPtr<IInputProcessor> InputProcessor;
	TWeakObjectPtr<APlayerController> CursorOwner;
	bool bSavedCursorVisibility = false;
	bool bCursorWasSaved = false;
	bool bIntroActive = false;
	bool bMediaOpened = false;
	bool bOpenRequested = false;
	bool bStartedPlaying = false;
	bool bHasPresentedFrame = false;
	double OpenStartedAt = 0.0;
	double LastProgressAt = 0.0;
	double LastMediaSeconds = -1.0;
};
