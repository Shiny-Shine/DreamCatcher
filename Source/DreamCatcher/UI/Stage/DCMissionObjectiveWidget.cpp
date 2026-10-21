#include "UI/Stage/DCMissionObjectiveWidget.h"
#include "UI/Stage/DCMissionEntryWidget.h"
#include "UI/Stage/DCMissionTransitionSwitcher.h"
#include "Stage/DCStageDirector.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#endif

namespace DCMissionHUD
{
constexpr float TitleGap = 8.f;

FString Escape(const FString& Value)
{
	return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""))
		.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
}
}

UDCMissionObjectiveWidget::UDCMissionObjectiveWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
	Pages.SetNum(2);
}

bool UDCMissionObjectiveWidget::IsEntryClassUsable(TSubclassOf<UUserWidget> EntryClass)
{
	if (!EntryClass || !EntryClass->IsChildOf(UDCMissionEntryWidget::StaticClass())
		|| !EntryClass->ImplementsInterface(UUserObjectListEntry::StaticClass())
		|| EntryClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) { return false; }
#if WITH_EDITOR
	if (!EntryClass->bCooked)
	{
		const UBlueprint* Blueprint = Cast<UBlueprint>(EntryClass->ClassGeneratedBy);
		if (!Blueprint || Blueprint->Status == BS_Error) { return false; }
	}
#endif
	return true;
}

FDCMissionHUDStyle UDCMissionObjectiveWidget::ResolveStyle(const FDCMissionHUDStyle& Source)
{
	if (Source.IsValid()) { return Source; }
	FDCMissionHUDStyle Safe;
	Safe.EntryWidgetClass = Source.EntryWidgetClass;
	Safe.PanelBrush = Source.PanelBrush; Safe.HeaderIconBrush = Source.HeaderIconBrush;
	Safe.bHeaderIncludedInPanel = Source.bHeaderIncludedInPanel;
	Safe.CheckboxBrush = Source.CheckboxBrush; Safe.CompletedBrush = Source.CompletedBrush;
	return Safe;
}

TSharedRef<SWidget> UDCMissionObjectiveWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MissionRoot"));
		WidgetTree->RootWidget = Root;
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MissionPanel"));
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
		PanelSlot->SetAnchors(FAnchors(1.f, 0.f)); PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
		Contents = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Contents);
		HeaderBox = WidgetTree->ConstructWidget<USizeBox>(); Contents->AddChildToVerticalBox(HeaderBox);
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(); HeaderBox->SetContent(Header);
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
		IconBox->SetWidthOverride(32); IconBox->SetHeightOverride(32);
		HeaderIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MissionHeaderIcon")); IconBox->SetContent(HeaderIcon);
		UHorizontalBoxSlot* IconSlot = Header->AddChildToHorizontalBox(IconBox);
		IconSlot->SetVerticalAlignment(VAlign_Center); IconSlot->SetPadding(FMargin(0, 0, 12, 0));
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MissionHeader"));
		HeaderText->SetText(NSLOCTEXT("DCMissionHUD", "Header", "MISSION"));
		Header->AddChildToHorizontalBox(HeaderText)->SetVerticalAlignment(VAlign_Center);
		BodySwitcher = WidgetTree->ConstructWidget<UDCMissionTransitionSwitcher>(UDCMissionTransitionSwitcher::StaticClass(), TEXT("MissionBodySwitcher"));
		Contents->AddChildToVerticalBox(BodySwitcher);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FDCMissionBodyPage& Page = Pages[Index];
			Page.Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("MissionBodyPage%d"), Index));
			BodySwitcher->AddChild(Page.Root);
			Page.PhaseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("MissionStageTitle%d"), Index));
			Page.PhaseText->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
			Page.Root->AddChildToVerticalBox(Page.PhaseText)->SetPadding(FMargin(0, 0, 0, DCMissionHUD::TitleGap));
			Page.ListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("MissionListArea%d"), Index));
			Page.Root->AddChildToVerticalBox(Page.ListBox);
		}
		BodySwitcher->OnMissionMidpoint.AddUObject(this, &ThisClass::HandleMidpoint);
		BodySwitcher->OnMissionFinished.AddUObject(this, &ThisClass::HandleFinished);
		BodySwitcher->SnapToPage(ActivePage, TransitionSerial);
		SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible);
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 Index = 0; Index < 2; ++Index) { SynchronizePage(Index); }
	SelectActiveAliases(); Refresh();
	return Super::RebuildWidget();
}

void UDCMissionObjectiveWidget::SetMissionStatus(const UDCMissionListData* Data, const FText& StageTitle,
	const TArray<FDCMissionProgress>& Progress)
{
	SetLiveMissionStatus(nullptr, INDEX_NONE, Data, StageTitle, Progress);
}

void UDCMissionObjectiveWidget::SetMissionSource(ADCStageDirector* Source)
{
	if (SourceDirector.Get() != Source) { ClearMissionStatus(); SourceDirector = Source; LastCompletedPhase = INDEX_NONE; }
}

bool UDCMissionObjectiveWidget::ValidatePresentation(const FDCMissionHUDStyle& Style, const TArray<FDCMissionProgress>& Progress)
{
	bool bValid = IsEntryClassUsable(Style.EntryWidgetClass);
	TSet<FName> Ids;
	for (const FDCMissionProgress& Row : Progress)
	{
		if (Row.MissionId.IsNone() || Ids.Contains(Row.MissionId)) { bValid = false; break; }
		Ids.Add(Row.MissionId);
	}
	if (!bValid)
	{
		if (!bReportedInvalidConfiguration)
		{
			UE_LOG(LogTemp, Warning, TEXT("Mission HUD requires unique mission IDs and a compiled Blueprint entry class; presentation hidden."));
		}
		bReportedInvalidConfiguration = true; ClearMissionStatus(); return false;
	}
	bReportedInvalidConfiguration = false;
	if (!Style.IsValid())
	{
		if (!bReportedStyleFallback)
		{
			UE_LOG(LogTemp, Warning, TEXT("Mission HUD invalid layout/text settings use safe defaults; entry class and original artwork are preserved."));
		}
		bReportedStyleFallback = true;
	}
	else { bReportedStyleFallback = false; }
	return true;
}

void UDCMissionObjectiveWidget::SetLiveMissionStatus(ADCStageDirector* Source, int32 PhaseIndex,
	const UDCMissionListData* Data, const FText& StageTitle, const TArray<FDCMissionProgress>& Progress)
{
	SetMissionSource(Source);
	if (!bResolvingMidpoint && (State == EBodyState::Holding || State == EBodyState::FadingOut)) { return; }
	if (!Data || !ValidatePresentation(Data->HUDStyle, Progress)) { if (!Data) { ClearMissionStatus(); } return; }
	const FDCMissionHUDStyle Style = ResolveStyle(Data->HUDStyle);
	if (bResolvingMidpoint && State == EBodyState::FadingOut)
	{
		ApplyPage(IncomingPage, PhaseIndex, StageTitle, Progress, Style);
		ActivePage = IncomingPage; bMidpointApplied = true; SelectActiveAliases(); Refresh(); return;
	}
	if (State == EBodyState::Holding || State == EBodyState::FadingOut) { return; }
	if (State == EBodyState::FadingIn && Pages[ActivePage].PhaseIndex != PhaseIndex)
	{
		++TransitionSerial; State = EBodyState::Live;
		if (BodySwitcher) { BodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	}
	if (State == EBodyState::Live) { Appearance = Style; }
	ApplyPage(ActivePage, PhaseIndex, StageTitle, Progress, Style);
	bHasStatus = true; SelectActiveAliases(); Refresh();
}

void UDCMissionObjectiveWidget::ApplyPage(int32 Index, int32 PhaseIndex, const FText& Title,
	const TArray<FDCMissionProgress>& Progress, const FDCMissionHUDStyle& Style)
{
	FDCMissionBodyPage& Page = Pages[Index];
	Page.PhaseIndex = PhaseIndex; Page.Title = Title; Page.Rows = Progress; Page.Style = Style;
	TMap<FName, UDCMissionEntryData*> Previous;
	for (UDCMissionEntryData* Item : Page.Items) { Previous.Add(Item->GetProgress().MissionId, Item); }
	TArray<TObjectPtr<UDCMissionEntryData>> Next;
	for (const FDCMissionProgress& Row : Progress)
	{
		UDCMissionEntryData* Item = Previous.FindRef(Row.MissionId);
		if (!Item) { Item = NewObject<UDCMissionEntryData>(this, NAME_None, RF_Transient); }
		Item->Update(Row, Style);
		Item->SetPaintContext(GetPathNameSafe(SourceDirector.Get()), PhaseIndex, Index, TransitionSerial);
		Next.Add(Item);
	}
	const bool bChanged = Next != Page.Items; Page.Items = MoveTemp(Next);
	SynchronizePage(Index);
	if (bChanged && Page.List) { Page.List->SetListItems(Page.Items); }
}

void UDCMissionObjectiveWidget::SynchronizePage(int32 Index)
{
	FDCMissionBodyPage& Page = Pages[Index];
	if (!Page.Root || !Page.ListBox || !Page.PhaseText || !IsEntryClassUsable(Page.Style.EntryWidgetClass)) { return; }
	if (Page.List && Page.List->GetEntryWidgetClass() != Page.Style.EntryWidgetClass) { RemoveList(Index); }
	if (!Page.List)
	{
		Page.List = WidgetTree->ConstructWidget<UDCMissionListView>(); Page.List->Configure(Page.Style.EntryWidgetClass);
		Page.List->SetListItems(Page.Items); Page.ListBox->SetContent(Page.List);
	}
	const float Width = Page.Style.PanelWidth - 2.f * Page.Style.Padding;
	Page.PhaseText->SetText(Page.Title); Page.PhaseText->SetWrapTextAt(Width);
	Page.PhaseText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Page.Style.StageFontSize));
	Page.PhaseText->SetColorAndOpacity(FSlateColor(Page.Style.TextColor));
	Page.PhaseText->SetVisibility(Page.Title.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	float Height = 0.f; for (const UDCMissionEntryData* Item : Page.Items) { Height += Item->GetRowHeight(); }
	Page.ListBox->SetHeightOverride(Height); Page.ListBox->SetWidthOverride(Width);
}

void UDCMissionObjectiveWidget::SelectActiveAliases()
{
	const FDCMissionBodyPage& Page = Pages[ActivePage];
	PhaseText = Page.PhaseText; ListBox = Page.ListBox; MissionList = Page.List;
	DisplayedMissions = Page.Rows; CurrentStageTitle = Page.Title;
}

void UDCMissionObjectiveWidget::RemoveList(int32 Index)
{
	FDCMissionBodyPage& Page = Pages[Index];
	if (Page.List)
	{
		Page.List->ClearListItems(); if (Page.ListBox) { Page.ListBox->ClearChildren(); }
		Page.List->ReleaseSlateResources(true); Page.List = nullptr;
	}
}

void UDCMissionObjectiveWidget::ClearPage(int32 Index)
{
	RemoveList(Index); FDCMissionBodyPage& Page = Pages[Index];
	Page.Items.Reset(); Page.Rows.Reset(); Page.Title = FText::GetEmpty(); Page.PhaseIndex = INDEX_NONE;
	Page.Style = FDCMissionHUDStyle();
	if (Page.PhaseText) { Page.PhaseText->SetText(FText::GetEmpty()); Page.PhaseText->SetVisibility(ESlateVisibility::Collapsed); }
	if (Page.ListBox) { Page.ListBox->SetHeightOverride(0.f); }
}

void UDCMissionObjectiveWidget::ShowCompletedPhase(ADCStageDirector* Source, const FDCMissionTransitionSnapshot& Snapshot)
{
	if (!IsValid(Source) || Source != SourceDirector.Get() || Snapshot.PhaseIndex < 0
		|| Source->IsActorBeingDestroyed() || Source->GetWorld() != GetWorld() || Source->GetRunState() != EDCStageRunState::Running
		|| Source->GetCurrentPhaseIndex() != Snapshot.PhaseIndex
		|| LastCompletedPhase == Snapshot.PhaseIndex
		|| !Snapshot.Missions.ContainsByPredicate([](const FDCMissionProgress& Row)
		{ return Row.ProgressSource == EDCMissionProgressSource::CurrentEncounterDefeats && Row.bCompleted
			&& Row.TargetCount > 0 && Row.CurrentCount >= Row.TargetCount; })
		|| !ValidatePresentation(Snapshot.HUDStyle, Snapshot.Missions)) { return; }
	LastCompletedPhase = Snapshot.PhaseIndex;
	++TransitionSerial;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(HoldTimer); }
	if (BodySwitcher) { ActivePage = BodySwitcher->GetDisplayedPageIndex(); BodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	ClearPage(1 - ActivePage);
	State = EBodyState::Holding;
	const FDCMissionHUDStyle Style = ResolveStyle(Snapshot.HUDStyle);
	if (!bHasStatus) { Appearance = Style; }
	ApplyPage(ActivePage, Snapshot.PhaseIndex, Snapshot.PhaseTitle, Snapshot.Missions, Style);
	bHasStatus = true; SelectActiveAliases(); Refresh();
	const int64 Serial = TransitionSerial;
	if (GetWorld() && Style.CompletedHoldDuration > 0)
	{
		FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(this, [this, Serial] { BeginFade(Serial); });
		GetWorld()->GetTimerManager().SetTimer(HoldTimer, Callback, Style.CompletedHoldDuration, false);
	}
	else { BeginFade(Serial); }
}

void UDCMissionObjectiveWidget::BeginFade(int64 Serial)
{
	if (Serial != TransitionSerial || State != EBodyState::Holding) { return; }
	if (!SourceDirector.IsValid() || SourceDirector->GetRunState() != EDCStageRunState::Running)
	{ ClearMissionStatus(); return; }
	FrozenPanelHeight = FMath::Clamp(RequiredContentHeight, Appearance.MinHeight, Appearance.MaxHeight);
	State = EBodyState::FadingOut; IncomingPage = 1 - ActivePage;
	if (!BodySwitcher || !BodySwitcher->FadeToPage(IncomingPage, Pages[ActivePage].Style.MissionTransitionDuration, Serial))
	{
		// No constructed Slate surface: refresh safely, without claiming an animation.
		HandleMidpoint(Serial, IncomingPage); HandleFinished(Serial);
	}
}

void UDCMissionObjectiveWidget::HandleMidpoint(int64 Serial, int32 Page)
{
	if (Serial != TransitionSerial || State != EBodyState::FadingOut || Page != IncomingPage || !SourceDirector.IsValid()) { return; }
	bMidpointApplied = false;
	{ TGuardValue<bool> Resolving(bResolvingMidpoint, true); RequestLatestLive.ExecuteIfBound(); }
	if (Serial != TransitionSerial || State != EBodyState::FadingOut) { return; }
	if (!bMidpointApplied) { ClearMissionStatus(); return; }
	State = EBodyState::FadingIn;
}

void UDCMissionObjectiveWidget::HandleFinished(int64 Serial)
{
	if (Serial != TransitionSerial || State != EBodyState::FadingIn) { return; }
	State = EBodyState::Live; Appearance = Pages[ActivePage].Style;
	ClearPage(1 - ActivePage); SelectActiveAliases(); Refresh();
}

void UDCMissionObjectiveWidget::ClearMissionStatus()
{
	++TransitionSerial; State = EBodyState::Live;
	LastCompletedPhase = INDEX_NONE; bResolvingMidpoint = false; bMidpointApplied = false; FrozenPanelHeight = 0.f;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(HoldTimer); }
	if (BodySwitcher) { ActivePage = BodySwitcher->GetDisplayedPageIndex(); BodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	bHasStatus = false; bContentOverflow = false; RequiredContentHeight = 0;
	for (int32 Index = 0; Index < 2; ++Index) { ClearPage(Index); }
	SourceDirector.Reset(); DisplayedMissions.Reset(); CurrentStageTitle = FText::GetEmpty(); SelectActiveAliases();
	if (Panel) { Panel->SetVisibility(ESlateVisibility::Collapsed); }
}

UWidget* UDCMissionObjectiveWidget::GetActiveBodyPage() const { return Pages[ActivePage].Root; }
int32 UDCMissionObjectiveWidget::GetDisplayedPhaseIndex() const { return Pages[ActivePage].PhaseIndex; }
FName UDCMissionObjectiveWidget::GetBodyTransitionState() const
{
	switch (State)
	{
	case EBodyState::Holding: return TEXT("holding");
	case EBodyState::FadingOut: return TEXT("fade_out");
	case EBodyState::FadingIn: return TEXT("fade_in");
	default: return TEXT("live");
	}
}

void UDCMissionObjectiveWidget::Refresh()
{
	if (!Panel || !Contents || !HeaderText || !BodySwitcher) { return; }
	if (!bHasStatus) { Panel->SetVisibility(ESlateVisibility::Collapsed); return; }
	SelectActiveAliases();
	const float Width = Appearance.PanelWidth - 2.f * Appearance.Padding;
	const float PhaseHeight = CurrentStageTitle.IsEmpty() ? 0.f
		: UDCMissionEntryWidget::MeasureTextHeight(CurrentStageTitle, Appearance.StageFontSize, Width) + DCMissionHUD::TitleGap;
	float RowsHeight = 0.f;
	for (const UDCMissionEntryData* Item : Pages[ActivePage].Items) { RowsHeight += Item->GetRowHeight(); }
	RequiredContentHeight = 2.f * Appearance.Padding + Appearance.HeaderHeight + PhaseHeight + RowsHeight;
	const bool bWasOverflow = bContentOverflow;
	bContentOverflow = RequiredContentHeight > Appearance.MaxHeight + KINDA_SMALL_NUMBER;
	if (bContentOverflow && !bWasOverflow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mission HUD content needs %.1f but MaxHeight is %.1f. Shorten/split the configured mission list; passive HUD does not scroll."),
			RequiredContentHeight, Appearance.MaxHeight);
	}
	const float PanelHeight = State == EBodyState::FadingOut || State == EBodyState::FadingIn
		? FrozenPanelHeight : FMath::Clamp(RequiredContentHeight, Appearance.MinHeight, Appearance.MaxHeight);
	UCanvasPanelSlot* PanelSlot = CastChecked<UCanvasPanelSlot>(Panel->Slot);
	PanelSlot->SetPosition(FVector2D(-Appearance.RightMargin, Appearance.TopOffset));
	PanelSlot->SetSize(FVector2D(Appearance.PanelWidth, PanelHeight));
	Panel->SetPadding(FMargin(Appearance.Padding)); Panel->SetBrush(Appearance.PanelBrush);
	HeaderBox->SetHeightOverride(Appearance.HeaderHeight);
	if (UWidget* HeaderContents = HeaderBox->GetContent())
	{
		HeaderContents->SetVisibility(Appearance.bHeaderIncludedInPanel ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}
	HeaderIcon->SetBrush(Appearance.HeaderIconBrush);
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Appearance.HeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(Appearance.TextColor));
	// Only the fixed outer frame owns the height budget. The two body slots stay
	// in place; long content is diagnosed by the existing overflow contract.
	if (ListBox) { ListBox->SetHeightOverride(FMath::Max(0.f, PanelHeight - 2.f * Appearance.Padding - Appearance.HeaderHeight - PhaseHeight)); }
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCMissionObjectiveWidget::NativeDestruct()
{
	ClearMissionStatus(); RequestLatestLive.Unbind(); Super::NativeDestruct();
}

int32 UDCMissionObjectiveWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	static const bool bDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics"));
	if (!bDiagnostics || GFrameCounter % 8 != 0 || !bHasStatus || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !Panel || !MissionList || !Panel->IsVisible()) { return Layer; }
	const FGeometry PanelGeometry = Panel->GetPaintSpaceGeometry(), ListGeometry = MissionList->GetPaintSpaceGeometry();
	const FVector2D RootSize = Geometry.GetLocalSize(), PanelSize = PanelGeometry.GetLocalSize(), ListSize = ListGeometry.GetLocalSize();
	if (RootSize.ContainsNaN() || PanelSize.ContainsNaN() || ListSize.ContainsNaN()
		|| RootSize.X <= 0 || RootSize.Y <= 0 || PanelSize.X <= 0 || PanelSize.Y <= 0) { return Layer; }
	const FVector2D Position = Geometry.AbsoluteToLocal(PanelGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D ListPosition = PanelGeometry.AbsoluteToLocal(ListGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	FString Rows, PaintedRows;
	for (const FDCMissionProgress& Row : DisplayedMissions)
	{
		if (!Rows.IsEmpty()) { Rows += TEXT(","); }
		Rows += FString::Printf(TEXT("{\"id\":\"%s\",\"text\":\"%s\",\"current\":%d,\"target\":%d,\"completed\":%s,\"tracked\":%s}"),
			*DCMissionHUD::Escape(Row.MissionId.ToString()), *DCMissionHUD::Escape(UDCMissionEntryWidget::FormatProgress(Row).ToString()),
			Row.CurrentCount, Row.TargetCount, Row.bCompleted ? TEXT("true") : TEXT("false"), Row.bTracked ? TEXT("true") : TEXT("false"));
	}
	for (const UUserWidget* EntryWidget : MissionList->GetDisplayedEntryWidgets())
	{
		const UDCMissionEntryWidget* Row = Cast<UDCMissionEntryWidget>(EntryWidget);
		if (!Row || !Row->IsVisible()) { continue; }
		const FGeometry RowGeometry = Row->GetPaintSpaceGeometry();
		const FVector2D RowPosition = ListGeometry.AbsoluteToLocal(RowGeometry.LocalToAbsolute(FVector2D::ZeroVector));
		const FVector2D RowSize = RowGeometry.GetLocalSize();
		if (!PaintedRows.IsEmpty()) { PaintedRows += TEXT(","); }
		PaintedRows += FString::Printf(TEXT("{\"id\":\"%s\",\"text\":\"%s\",\"position\":[%.6f,%.6f],\"size\":[%.6f,%.6f]}"),
			*DCMissionHUD::Escape(Row->GetDisplayedMissionId().ToString()), *DCMissionHUD::Escape(Row->GetDisplayedText().ToString()),
			RowPosition.X, RowPosition.Y, RowSize.X, RowSize.Y);
	}
	UE_LOG(LogTemp, Display, TEXT("DCMISSION_HUD_PAINT {\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"source\":\"%s\",\"active_page\":%d,\"phase_index\":%d,\"serial\":%lld,\"transition_state\":\"%s\",\"root_size\":[%.6f,%.6f],\"panel_position\":[%.6f,%.6f],\"panel_size\":[%.6f,%.6f],\"list_position\":[%.6f,%.6f],\"list_size\":[%.6f,%.6f],\"rows_count\":%d,\"required_height\":%.6f,\"overflow\":%s,\"phase\":\"%s\",\"rows\":[%s],\"painted_rows\":[%s]}"),
		GFrameCounter, *DCMissionHUD::Escape(GetWorld()->GetPathName()), *DCMissionHUD::Escape(GetPathName()),
		*DCMissionHUD::Escape(GetPathNameSafe(SourceDirector.Get())), ActivePage, GetDisplayedPhaseIndex(), TransitionSerial, *GetBodyTransitionState().ToString(), RootSize.X, RootSize.Y,
		Position.X, Position.Y, PanelSize.X, PanelSize.Y, ListPosition.X, ListPosition.Y, ListSize.X, ListSize.Y,
		DisplayedMissions.Num(), RequiredContentHeight, bContentOverflow ? TEXT("true") : TEXT("false"),
		*DCMissionHUD::Escape(CurrentStageTitle.ToString()), *Rows, *PaintedRows);
	return Layer;
}
