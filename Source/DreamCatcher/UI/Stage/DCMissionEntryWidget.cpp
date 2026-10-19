#include "UI/Stage/DCMissionEntryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableViewBase.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/WidgetStyle.h"

namespace DCMissionRow
{
constexpr float IconSize = 24.f;
constexpr float IconGap = 16.f;
constexpr float VerticalPadding = 6.f;

bool SameProgress(const FDCMissionProgress& A, const FDCMissionProgress& B)
{
	return A.MissionId == B.MissionId && A.DisplayText.IdenticalTo(B.DisplayText)
		&& A.ProgressSource == B.ProgressSource && A.CurrentCount == B.CurrentCount
		&& A.TargetCount == B.TargetCount && A.bCompleted == B.bCompleted && A.bTracked == B.bTracked;
}
}

void UDCMissionEntryData::Update(const FDCMissionProgress& InProgress, const FDCMissionHUDStyle& InStyle)
{
	if (DCMissionRow::SameProgress(Progress, InProgress)
		&& FDCMissionHUDStyle::StaticStruct()->CompareScriptStruct(&Appearance, &InStyle, 0)) { return; }
	Progress = InProgress; Appearance = InStyle;
	RowHeight = UDCMissionEntryWidget::MeasureRowHeight(Progress, Appearance);
	OnChanged.Broadcast();
}

void UDCMissionEntryFactory::Configure(TSubclassOf<UUserWidget> WidgetClass)
{
	EntryWidgetForClass.Reset();
	EntryWidgetForClass.Add(TSoftClassPtr<UObject>(UDCMissionEntryData::StaticClass()), WidgetClass);
}

UDCMissionListView::UDCMissionListView(const FObjectInitializer& Initializer) : Super(Initializer)
{
	SelectionMode = ESelectionMode::None;
	ConsumeMouseWheel = EConsumeMouseWheel::Never;
	bIsFocusable = false;
	bEnableProximateEntryNavigation = false;
	bSelectItemOnNavigation = false;
	bReturnFocusToSelection = false;
	bEnableRightClickScrolling = false;
	bEnableTouchScrolling = false;
	bIsPointerScrollingEnabled = false;
	bIsGamepadScrollingEnabled = false;
	bAllowDragging = false; bAllowDragDrop = false;
	AllowOverscroll = false; WheelScrollMultiplier = 0.f;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCMissionListView::Configure(TSubclassOf<UUserWidget> WidgetClass)
{
	// Both the original fallback and original class factory use the actual BP class.
	EntryWidgetClass = WidgetClass;
	FactoryRules.Reset();
	UDCMissionEntryFactory* Factory = NewObject<UDCMissionEntryFactory>(this);
	Factory->Configure(WidgetClass); FactoryRules.Add(Factory);
}

TSharedRef<STableViewBase> UDCMissionListView::RebuildListWidget()
{
	TSharedRef<STableViewBase> Result = Super::RebuildListWidget();
	Result->SetScrollbarVisibility(EVisibility::Collapsed);
	return Result;
}

FText UDCMissionEntryWidget::FormatProgress(const FDCMissionProgress& Progress)
{
	if (Progress.ProgressSource == EDCMissionProgressSource::Guidance) { return Progress.DisplayText; }
	if (Progress.bCompleted && Progress.ProgressSource == EDCMissionProgressSource::CurrentEncounterDefeats)
	{
		return FText::Format(NSLOCTEXT("DCMissionHUD", "CompletedProgress", "{0}/{1}  {2} · 완료"),
			FText::AsNumber(Progress.CurrentCount), FText::AsNumber(Progress.TargetCount), Progress.DisplayText);
	}
	return FText::Format(NSLOCTEXT("DCMissionHUD", "Progress", "{0}/{1}  {2}"),
		FText::AsNumber(Progress.CurrentCount), FText::AsNumber(Progress.TargetCount), Progress.DisplayText);
}

float UDCMissionEntryWidget::MeasureTextHeight(const FText& Text, int32 FontSize, float Width)
{
	TSharedRef<STextBlock> Measure = SNew(STextBlock).Text(Text)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", FontSize)).WrapTextAt(FMath::Max(1.f, Width))
		.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
	Measure->SlatePrepass();
	return FMath::CeilToFloat(Measure->GetDesiredSize().Y);
}

float UDCMissionEntryWidget::MeasureRowHeight(const FDCMissionProgress& Progress, const FDCMissionHUDStyle& Style)
{
	const float TextWidth = Style.PanelWidth - 2.f * Style.Padding - DCMissionRow::IconSize - DCMissionRow::IconGap;
	const float ContentHeight = FMath::Max(DCMissionRow::IconSize, MeasureTextHeight(FormatProgress(Progress), Style.FontSize, TextWidth));
	return FMath::Max(Style.RowMinHeight, ContentHeight + 2.f * DCMissionRow::VerticalPadding);
}

TSharedRef<SWidget> UDCMissionEntryWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		RowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MissionRow"));
		WidgetTree->RootWidget = RowBox;
		UHorizontalBox* Contents = WidgetTree->ConstructWidget<UHorizontalBox>(); RowBox->SetContent(Contents);
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
		IconBox->SetWidthOverride(DCMissionRow::IconSize); IconBox->SetHeightOverride(DCMissionRow::IconSize);
		Checkbox = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MissionCheckbox")); IconBox->SetContent(Checkbox);
		UHorizontalBoxSlot* IconSlot = Contents->AddChildToHorizontalBox(IconBox);
		IconSlot->SetVerticalAlignment(VAlign_Center); IconSlot->SetPadding(FMargin(0, 0, DCMissionRow::IconGap, 0));
		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MissionText"));
		Label->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		UHorizontalBoxSlot* TextSlot = Contents->AddChildToHorizontalBox(Label);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(0, DCMissionRow::VerticalPadding));
		SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	Refresh();
	return Super::RebuildWidget();
}

void UDCMissionEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UnbindEntry();
	Entry = Cast<UDCMissionEntryData>(ListItemObject);
	if (Entry.IsValid()) { ChangeHandle = Entry->OnChanged.AddUObject(this, &ThisClass::Refresh); }
	Refresh();
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
}

void UDCMissionEntryWidget::UnbindEntry()
{
	if (Entry.IsValid()) { Entry->OnChanged.Remove(ChangeHandle); }
	ChangeHandle.Reset(); Entry.Reset();
}

void UDCMissionEntryWidget::Refresh()
{
	if (!RowBox || !Checkbox || !Label) { return; }
	UDCMissionEntryData* Item = Entry.Get();
	if (!Item)
	{
		Label->SetText(FText::GetEmpty()); Checkbox->SetBrush(FSlateBrush());
		SetVisibility(ESlateVisibility::Collapsed); return;
	}
	const FDCMissionProgress& Progress = Item->GetProgress();
	const FDCMissionHUDStyle& Style = Item->GetStyle();
	RowBox->SetWidthOverride(Style.PanelWidth - 2.f * Style.Padding);
	RowBox->SetHeightOverride(Item->GetRowHeight());
	Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Style.FontSize));
	Label->SetColorAndOpacity(FSlateColor(Style.TextColor)); Label->SetText(FormatProgress(Progress));
	Label->SetWrapTextAt(Style.PanelWidth - 2.f * Style.Padding - DCMissionRow::IconSize - DCMissionRow::IconGap);
	FSlateBrush Brush = Progress.bCompleted ? Style.CompletedBrush : Style.CheckboxBrush;
	if (Brush.DrawAs == ESlateBrushDrawType::NoDrawType)
	{
		Brush.DrawAs = Progress.bCompleted ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Border;
		Brush.ImageSize = FVector2D(8, 8); Brush.Margin = FMargin(.125f);
		Brush.TintColor = FSlateColor(Style.TextColor);
	}
	// Material emissive colors are supplied by the unchanged source brush asset.
	// Do not assume Slate vertex tint is a UI material's final color multiplier.
	Checkbox->SetBrush(Brush);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCMissionEntryWidget::NativeOnEntryReleased()
{
	UnbindEntry(); Refresh(); IUserListEntry::NativeOnEntryReleased();
}

void UDCMissionEntryWidget::NativeDestruct()
{
	UnbindEntry(); Refresh();
	if (bDiagnosticVolatility) { ForceVolatile(bPreviousVolatility); bDiagnosticVolatility = false; }
	Super::NativeDestruct();
}

void UDCMissionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics")) && !bDiagnosticVolatility)
	{ bPreviousVolatility = bIsVolatile; ForceVolatile(true); bDiagnosticVolatility = true; }
}

int32 UDCMissionEntryWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics")) || !Entry.IsValid()
		|| !GetWorld() || !GetWorld()->IsGameWorld() || !Label) { return Layer; }
	auto Escape = [](const FString& Value) { return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""))
		.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t")); };
	const UDCMissionEntryData* Item = Entry.Get(); const FDCMissionProgress Row = Item->GetProgress();
	const FVector2D Size = Geometry.GetLocalSize();
	UE_LOG(LogTemp, Display, TEXT("DCMISSION_ROW_PAINT {\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"source\":\"%s\",\"phase_index\":%d,\"page\":%d,\"serial\":%lld,\"worldtime\":%.6f,\"alpha\":%.6f,\"id\":\"%s\",\"text\":\"%s\",\"current\":%d,\"target\":%d,\"completed\":%s,\"size\":[%.6f,%.6f]}"),
		GFrameCounter, *Escape(GetWorld()->GetPathName()), *Escape(GetPathName()), *Escape(Item->PaintSource),
		Item->PaintPhase, Item->PaintPage, Item->PaintSerial, GetWorld()->GetTimeSeconds(),
		Style.GetColorAndOpacityTint().A, *Escape(Row.MissionId.ToString()), *Escape(Label->GetText().ToString()),
		Row.CurrentCount, Row.TargetCount, Row.bCompleted ? TEXT("true") : TEXT("false"), Size.X, Size.Y);
	return Layer;
}

FText UDCMissionEntryWidget::GetDisplayedText() const { return Label ? Label->GetText() : FText::GetEmpty(); }
FName UDCMissionEntryWidget::GetDisplayedMissionId() const { return Entry.IsValid() ? Entry->GetProgress().MissionId : NAME_None; }
