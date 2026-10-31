#include "UI/Stage/DCMissionObjectiveWidget.h"
#include "UI/Stage/DCMissionEntryWidget.h"
#include "UI/Stage/DCMissionTransitionSwitcher.h"
#include "Stage/DCStageDirector.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#endif

namespace DCMissionHUD
{
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
	const UWidgetTree* Design = UDCMissionEntryWidget::GetDesignerTree(EntryClass);
	return Design && Cast<USizeBox>(Design->FindWidget(TEXT("MissionRow")))
		&& Cast<USizeBox>(Design->FindWidget(TEXT("MissionCheckboxBox")))
		&& Cast<UImage>(Design->FindWidget(TEXT("MissionCheckbox")))
		&& Cast<UTextBlock>(Design->FindWidget(TEXT("MissionText")));
}

FDCMissionHUDStyle UDCMissionObjectiveWidget::ResolveStyle(const FDCMissionHUDStyle& Source)
{
	if (Source.IsValid()) { return Source; }
	FDCMissionHUDStyle Safe;
	Safe.EntryWidgetClass = Source.EntryWidgetClass;
	return Safe;
}

bool UDCMissionObjectiveWidget::BindDesignerWidgets()
{
	MissionPanelSize = Cast<USizeBox>(GetWidgetFromName(TEXT("MissionPanelSize")));
	MissionPanel = Cast<UBorder>(GetWidgetFromName(TEXT("MissionPanel")));
	MissionContents = Cast<UVerticalBox>(GetWidgetFromName(TEXT("MissionContents")));
	MissionHeaderSpace = Cast<USizeBox>(GetWidgetFromName(TEXT("MissionHeaderSpace")));
	MissionBodySwitcher = Cast<UDCMissionTransitionSwitcher>(GetWidgetFromName(TEXT("MissionBodySwitcher")));
	bool bValid = MissionPanelSize && MissionPanel && MissionContents && MissionHeaderSpace && MissionBodySwitcher;
	if (bValid)
	{
		bValid = MissionPanel->GetParent() == MissionPanelSize && MissionContents->GetParent() == MissionPanel
			&& MissionHeaderSpace->GetParent() == MissionContents && MissionBodySwitcher->GetParent() == MissionContents
			&& MissionBodySwitcher->GetChildrenCount() == 2;
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FDCMissionBodyPage& Page = Pages[Index];
		Page.Root = Cast<UVerticalBox>(GetWidgetFromName(*FString::Printf(TEXT("MissionBodyPage%d"), Index)));
		Page.PhaseText = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("MissionStageTitle%d"), Index)));
		Page.ListBox = Cast<USizeBox>(GetWidgetFromName(*FString::Printf(TEXT("MissionListArea%d"), Index)));
		Page.List = Cast<UDCMissionListView>(GetWidgetFromName(*FString::Printf(TEXT("MissionList%d"), Index)));
		bValid &= Page.Root && Page.PhaseText && Page.ListBox && Page.List;
		if (Page.Root && Page.PhaseText && Page.ListBox && Page.List)
		{
			bValid &= MissionBodySwitcher && MissionBodySwitcher->GetChildAt(Index) == Page.Root
				&& Page.Root->GetParent() == MissionBodySwitcher && Page.PhaseText->GetParent() == Page.Root
				&& Page.ListBox->GetParent() == Page.Root && Page.List->GetParent() == Page.ListBox;
		}
	}
	bDesignerTreeReady = bValid;
	return bValid;
}

float UDCMissionObjectiveWidget::GetContentWidth() const
{
	if (!MissionPanelSize || !MissionPanel) { return 0.f; }
	const FMargin PanelPadding = MissionPanel->GetPadding();
	return MissionPanelSize->GetWidthOverride() - PanelPadding.Left - PanelPadding.Right;
}

bool UDCMissionObjectiveWidget::HasValidDesignerLayout() const
{
	if (!bDesignerTreeReady || !MissionPanelSize->IsWidthOverride()
		|| !MissionPanelSize->IsMinDesiredHeightOverride() || !MissionPanelSize->IsMaxDesiredHeightOverride()
		|| !MissionHeaderSpace->IsHeightOverride()) { return false; }
	const FMargin PanelPadding = MissionPanel->GetPadding();
	const float Minimum = MissionPanelSize->GetMinDesiredHeight(), Maximum = MissionPanelSize->GetMaxDesiredHeight();
	const float Header = MissionHeaderSpace->GetHeightOverride(), Width = GetContentWidth();
	return FMath::IsFinite(PanelPadding.Left) && PanelPadding.Left >= 0 && FMath::IsFinite(PanelPadding.Right) && PanelPadding.Right >= 0
		&& FMath::IsFinite(PanelPadding.Top) && PanelPadding.Top >= 0 && FMath::IsFinite(PanelPadding.Bottom) && PanelPadding.Bottom >= 0
		&& FMath::IsFinite(Header) && Header >= 0 && FMath::IsFinite(Width) && Width > 0
		&& FMath::IsFinite(Minimum) && Minimum >= PanelPadding.Top + PanelPadding.Bottom + Header
		&& FMath::IsFinite(Maximum) && Maximum >= Minimum;
}

float UDCMissionObjectiveWidget::GetPhaseHeight(int32 Index)
{
	FDCMissionBodyPage& Page = Pages[Index];
	if (!Page.PhaseText || Page.Title.IsEmpty() || Page.PhaseText->GetVisibility() == ESlateVisibility::Collapsed) { return 0.f; }
	const UVerticalBoxSlot* PhaseSlot = Cast<UVerticalBoxSlot>(Page.PhaseText->Slot);
	const FMargin PhasePadding = PhaseSlot ? PhaseSlot->GetPadding() : FMargin(0);
	return UDCMissionEntryWidget::MeasureTextHeight(Page.PhaseText, Page.Title, GetContentWidth())
		+ PhasePadding.Top + PhasePadding.Bottom;
}

TSharedRef<SWidget> UDCMissionObjectiveWidget::RebuildWidget()
{
	BindDesignerWidgets();
	if (MissionBodySwitcher)
	{
		MissionBodySwitcher->OnMissionMidpoint.RemoveAll(this);
		MissionBodySwitcher->OnMissionFinished.RemoveAll(this);
		MissionBodySwitcher->OnMissionMidpoint.AddUObject(this, &ThisClass::HandleMidpoint);
		MissionBodySwitcher->OnMissionFinished.AddUObject(this, &ThisClass::HandleFinished);
		MissionBodySwitcher->SnapToPage(ActivePage, TransitionSerial);
	}
	if (!IsDesignTime())
	{
		SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible);
		for (int32 Index = 0; Index < 2; ++Index) { SynchronizePage(Index); }
		SelectActiveAliases(); Refresh();
	}
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
	if (!bDesignerTreeReady) { BindDesignerWidgets(); }
	bool bValid = HasValidDesignerLayout() && IsEntryClassUsable(Style.EntryWidgetClass);
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
			UE_LOG(LogTemp, Warning, TEXT("Mission HUD requires unique mission IDs, the named Designer hierarchy and a compiled Designer entry class; presentation hidden."));
		}
		bReportedInvalidConfiguration = true; ClearMissionStatus(); return false;
	}
	bReportedInvalidConfiguration = false;
	if (!Style.IsValid())
	{
		if (!bReportedStyleFallback)
		{
			UE_LOG(LogTemp, Warning, TEXT("Mission HUD invalid transition timing uses safe defaults; the selected entry class is preserved."));
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
		if (!ApplyPage(IncomingPage, PhaseIndex, StageTitle, Progress, Style)) { ClearMissionStatus(); return; }
		ActivePage = IncomingPage; bMidpointApplied = true; SelectActiveAliases(); Refresh(); return;
	}
	if (State == EBodyState::Holding || State == EBodyState::FadingOut) { return; }
	if (State == EBodyState::FadingIn && Pages[ActivePage].PhaseIndex != PhaseIndex)
	{
		++TransitionSerial; State = EBodyState::Live;
		if (MissionBodySwitcher) { MissionBodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	}
	if (State == EBodyState::Live) { Appearance = Style; }
	if (!ApplyPage(ActivePage, PhaseIndex, StageTitle, Progress, Style)) { ClearMissionStatus(); return; }
	bHasStatus = true; SelectActiveAliases(); Refresh();
}

bool UDCMissionObjectiveWidget::ApplyPage(int32 Index, int32 PhaseIndex, const FText& Title,
	const TArray<FDCMissionProgress>& Progress, const FDCMissionHUDStyle& Style)
{
	FDCMissionBodyPage& Page = Pages[Index];
	Page.PhaseIndex = PhaseIndex; Page.Title = Title; Page.Rows = Progress; Page.Style = Style;
	SynchronizePage(Index);
	const UWidgetTree* Design = UDCMissionEntryWidget::GetDesignerTree(Style.EntryWidgetClass);
	const float Width = GetContentWidth();
	TArray<float> Heights;
	for (const FDCMissionProgress& Row : Progress)
	{
		const float Height = UDCMissionEntryWidget::MeasureRowHeight(Row, Design, Page.MeasurementText, Width);
		if (!FMath::IsFinite(Height) || Height <= 0.f) { return false; }
		Heights.Add(Height);
	}
	TMap<FName, UDCMissionEntryData*> Previous;
	for (UDCMissionEntryData* Item : Page.Items) { Previous.Add(Item->GetProgress().MissionId, Item); }
	TArray<TObjectPtr<UDCMissionEntryData>> Next;
	for (int32 RowIndex = 0; RowIndex < Progress.Num(); ++RowIndex)
	{
		const FDCMissionProgress& Row = Progress[RowIndex];
		UDCMissionEntryData* Item = Previous.FindRef(Row.MissionId);
		if (!Item) { Item = NewObject<UDCMissionEntryData>(this, NAME_None, RF_Transient); }
		Item->Update(Row, Width, Heights[RowIndex]);
		Item->SetPaintContext(GetPathNameSafe(SourceDirector.Get()), PhaseIndex, Index, TransitionSerial);
		Next.Add(Item);
	}
	const bool bChanged = Next != Page.Items; Page.Items = MoveTemp(Next);
	SynchronizePage(Index);
	if (bChanged && Page.List) { Page.List->SetListItems(Page.Items); }
	return true;
}

void UDCMissionObjectiveWidget::SynchronizePage(int32 Index)
{
	FDCMissionBodyPage& Page = Pages[Index];
	if (!Page.Root || !Page.ListBox || !Page.PhaseText || !Page.List || !IsEntryClassUsable(Page.Style.EntryWidgetClass)) { return; }
	if (!Page.MeasurementText || Page.MeasuredEntryClass != Page.Style.EntryWidgetClass)
	{
		RemoveList(Index);
		Page.List->Configure(Page.Style.EntryWidgetClass);
		Page.MeasurementText = UDCMissionEntryWidget::CreateMeasurementText(
			UDCMissionEntryWidget::GetDesignerTree(Page.Style.EntryWidgetClass), this);
		Page.MeasuredEntryClass = Page.Style.EntryWidgetClass;
		Page.List->SetListItems(Page.Items);
	}
	const float Width = GetContentWidth();
	Page.PhaseText->SetText(Page.Title); Page.PhaseText->SetWrapTextAt(Width);
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
	if (Page.List) { Page.List->ClearListItems(); }
	if (Page.MeasurementText) { Page.MeasurementText->ReleaseSlateResources(true); }
	Page.MeasurementText = nullptr; Page.MeasuredEntryClass = nullptr;
	// Designer-owned lists and both page roots stay attached for their full lifetime.
}

void UDCMissionObjectiveWidget::ClearPage(int32 Index)
{
	RemoveList(Index); FDCMissionBodyPage& Page = Pages[Index];
	Page.Items.Reset(); Page.Rows.Reset(); Page.Title = FText::GetEmpty(); Page.PhaseIndex = INDEX_NONE;
	Page.Style = FDCMissionHUDStyle();
	if (Page.PhaseText) { Page.PhaseText->SetText(FText::GetEmpty()); }
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
	if (MissionBodySwitcher) { ActivePage = MissionBodySwitcher->GetDisplayedPageIndex(); MissionBodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	ClearPage(1 - ActivePage);
	State = EBodyState::Holding;
	const FDCMissionHUDStyle Style = ResolveStyle(Snapshot.HUDStyle);
	if (!bHasStatus) { Appearance = Style; }
	if (!ApplyPage(ActivePage, Snapshot.PhaseIndex, Snapshot.PhaseTitle, Snapshot.Missions, Style)) { ClearMissionStatus(); return; }
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
	FrozenPanelHeight = FMath::Clamp(RequiredContentHeight, MissionPanelSize->GetMinDesiredHeight(), MissionPanelSize->GetMaxDesiredHeight());
	State = EBodyState::FadingOut; IncomingPage = 1 - ActivePage;
	if (!MissionBodySwitcher || !MissionBodySwitcher->FadeToPage(IncomingPage, Pages[ActivePage].Style.MissionTransitionDuration, Serial))
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
	if (MissionBodySwitcher) { ActivePage = MissionBodySwitcher->GetDisplayedPageIndex(); MissionBodySwitcher->SnapToPage(ActivePage, TransitionSerial); }
	bHasStatus = false; bContentOverflow = false; RequiredContentHeight = 0;
	for (int32 Index = 0; Index < 2; ++Index) { ClearPage(Index); }
	SourceDirector.Reset(); DisplayedMissions.Reset(); CurrentStageTitle = FText::GetEmpty(); SelectActiveAliases();
	if (MissionPanel) { MissionPanel->SetVisibility(ESlateVisibility::Collapsed); }
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
	if (!bDesignerTreeReady || !HasValidDesignerLayout()) { return; }
	if (!bHasStatus) { MissionPanel->SetVisibility(ESlateVisibility::Collapsed); return; }
	SelectActiveAliases();
	const FMargin PanelPadding = MissionPanel->GetPadding();
	const float HeaderHeight = MissionHeaderSpace->GetHeightOverride();
	const float PhaseHeight = GetPhaseHeight(ActivePage);
	float RowsHeight = 0.f;
	for (const UDCMissionEntryData* Item : Pages[ActivePage].Items) { RowsHeight += Item->GetRowHeight(); }
	RequiredContentHeight = PanelPadding.Top + PanelPadding.Bottom + HeaderHeight + PhaseHeight + RowsHeight;
	const float Minimum = MissionPanelSize->GetMinDesiredHeight(), Maximum = MissionPanelSize->GetMaxDesiredHeight();
	const bool bWasOverflow = bContentOverflow;
	bContentOverflow = RequiredContentHeight > Maximum + KINDA_SMALL_NUMBER;
	if (bContentOverflow && !bWasOverflow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mission HUD content needs %.1f but Designer Max Desired Height is %.1f. Shorten/split the configured mission list; passive HUD does not scroll."),
			RequiredContentHeight, Maximum);
	}
	const float PanelHeight = State == EBodyState::FadingOut || State == EBodyState::FadingIn
		? FrozenPanelHeight : FMath::Clamp(RequiredContentHeight, Minimum, Maximum);
	// Only content-dependent dimensions change; anchors, offsets, fonts and brushes stay authored.
	MissionPanelSize->SetHeightOverride(PanelHeight);
	if (ListBox) { ListBox->SetHeightOverride(FMath::Max(0.f, PanelHeight - PanelPadding.Top - PanelPadding.Bottom - HeaderHeight - PhaseHeight)); }
	MissionPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCMissionObjectiveWidget::NativeDestruct()
{
	ClearMissionStatus(); RequestLatestLive.Unbind();
	// The owned switcher may keep its Slate surface in a pool. ClearMissionStatus
	// cancels the transition; keep the weak self callbacks for reentry without a rebuild.
	if (MissionPanelSize) { MissionPanelSize->ClearHeightOverride(); }
	Super::NativeDestruct();
}

int32 UDCMissionObjectiveWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	static const bool bDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDPaintDiagnostics"));
	if (!bDiagnostics || GFrameCounter % 8 != 0 || !bHasStatus || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !MissionPanel || !MissionList || !MissionPanel->IsVisible()) { return Layer; }
	const FGeometry PanelGeometry = MissionPanel->GetPaintSpaceGeometry(), ListGeometry = MissionList->GetPaintSpaceGeometry();
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
