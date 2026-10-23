#pragma once

#include "UI/Lyra/DCLyraActivatableWidget.h"
#include "DCFrontendScreen.generated.h"

class ADCFrontendGameMode;
class UCommonButtonBase;
class UDCModeCardButton;
class UTextBlock;

/** Mode-selection artwork lives in W_DC_Frontend; this class routes its actions. */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCFrontendScreen : public UDCLyraActivatableWidget
{
	GENERATED_BODY()
public:
	UDCFrontendScreen(const FObjectInitializer& ObjectInitializer);
	UFUNCTION(BlueprintCallable, Category="Frontend") void StartStory();
	UFUNCTION(BlueprintCallable, Category="Frontend") void OpenSettings();
	UFUNCTION(BlueprintCallable, Category="Frontend") void RequestQuit();
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, Category="Frontend")
	TSubclassOf<UCommonActivatableWidget> SettingsScreenClass;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDCModeCardButton> StoryButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDCModeCardButton> FreeButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDCModeCardButton> ChallengeButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> SettingsButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> QuitButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
private:
	void FreeModeUnavailable();
	void ChallengeModeUnavailable();
	void ShowUnavailableMode(const FText& ModeName);
	void StoryTravelFailed(const FText& Message);
	void SetTravelPending(bool bPending);
	void CloseSettings();
	void SettingsClosed();
	TWeakObjectPtr<ADCFrontendGameMode> Mode;
	TWeakObjectPtr<UCommonActivatableWidget> SettingsScreen;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle SettingsDeactivateHandle;
	bool bTravelPending = false;
	bool bDialogOpen = false;
};
