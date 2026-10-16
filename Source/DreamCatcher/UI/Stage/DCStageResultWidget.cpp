#include "UI/Stage/DCStageResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "DCStageResult"

TSharedRef<SWidget> UDCStageResultWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StageRoot"));
		WidgetTree->RootWidget = Root;
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		TitlePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PhasePanel"));
		TitlePanel->SetPadding(FMargin(22, 12));
		TitlePanel->SetBrushColor(FLinearColor(.025f, .04f, .06f, .92f));
		UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(TitlePanel);
		TitleSlot->SetAnchors(FAnchors(.5f, 0.f));
		TitleSlot->SetAlignment(FVector2D(.5f, 0.f));
		TitleSlot->SetPosition(FVector2D(0, 24));
		TitleSlot->SetAutoSize(true);
		StageTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageTitle"));
		StageTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 22));
		StageTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TitlePanel->SetContent(StageTitle);

		ResultPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultPanel"));
		ResultPanel->SetPadding(FMargin(32));
		ResultPanel->SetBrushColor(FLinearColor(.025f, .04f, .06f, .98f));
		UCanvasPanelSlot* ResultSlot = Root->AddChildToCanvas(ResultPanel);
		ResultSlot->SetAnchors(FAnchors(.5f, .5f));
		ResultSlot->SetAlignment(FVector2D(.5f, .5f));
		ResultSlot->SetAutoSize(true);
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(360);
		ResultPanel->SetContent(Width);
		UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
		Width->SetContent(Rows);
		ResultTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultTitle"));
		ResultTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 30));
		ResultTitle->SetJustification(ETextJustify::Center);
		ResultTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Rows->AddChildToVerticalBox(ResultTitle)->SetPadding(FMargin(0, 0, 0, 24));

		auto AddButton = [this, Rows](FName Name, const FText& Label)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			Button->SetBackgroundColor(FLinearColor(.2f, .65f, .7f, 1.f));
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
			Text->SetText(Label);
			Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 22));
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
			Text->SetJustification(ETextJustify::Center);
			Button->SetContent(Text);
			Rows->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 8));
			return Button;
		};
		RetryButton = AddButton(TEXT("RetryButton"), LOCTEXT("Retry", "다시 시작"));
		ExitButton = AddButton(TEXT("ExitButton"), LOCTEXT("ReturnToMenu", "메인화면으로"));
		RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RetryClicked);
		ExitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ExitClicked);
	}
	RefreshText();
	return Super::RebuildWidget();
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
	if (TitlePanel) { TitlePanel->SetVisibility(bTitleVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
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
