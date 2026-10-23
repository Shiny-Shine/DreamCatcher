#pragma once

#include "UI/Lyra/DCLyraActivatableWidget.h"
#include "DCMissionClearWidget.generated.h"

class ADCStageRunGameMode;
class UButton;
class UImage;
class UMediaPlayer;
class UMediaSoundComponent;
class UMediaTexture;
class UTextBlock;
class UVerticalBox;

/** Local story completion. Only the media end event authorizes automatic travel. */
UCLASS()
class DREAMCATCHER_API UDCMissionClearWidget : public UDCLyraActivatableWidget
{
	GENERATED_BODY()
public:
	UDCMissionClearWidget(const FObjectInitializer& Initializer);
	void BindStage(ADCStageRunGameMode* Stage);
	UFUNCTION(BlueprintPure, Category="Stage|Mission Clear")
	bool HasPlaybackFinished() const { return bPlaybackFinished; }
	UFUNCTION(BlueprintPure, Category="Stage|Mission Clear")
	UMediaPlayer* GetMissionClearPlayer() const { return Player; }
	UFUNCTION(BlueprintPure, Category="Stage|Mission Clear")
	bool IsShowingFailure() const;
	UFUNCTION(BlueprintPure, Category="Stage|Mission Clear")
	float GetBlackoutOpacity() const;
	void ShowFailure(const FText& Message);
	void StopPresentation();
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override { return true; }
private:
	enum class EPresentation : uint8 { Idle, Fading, Opening, Playing, Finished, Failed, Stopped };
	void OpenMovie();
	void StartMovieIfReady();
	void StopMedia();
	void ConsumeEscape() {}
	UFUNCTION() void MediaOpened(FString Url);
	UFUNCTION() void MediaOpenFailed(FString Url);
	UFUNCTION() void MediaEnded();
	UFUNCTION() void RestartLevel();
	UFUNCTION() void ReturnToModes();
	TWeakObjectPtr<ADCStageRunGameMode> Mode;
	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
	UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
	UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
	UPROPERTY(Transient) TObjectPtr<UImage> Blackout;
	UPROPERTY(Transient) TObjectPtr<UImage> MovieImage;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ErrorPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ErrorText;
	UPROPERTY(Transient) TObjectPtr<UButton> RetryButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnButton;
	EPresentation Presentation = EPresentation::Idle;
	double PhaseStartedAt = 0.0;
	double LastProgressAt = 0.0;
	double LastMediaSeconds = -1.0;
	float FadeDuration = .5f;
	bool bPlaybackFinished = false;
	bool bMediaOpened = false;
	bool bHasPresentedVideoFrame = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCStageFlowTestAccess;
	friend struct FDCStageUITestAccess;
#endif
};
