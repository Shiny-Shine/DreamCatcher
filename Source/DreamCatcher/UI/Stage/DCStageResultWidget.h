#pragma once

#include "Blueprint/UserWidget.h"
#include "DCStageResultWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCStageResultAction);

/** Designer-authored result content shared by the legacy and CommonUI hosts. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCStageResultWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetStageTitle(const FText& Title);
	void ShowResult(bool bSucceeded);
	void SetTravelPending(bool bPending);
	void SetStageTitleVisible(bool bVisible);
	UWidget* GetResultFocusTarget() const;
	bool HasDesignerContent() const;
	UFUNCTION(BlueprintPure, Category="Stage")
	bool IsResultVisible() const { return bResultVisible; }
	UPROPERTY(BlueprintAssignable, Category="Stage")
	FDCStageResultAction OnRetryRequested;
	UPROPERTY(BlueprintAssignable, Category="Stage")
	FDCStageResultAction OnExitRequested;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void RefreshText();
	UFUNCTION()
	void RetryClicked();
	UFUNCTION()
	void ExitClicked();
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> StageTitle;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> PhasePanel;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ResultTitle;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> ResultPanel;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ExitButton;
	FText CurrentTitle;
	bool bResultVisible = false;
	bool bSucceeded = false;
	bool bActionRequested = false;
	bool bTitleVisible = true;
};
