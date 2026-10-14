#pragma once

#include "Blueprint/UserWidget.h"
#include "DCStageResultWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCStageResultAction);

/** Temporary R11 presentation only; the final Lyra HUD/layout work remains R13. */
UCLASS()
class DREAMCATCHER_API UDCStageResultWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetStageTitle(const FText& Title);
	void ShowResult(bool bSucceeded);
	void SetTravelPending(bool bPending);
	void SetStageTitleVisible(bool bVisible);
	UWidget* GetResultFocusTarget() const;
	UFUNCTION(BlueprintPure, Category="Stage")
	bool IsResultVisible() const { return bResultVisible; }
	UPROPERTY(BlueprintAssignable, Category="Stage")
	FDCStageResultAction OnRetryRequested;
	UPROPERTY(BlueprintAssignable, Category="Stage")
	FDCStageResultAction OnExitRequested;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	void RefreshText();
	UFUNCTION()
	void RetryClicked();
	UFUNCTION()
	void ExitClicked();
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageTitle;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> TitlePanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultTitle;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ResultPanel;
	UPROPERTY(Transient)
	TObjectPtr<UButton> RetryButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ExitButton;
	FText CurrentTitle;
	bool bResultVisible = false;
	bool bSucceeded = false;
	bool bActionRequested = false;
	bool bTitleVisible = true;
};
