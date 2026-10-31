#include "UI/Stage/DCStageResultWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "DCStageResult"

bool UDCStageResultWidget::HasDesignerContent() const
{
	return PhasePanel && StageTitle && ResultTitle && ResultPanel && RetryButton && ExitButton;
}

void UDCStageResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RetryButton) { RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RetryClicked); }
	if (ExitButton) { ExitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ExitClicked); }
	RefreshText();
}

void UDCStageResultWidget::NativeDestruct()
{
	if (RetryButton) { RetryButton->OnClicked.RemoveDynamic(this, &ThisClass::RetryClicked); }
	if (ExitButton) { ExitButton->OnClicked.RemoveDynamic(this, &ThisClass::ExitClicked); }
	bResultVisible = false; bActionRequested = false;
	Super::NativeDestruct();
}

void UDCStageResultWidget::SetStageTitle(const FText& Title)
{
	CurrentTitle = Title;
	RefreshText();
}

void UDCStageResultWidget::ShowResult(bool bInSucceeded)
{
	bResultVisible = true;
	bSucceeded = bInSucceeded;
	RefreshText();
	if (RetryButton && GetOwningPlayer()) { RetryButton->SetUserFocus(GetOwningPlayer()); }
}

void UDCStageResultWidget::RefreshText()
{
	if (PhasePanel) { PhasePanel->SetVisibility(bTitleVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
	if (StageTitle) { StageTitle->SetText(CurrentTitle); }
	if (ResultTitle) { ResultTitle->SetText(bSucceeded ? LOCTEXT("Clear", "스테이지 클리어") : LOCTEXT("Failed", "스테이지 실패")); }
	if (ResultPanel) { ResultPanel->SetVisibility(bResultVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
}

void UDCStageResultWidget::SetTravelPending(bool bPending)
{
	bActionRequested = bPending;
	if (RetryButton) { RetryButton->SetIsEnabled(!bPending); }
	if (ExitButton) { ExitButton->SetIsEnabled(!bPending); }
}

void UDCStageResultWidget::SetStageTitleVisible(bool bVisible)
{
	bTitleVisible = bVisible;
	RefreshText();
}

UWidget* UDCStageResultWidget::GetResultFocusTarget() const
{
	return RetryButton;
}

void UDCStageResultWidget::RetryClicked()
{
	if (!bResultVisible || bActionRequested) { return; }
	OnRetryRequested.Broadcast();
}

void UDCStageResultWidget::ExitClicked()
{
	if (!bResultVisible || bActionRequested) { return; }
	OnExitRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
