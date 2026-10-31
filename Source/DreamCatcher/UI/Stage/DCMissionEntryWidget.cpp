#include "UI/Stage/DCMissionEntryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Widgets/Views/STableViewBase.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/WidgetStyle.h"

namespace DCMissionRow
{
float TextWidth(const UWidgetTree* Tree, float Width)
{
	const USizeBox* Box = Tree ? Cast<USizeBox>(Tree->FindWidget(TEXT("MissionCheckboxBox"))) : nullptr;
	const UTextBlock* Text = Tree ? Cast<UTextBlock>(Tree->FindWidget(TEXT("MissionText"))) : nullptr;
	const UHorizontalBoxSlot* IconSlot = Box ? Cast<UHorizontalBoxSlot>(Box->Slot) : nullptr;
	const UHorizontalBoxSlot* TextSlot = Text ? Cast<UHorizontalBoxSlot>(Text->Slot) : nullptr;
	if (!Box || !Text || !IconSlot || !TextSlot || !Box->IsWidthOverride()) { return 0.f; }
	const FMargin IconPadding = IconSlot->GetPadding(), TextPadding = TextSlot->GetPadding();
	return Width - Box->GetWidthOverride() - IconPadding.Left - IconPadding.Right - TextPadding.Left - TextPadding.Right;
}

bool SameProgress(const FDCMissionProgress& A, const FDCMissionProgress& B)
{
	return A.MissionId == B.MissionId && A.DisplayText.IdenticalTo(B.DisplayText)
		&& A.ProgressSource == B.ProgressSource && A.CurrentCount == B.CurrentCount
		&& A.TargetCount == B.TargetCount && A.bCompleted == B.bCompleted && A.bTracked == B.bTracked;
}
}

void UDCMissionEntryData::Update(const FDCMissionProgress& InProgress, float InWidth, float InHeight)
{
	if (DCMissionRow::SameProgress(Progress, InProgress) && AvailableWidth == InWidth && RowHeight == InHeight) { return; }
	Progress = InProgress; AvailableWidth = InWidth; RowHeight = InHeight;
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
	if (GetCachedWidget().IsValid()) { RegenerateAllEntries(); }
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

const UWidgetTree* UDCMissionEntryWidget::GetDesignerTree(TSubclassOf<UUserWidget> EntryClass)
{
	UWidgetBlueprintGeneratedClass* Class = Cast<UWidgetBlueprintGeneratedClass>(EntryClass.Get());
	Class = Class ? Class->FindWidgetTreeOwningClass() : nullptr;
	return Class ? Class->GetWidgetTreeArchetype() : nullptr;
}

UTextBlock* UDCMissionEntryWidget::CreateMeasurementText(const UWidgetTree* Design, UObject* Outer)
{
	const UTextBlock* Text = Design ? Cast<UTextBlock>(Design->FindWidget(TEXT("MissionText"))) : nullptr;
	if (!Text) { return nullptr; }
	// One transient text clone per page/class preserves font, shaping, margins and line spacing.
	// Measuring never builds Slate on a saved Blueprint template or modifies that template.
	UTextBlock* Copy = DuplicateObject<UTextBlock>(Text, Outer, MakeUniqueObjectName(Outer, Text->GetClass(), TEXT("MissionMeasureText")));
	Copy->ClearFlags(RF_ArchetypeObject | RF_ClassDefaultObject | RF_Public | RF_Standalone);
	Copy->SetFlags(RF_Transient); Copy->Slot = nullptr;
	return Copy;
}

float UDCMissionEntryWidget::MeasureTextHeight(UTextBlock* TextWidget, const FText& Text, float Width)
{
	if (!TextWidget || !FMath::IsFinite(Width) || Width <= 0.f
		|| !FMath::IsFinite(TextWidget->GetFont().Size) || TextWidget->GetFont().Size <= 0.f) { return 0.f; }
	TextWidget->SetText(Text); TextWidget->SetWrapTextAt(Width);
	const TSharedRef<SWidget> Slate = TextWidget->TakeWidget();
	Slate->SlatePrepass(1.f);
	return FMath::CeilToFloat(Slate->GetDesiredSize().Y);
}

float UDCMissionEntryWidget::MeasureRowHeight(const FDCMissionProgress& Progress, const UWidgetTree* Design,
	UTextBlock* MeasurementText, float Width)
{
	const USizeBox* Row = Design ? Cast<USizeBox>(Design->FindWidget(TEXT("MissionRow"))) : nullptr;
	const USizeBox* Box = Design ? Cast<USizeBox>(Design->FindWidget(TEXT("MissionCheckboxBox"))) : nullptr;
	const UTextBlock* Text = Design ? Cast<UTextBlock>(Design->FindWidget(TEXT("MissionText"))) : nullptr;
	const UHorizontalBoxSlot* IconSlot = Box ? Cast<UHorizontalBoxSlot>(Box->Slot) : nullptr;
	const UHorizontalBoxSlot* TextSlot = Text ? Cast<UHorizontalBoxSlot>(Text->Slot) : nullptr;
	const float AvailableText = DCMissionRow::TextWidth(Design, Width);
	if (!Row || !Box || !Text || !MeasurementText || !IconSlot || !TextSlot || !Box->IsHeightOverride()
		|| !FMath::IsFinite(AvailableText) || AvailableText <= 0.f) { return 0.f; }
	const FMargin IconPadding = IconSlot->GetPadding(), TextPadding = TextSlot->GetPadding();
	const float TextHeight = MeasureTextHeight(MeasurementText, FormatProgress(Progress), AvailableText)
		+ TextPadding.Top + TextPadding.Bottom;
	const float IconHeight = Box->GetHeightOverride() + IconPadding.Top + IconPadding.Bottom;
	const float Minimum = Row->IsMinDesiredHeightOverride() ? Row->GetMinDesiredHeight() : 0.f;
	if (!FMath::IsFinite(Minimum) || Minimum < 0.f || !FMath::IsFinite(IconHeight) || IconHeight < 0.f
		|| !FMath::IsFinite(TextHeight) || TextHeight < 0.f) { return 0.f; }
	return FMath::Max(Minimum, FMath::Max(TextHeight, IconHeight));
}

void UDCMissionEntryWidget::BindDesignerWidgets()
{
	MissionRow = Cast<USizeBox>(GetWidgetFromName(TEXT("MissionRow")));
	MissionCheckboxBox = Cast<USizeBox>(GetWidgetFromName(TEXT("MissionCheckboxBox")));
	MissionCheckbox = Cast<UImage>(GetWidgetFromName(TEXT("MissionCheckbox")));
	MissionText = Cast<UTextBlock>(GetWidgetFromName(TEXT("MissionText")));
	if (MissionCheckbox && !bBrushCached) { UncheckedBrush = MissionCheckbox->GetBrush(); bBrushCached = true; }
}

TSharedRef<SWidget> UDCMissionEntryWidget::RebuildWidget()
{
	BindDesignerWidgets();
	if (!IsDesignTime()) { SetIsFocusable(false); Refresh(); }
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
	// A pooled BP row can receive its item after Initialize but before RebuildWidget.
	if (!bBrushCached) { BindDesignerWidgets(); }
	if (!MissionRow || !MissionCheckboxBox || !MissionCheckbox || !MissionText || IsDesignTime()) { return; }
	const UDCMissionEntryData* Item = Entry.Get();
	if (!Item)
	{
		MissionText->SetText(FText::GetEmpty()); MissionCheckbox->SetBrush(UncheckedBrush);
		SetVisibility(ESlateVisibility::Collapsed); return;
	}
	MissionRow->SetWidthOverride(Item->GetAvailableWidth());
	MissionRow->SetHeightOverride(Item->GetRowHeight());
	MissionText->SetWrapTextAt(DCMissionRow::TextWidth(WidgetTree, Item->GetAvailableWidth()));
	MissionText->SetText(FormatProgress(Item->GetProgress()));
	MissionCheckbox->SetBrush(Item->GetProgress().bCompleted ? CompletedBrush : UncheckedBrush);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCMissionEntryWidget::NativeOnEntryReleased()
{
	UnbindEntry(); Refresh(); IUserListEntry::NativeOnEntryReleased();
}

void UDCMissionEntryWidget::NativeDestruct()
{
	UnbindEntry(); Refresh();
	if (MissionCheckbox && bBrushCached) { MissionCheckbox->SetBrush(UncheckedBrush); }
	bBrushCached = false;
	if (bDiagnosticVolatility) { ForceVolatile(bPreviousVolatility); bDiagnosticVolatility = false; }
	Super::NativeDestruct();
}

void UDCMissionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindDesignerWidgets(); Refresh();
	if (FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics")) && !bDiagnosticVolatility)
	{ bPreviousVolatility = bIsVolatile; ForceVolatile(true); bDiagnosticVolatility = true; }
}

int32 UDCMissionEntryWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics")) || !Entry.IsValid()
		|| !GetWorld() || !GetWorld()->IsGameWorld() || !MissionText) { return Layer; }
	auto Escape = [](const FString& Value) { return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""))
		.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t")); };
	const UDCMissionEntryData* Item = Entry.Get(); const FDCMissionProgress Row = Item->GetProgress();
	const FVector2D Size = Geometry.GetLocalSize();
	UE_LOG(LogTemp, Display, TEXT("DCMISSION_ROW_PAINT {\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"source\":\"%s\",\"phase_index\":%d,\"page\":%d,\"serial\":%lld,\"worldtime\":%.6f,\"alpha\":%.6f,\"id\":\"%s\",\"text\":\"%s\",\"current\":%d,\"target\":%d,\"completed\":%s,\"size\":[%.6f,%.6f]}"),
		GFrameCounter, *Escape(GetWorld()->GetPathName()), *Escape(GetPathName()), *Escape(Item->PaintSource),
		Item->PaintPhase, Item->PaintPage, Item->PaintSerial, GetWorld()->GetTimeSeconds(),
		Style.GetColorAndOpacityTint().A, *Escape(Row.MissionId.ToString()), *Escape(MissionText->GetText().ToString()),
		Row.CurrentCount, Row.TargetCount, Row.bCompleted ? TEXT("true") : TEXT("false"), Size.X, Size.Y);
	return Layer;
}

FText UDCMissionEntryWidget::GetDisplayedText() const { return MissionText ? MissionText->GetText() : FText::GetEmpty(); }
FName UDCMissionEntryWidget::GetDisplayedMissionId() const { return Entry.IsValid() ? Entry->GetProgress().MissionId : NAME_None; }
