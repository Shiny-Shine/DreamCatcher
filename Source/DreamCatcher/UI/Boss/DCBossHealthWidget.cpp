#include "UI/Boss/DCBossHealthWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UDCBossHealthWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		UBorder* Back = WidgetTree->ConstructWidget<UBorder>();
		Back->SetBrushColor(FLinearColor(.025f, .03f, .055f, .95f));
		Back->SetPadding(FMargin(16));
		UCanvasPanelSlot* BossPanelSlot = Root->AddChildToCanvas(Back);
		BossPanelSlot->SetAnchors(FAnchors(.5f, 0.f));
		BossPanelSlot->SetAlignment(FVector2D(.5f, 0.f));
		BossPanelSlot->SetPosition(FVector2D(0, 88));
		BossPanelSlot->SetAutoSize(true);
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(540);
		Back->SetContent(Width);
		UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
		Width->SetContent(Rows);
		Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
		Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Title->SetJustification(ETextJustify::Center);
		Rows->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 8));
		USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetHeightOverride(18);
		Rows->AddChildToVerticalBox(Height);
		Bar = WidgetTree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(FLinearColor(.95f, .24f, .16f));
		Height->SetContent(Bar);
	}
	Refresh();
	return Super::RebuildWidget();
}

void UDCBossHealthWidget::SetBossStatus(const FText& Name, float Health, float MaxHealth, int32 Phase, bool bTransitioning)
{
	Fraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	const FText PhaseText = bTransitioning ? NSLOCTEXT("DCBoss", "Transition", "페이즈 전환")
		: FText::Format(NSLOCTEXT("DCBoss", "Phase", "페이즈 {0}"), FText::AsNumber(Phase));
	Caption = FText::Format(NSLOCTEXT("DCBoss", "Health", "{0}  ·  {1}  ·  {2}/{3}"),
		Name, PhaseText, FText::AsNumber(FMath::CeilToInt(Health)), FText::AsNumber(FMath::CeilToInt(MaxHealth)));
	Refresh();
}

void UDCBossHealthWidget::Refresh()
{
	if (Title) { Title->SetText(Caption); }
	if (Bar) { Bar->SetPercent(Fraction); }
}
