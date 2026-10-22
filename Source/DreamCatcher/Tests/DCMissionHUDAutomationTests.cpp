// Transient UI checks. Encounter/stage tests own progress and completion rules.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/Border.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/Stage/DCMissionEntryWidget.h"
#include "UI/Stage/DCMissionObjectiveWidget.h"
#include "UI/Stage/DCMissionTransitionSwitcher.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"
#include <limits>

struct FDCMissionHUDTestAccess
{
	static void Bind(UDCMissionEntryWidget* Row, UObject* Item) { Row->NativeOnListItemObjectSet(Item); }
	static void Release(UDCMissionEntryWidget* Row) { Row->NativeOnEntryReleased(); }
	static void Destruct(UDCMissionEntryWidget* Row) { Row->NativeDestruct(); }
	static UTextBlock* Label(const UDCMissionEntryWidget* Row) { return Row->Label; }
	static const USizeBox* CheckboxBox(const UDCMissionEntryWidget* Row) { return Row->CheckboxBox; }
	static const UImage* Checkbox(const UDCMissionEntryWidget* Row) { return Row->Checkbox; }
	static float CheckboxGap(const UDCMissionEntryWidget* Row)
	{ return CastChecked<UHorizontalBoxSlot>(Row->CheckboxBox->Slot)->GetPadding().Right; }
	static const UTextBlock* StageTitle(const UDCMissionObjectiveWidget* W) { return W->PhaseText; }
	static FDCMissionHUDStyle Resolve(const FDCMissionHUDStyle& Source) { return UDCMissionObjectiveWidget::ResolveStyle(Source); }
	static bool HasList(const UDCMissionObjectiveWidget* Panel) { return Panel->MissionList != nullptr; }
	static bool PanelCollapsed(const UDCMissionObjectiveWidget* Panel) { return Panel->Panel && Panel->Panel->GetVisibility() == ESlateVisibility::Collapsed; }
	static void Midpoint(UDCMissionObjectiveWidget* W, int64 Serial)
	{
		// Deliver an explicit engine midpoint for state-machine unit checks;
		// world ticks do not advance CommonUI's Slate animation clock.
		if (W->BodySwitcher) { W->BodySwitcher->SnapToPage(W->IncomingPage, Serial); }
		W->HandleMidpoint(Serial, W->IncomingPage);
	}
	static void StaleMidpoint(UDCMissionObjectiveWidget* W, int64 Serial) { W->HandleMidpoint(Serial, 1 - W->ActivePage); }
	static void Finish(UDCMissionObjectiveWidget* W, int64 Serial) { W->HandleFinished(Serial); }
	static UDCMissionEntryData* Item(UDCMissionObjectiveWidget* W, FName Id)
	{
		for (UDCMissionEntryData* Value : W->Pages[W->ActivePage].Items)
		{ if (Value->GetProgress().MissionId == Id) { return Value; } }
		return nullptr;
	}
	static int32 Slots(UDCMissionObjectiveWidget* W) { return W->BodySwitcher->GetChildrenCount(); }
	static UWidget* Page(UDCMissionObjectiveWidget* W, int32 Index) { return W->Pages[Index].Root; }
	static bool HoldTimer(UDCMissionObjectiveWidget* W)
	{ return W->GetWorld()->GetTimerManager().TimerExists(W->HoldTimer); }
	static void Destruct(UDCMissionObjectiveWidget* W) { W->NativeDestruct(); }
	static uint64 CancelGeneration(UDCMissionTransitionSwitcher* W) { return W->CancelGeneration; }
	static bool Passive(const UDCMissionListView* List)
	{
		return List->SelectionMode == ESelectionMode::None && List->ConsumeMouseWheel == EConsumeMouseWheel::Never
			&& !List->bIsFocusable && !List->bEnableProximateEntryNavigation && !List->bSelectItemOnNavigation
			&& !List->bReturnFocusToSelection && !List->bEnableRightClickScrolling && !List->bEnableTouchScrolling
			&& !List->bIsPointerScrollingEnabled && !List->bIsGamepadScrollingEnabled
			&& !List->bAllowDragging && !List->bAllowDragDrop && !List->AllowOverscroll && List->WheelScrollMultiplier == 0.f;
	}
};

namespace DCMissionHUDTests
{
bool Context(FAutomationTestBase& Test)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCMissionHUDIsolatedAutomation")))
	{
		Test.AddError(TEXT("Use a separate unattended editor with -DCMissionHUDIsolatedAutomation.")); return false;
	}
	return true;
}

FDCMissionProgress Progress(FName Id, const TCHAR* Text, EDCMissionProgressSource Source)
{
	FDCMissionProgress P; P.MissionId = Id; P.DisplayText = FText::FromString(Text); P.ProgressSource = Source;
	P.CurrentCount = 2; P.TargetCount = 5; return P;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionRowLifecycleTest, "DreamCatcher.UI.MissionHUD.PooledEntryLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionRowLifecycleTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	TStrongObjectPtr<UDCMissionEntryData> First(NewObject<UDCMissionEntryData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCMissionEntryData> Second(NewObject<UDCMissionEntryData>(GetTransientPackage(), NAME_None, RF_Transient));
	FDCMissionHUDStyle Style;
	FDCMissionProgress A = DCMissionHUDTests::Progress(TEXT("Defeat"), TEXT("악몽 처치하기"), EDCMissionProgressSource::CurrentEncounterDefeats);
	FDCMissionProgress B = DCMissionHUDTests::Progress(TEXT("Guide"), TEXT("광장구역으로 이동하기"), EDCMissionProgressSource::Guidance);
	First->Update(A, Style); Second->Update(B, Style);
	TStrongObjectPtr<UDCMissionEntryWidget> Row(NewObject<UDCMissionEntryWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	Row->Initialize(); TSharedPtr<SWidget> Slate = Row->TakeWidget();
	FDCMissionHUDTestAccess::Bind(Row.Get(), First.Get());
	TestEqual(TEXT("Bound defeat identity"), Row->GetDisplayedMissionId(), FName(TEXT("Defeat")));
	TestTrue(TEXT("Actual count formatting"), Row->GetDisplayedText().ToString().StartsWith(TEXT("2/5")));
	A.CurrentCount = 3; First->Update(A, Style);
	TestTrue(TEXT("Existing item updates its bound row"), Row->GetDisplayedText().ToString().StartsWith(TEXT("3/5")));
	FDCMissionHUDTestAccess::Bind(Row.Get(), Second.Get());
	TestEqual(TEXT("Guidance has no synthetic count"), Row->GetDisplayedText().ToString(), B.DisplayText.ToString());
	A.CurrentCount = 4; First->Update(A, Style);
	TestEqual(TEXT("Previous item cannot overwrite a reused row"), Row->GetDisplayedMissionId(), B.MissionId);
	TestEqual(TEXT("Previous item's subscription was removed"), Row->GetDisplayedText().ToString(), B.DisplayText.ToString());
	B.DisplayText = FText::FromString(TEXT("수정한 목표 지점")); Second->Update(B, Style);
	TestEqual(TEXT("New item remains subscribed"), Row->GetDisplayedText().ToString(), B.DisplayText.ToString());
	FDCMissionHUDTestAccess::Release(Row.Get());
	TestTrue(TEXT("Pool release clears text"), Row->GetDisplayedText().IsEmpty());
	TestTrue(TEXT("Pool release clears mission identity"), Row->GetDisplayedMissionId().IsNone());
	B.DisplayText = FText::FromString(TEXT("숨겨진 이전 값")); Second->Update(B, Style);
	TestTrue(TEXT("Released item no longer updates the row"), Row->GetDisplayedText().IsEmpty());
	FDCMissionHUDTestAccess::Bind(Row.Get(), First.Get()); FDCMissionHUDTestAccess::Destruct(Row.Get());
	A.CurrentCount = 5; First->Update(A, Style);
	TestTrue(TEXT("Destruct removes the last subscription"), Row->GetDisplayedMissionId().IsNone() && Row->GetDisplayedText().IsEmpty());
	Slate.Reset(); Row->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionHUDSafetyTest, "DreamCatcher.UI.MissionHUD.ConfigurationSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionHUDSafetyTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	TestFalse(TEXT("Missing entry class is rejected before building a list"), UDCMissionObjectiveWidget::IsEntryClassUsable(nullptr));
	TestFalse(TEXT("Native dummy fallback cannot evade the engine BP requirement"), UDCMissionObjectiveWidget::IsEntryClassUsable(UDCMissionEntryWidget::StaticClass()));
	FDCMissionHUDStyle Bad;
	Bad.PanelWidth = std::numeric_limits<float>::quiet_NaN(); Bad.FontSize = 0;
	Bad.EntryWidgetClass = UDCMissionEntryWidget::StaticClass();
	TStrongObjectPtr<UTexture2D> Art(NewObject<UTexture2D>(GetTransientPackage(), NAME_None, RF_Transient));
	Bad.PanelBrush.SetResourceObject(Art.Get()); Bad.HeaderIconBrush.SetResourceObject(Art.Get());
	Bad.CheckboxBrush.SetResourceObject(Art.Get()); Bad.CompletedBrush.SetResourceObject(Art.Get());
	const FDCMissionHUDStyle Safe = FDCMissionHUDTestAccess::Resolve(Bad);
	TestTrue(TEXT("Bad geometry and text settings receive valid defaults"), Safe.IsValid());
	TestEqual(TEXT("Default width"), Safe.PanelWidth, 360.f); TestEqual(TEXT("Default font"), Safe.FontSize, 18);
	TestEqual(TEXT("Existing checkbox size"), Safe.CheckboxSize, 24.f);
	TestEqual(TEXT("Existing checkbox gap"), Safe.CheckboxTextGap, 16.f);
	TestTrue(TEXT("Existing font units and stage title remain the defaults"), !Safe.bFontSizeInPixels && Safe.bShowStageTitle);
	TestEqual(TEXT("Existing default font point size is unchanged"), Safe.GetMissionFont().Size, 18.f);
	TestTrue(TEXT("Fallback preserves actual selected class"), Safe.EntryWidgetClass == Bad.EntryWidgetClass);
	TestTrue(TEXT("Fallback preserves all original brush resources"), Safe.PanelBrush.GetResourceObject() == Art.Get()
		&& Safe.HeaderIconBrush.GetResourceObject() == Art.Get() && Safe.CheckboxBrush.GetResourceObject() == Art.Get()
		&& Safe.CompletedBrush.GetResourceObject() == Art.Get());
	TestTrue(TEXT("Authoring data is not repaired in place"), FMath::IsNaN(Bad.PanelWidth) && Bad.FontSize == 0);
	TStrongObjectPtr<UDCMissionObjectiveWidget> Panel(NewObject<UDCMissionObjectiveWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	Panel->Initialize(); TSharedPtr<SWidget> Slate = Panel->TakeWidget();
	TestFalse(TEXT("First rebuild does not cache a null-class list"), FDCMissionHUDTestAccess::HasList(Panel.Get()));
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>(GetTransientPackage(), NAME_None, RF_Transient));
	Data->HUDStyle.EntryWidgetClass = UDCMissionEntryWidget::StaticClass();
	AddExpectedError(TEXT("Mission HUD requires unique mission IDs"), EAutomationExpectedErrorFlags::Contains, 1);
	Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {});
	TestTrue(TEXT("Invalid class safely hides only presentation"), FDCMissionHUDTestAccess::PanelCollapsed(Panel.Get()));
	TestFalse(TEXT("Invalid class never builds a list"), FDCMissionHUDTestAccess::HasList(Panel.Get()));
	TestEqual(TEXT("Definition asset is not modified"), Data->Missions.Num(), 0);
	Panel->ClearMissionStatus(); Slate.Reset(); Panel->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionHUDWrapTest, "DreamCatcher.UI.MissionHUD.WrappingAndPassiveInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionHUDWrapTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	FDCMissionHUDStyle Style;
	const FDCMissionProgress Short = DCMissionHUDTests::Progress(TEXT("Guide"), TEXT("광장구역으로 이동하기"), EDCMissionProgressSource::Guidance);
	FDCMissionProgress Long = Short;
	Long.DisplayText = FText::FromString(TEXT("광장구역으로 이동하고 주변을 탐색해서 다음 목표에 관한 단서를 찾은 다음 안전한 목표 지점으로 이동하기"));
	const float ShortHeight = UDCMissionEntryWidget::MeasureRowHeight(Short, Style);
	const float LongHeight = UDCMissionEntryWidget::MeasureRowHeight(Long, Style);
	TestTrue(TEXT("Short rows reserve their configured minimum"), ShortHeight >= Style.RowMinHeight);
	TestTrue(TEXT("Korean long text expands before overflow detection"), LongHeight > ShortHeight);
	FDCMissionHUDStyle Narrow = Style; Narrow.PanelWidth = 240;
	TestTrue(TEXT("Narrower rows retain all wrapped lines"), UDCMissionEntryWidget::MeasureRowHeight(Long, Narrow) >= LongHeight);
	TStrongObjectPtr<UDCMissionListView> List(NewObject<UDCMissionListView>(GetTransientPackage(), NAME_None, RF_Transient));
	TestTrue(TEXT("Mission list consumes no HUD controls"), FDCMissionHUDTestAccess::Passive(List.Get()));
	TestTrue(TEXT("List subtree is excluded from hit testing"), List->GetVisibility() == ESlateVisibility::HitTestInvisible);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionArtTypographyTest, "DreamCatcher.UI.MissionHUD.ArtTypographyAndLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionArtTypographyTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	FDCMissionHUDStyle Style;
	Style.MissionFont = LoadObject<UFont>(nullptr, TEXT("/Game/DreamCatcher/UI/Font/Paperlogy-6SemiBold_Font.Paperlogy-6SemiBold_Font"));
	if (!TestNotNull(TEXT("Selected Paperlogy family exists without creating or saving it"), Style.MissionFont.Get())) { return false; }
	Style.FontSize = 16; Style.bFontSizeInPixels = true; Style.bShowStageTitle = false;
	Style.PanelWidth = 373.f; Style.MinHeight = 205.f; Style.MaxHeight = 392.f;
	Style.Padding = 21.f; Style.HeaderHeight = 43.f; Style.RowMinHeight = 40.f;
	Style.CheckboxSize = 20.f; Style.CheckboxTextGap = 15.f; Style.bHeaderIncludedInPanel = true;
	Style.CheckboxBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/DreamCatcher/Screens/InGame_UI/Check_False.Check_False")));
	Style.CompletedBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/DreamCatcher/Screens/InGame_UI/Check_True.Check_True")));
	if (!TestNotNull(TEXT("Unchecked source image"), Style.CheckboxBrush.GetResourceObject())
		|| !TestNotNull(TEXT("Completed source image"), Style.CompletedBrush.GetResourceObject())) { return false; }
	TestTrue(TEXT("Approved art geometry is valid"), Style.IsValid());
	const FSlateFontInfo ResolvedFont = Style.GetMissionFont();
	TestEqual(TEXT("Design pixel 16 resolves to native point 12 at the fixed Slate render DPI"), ResolvedFont.Size, 12.f);
	TestTrue(TEXT("Resolver uses the selected real font"), ResolvedFont.FontObject == Style.MissionFont.Get());
	FDCMissionHUDStyle Invalid = Style; Invalid.CheckboxSize = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Non-finite checkbox size is invalid"), Invalid.IsValid());
	const FDCMissionHUDStyle Safe = FDCMissionHUDTestAccess::Resolve(Invalid);
	TestTrue(TEXT("Fallback keeps the selected family and presentation options"), Safe.MissionFont == Style.MissionFont
		&& Safe.bFontSizeInPixels && !Safe.bShowStageTitle && Safe.CheckboxSize == 24.f && Safe.CheckboxTextGap == 16.f);
	Invalid = Style; Invalid.CheckboxTextGap = -1.f;
	TestFalse(TEXT("Negative checkbox gap is invalid"), Invalid.IsValid());
	Invalid = Style; Invalid.PanelWidth = 2.f * Invalid.Padding + Invalid.CheckboxSize + Invalid.CheckboxTextGap;
	TestFalse(TEXT("A non-positive remaining text width is invalid"), Invalid.IsValid());
	FDCMissionProgress Progress = DCMissionHUDTests::Progress(TEXT("Defeat"), TEXT("악몽 처치하기"), EDCMissionProgressSource::CurrentEncounterDefeats);
	TStrongObjectPtr<UDCMissionEntryData> Item(NewObject<UDCMissionEntryData>(GetTransientPackage(), NAME_None, RF_Transient));
	Item->Update(Progress, Style);
	TStrongObjectPtr<UDCMissionEntryWidget> Row(NewObject<UDCMissionEntryWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	Row->Initialize(); TSharedPtr<SWidget> RowSlate = Row->TakeWidget(); FDCMissionHUDTestAccess::Bind(Row.Get(), Item.Get());
	UTextBlock* Label = FDCMissionHUDTestAccess::Label(Row.Get());
	TestTrue(TEXT("Actual label uses the same complete font info as measurement"), Label->GetFont() == ResolvedFont);
	TestEqual(TEXT("Actual checkbox width"), FDCMissionHUDTestAccess::CheckboxBox(Row.Get())->GetWidthOverride(), 20.f);
	TestEqual(TEXT("Actual checkbox height"), FDCMissionHUDTestAccess::CheckboxBox(Row.Get())->GetHeightOverride(), 20.f);
	TestEqual(TEXT("Actual checkbox text gap"), FDCMissionHUDTestAccess::CheckboxGap(Row.Get()), 15.f);
	TestTrue(TEXT("Unfinished row uses Check_False"), FDCMissionHUDTestAccess::Checkbox(Row.Get())->GetBrush().GetResourceObject() == Style.CheckboxBrush.GetResourceObject());
	TestEqual(TEXT("A short row retains the approved 40-unit pitch"), Item->GetRowHeight(), 40.f);
	Progress.bCompleted = true; Progress.CurrentCount = Progress.TargetCount; Item->Update(Progress, Style);
	TestTrue(TEXT("Completed row uses Check_True while retaining actual progress formatting"),
		FDCMissionHUDTestAccess::Checkbox(Row.Get())->GetBrush().GetResourceObject() == Style.CompletedBrush.GetResourceObject()
		&& Row->GetDisplayedText().ToString().Contains(TEXT("· 완료")));
	Progress.DisplayText = FText::FromString(TEXT("광장구역으로 이동하고 주변을 탐색해서 다음 목표에 관한 단서를 찾은 다음 안전한 목표 지점으로 이동하기"));
	Item->Update(Progress, Style); Row->ForceLayoutPrepass();
	const float TextWidth = Style.PanelWidth - 2.f * Style.Padding - Style.CheckboxSize - Style.CheckboxTextGap;
	const float MeasuredHeight = UDCMissionEntryWidget::MeasureTextHeight(Row->GetDisplayedText(), ResolvedFont, TextWidth);
	// This detached fixture has no viewport prepass. Refresh the actual label's
	// cached Slate widget explicitly before comparing it with the independent measure.
	TSharedRef<SWidget> LabelSlate = Label->TakeWidget();
	LabelSlate->Invalidate(EInvalidateWidgetReason::Layout); LabelSlate->SlatePrepass(1.f);
	const float ActualHeight = Label->GetDesiredSize().Y;
	TestTrue(TEXT("Long Korean text wraps with the selected family before overflow detection"), Item->GetRowHeight() > Style.RowMinHeight);
	TestTrue(FString::Printf(TEXT("Actual label and common measurement agree on wrapped height (actual=%.3f, expected=%.3f, width=%.3f, font=%.3f)"),
		ActualHeight, MeasuredHeight, TextWidth, ResolvedFont.Size), FMath::IsNearlyEqual(ActualHeight, MeasuredHeight, 1.f));
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>(GetTransientPackage(), NAME_None, RF_Transient));
	Style.EntryWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/DreamCatcher/UI/HUD/W_DC_MissionEntry.W_DC_MissionEntry_C"));
	if (!TestTrue(TEXT("Existing compiled entry BP"), UDCMissionObjectiveWidget::IsEntryClassUsable(Style.EntryWidgetClass))) { return false; }
	Data->HUDStyle = Style;
	FDCMissionProgress Short = DCMissionHUDTests::Progress(TEXT("Guide"), TEXT("광장구역으로 이동하기"), EDCMissionProgressSource::Guidance);
	Progress.DisplayText = FText::FromString(TEXT("악몽 처치하기")); Progress.bCompleted = false;
	TStrongObjectPtr<UDCMissionObjectiveWidget> Panel(NewObject<UDCMissionObjectiveWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	Panel->Initialize(); Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {Progress, Short});
	TSharedPtr<SWidget> PanelSlate = Panel->TakeWidget();
	TestEqual(TEXT("Hidden stage title is actually collapsed"), FDCMissionHUDTestAccess::StageTitle(Panel.Get())->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Hidden title reserves neither text height nor its eight-unit gap"), Panel->GetRequiredContentHeight(), 165.f);
	Data->HUDStyle.bShowStageTitle = true;
	Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {Progress, Short});
	TestTrue(TEXT("Other styles can still show their stage title"), FDCMissionHUDTestAccess::StageTitle(Panel.Get())->GetVisibility() == ESlateVisibility::HitTestInvisible
		&& Panel->GetRequiredContentHeight() > 165.f);
	FDCMissionHUDTestAccess::Destruct(Panel.Get()); PanelSlate.Reset(); Panel->ReleaseSlateResources(true);
	FDCMissionHUDTestAccess::Destruct(Row.Get()); RowSlate.Reset(); Row->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

#if WITH_SERVER_CODE
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCStageFlowTestTypes.h"

namespace DCMissionInPanelTests
{
struct FFixture
{
	FTestWorldWrapper World;
	TStrongObjectPtr<UDCMissionListData> Data { NewObject<UDCMissionListData>(GetTransientPackage(), NAME_None, RF_Transient) };
	bool Start(FAutomationTestBase& Test)
	{
		if (!DCMissionHUDTests::Context(Test)) { return false; }
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR11ModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		Data->HUDStyle.EntryWidgetClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/DreamCatcher/UI/HUD/W_DC_MissionEntry.W_DC_MissionEntry_C"));
		if (!Test.TestTrue(TEXT("Existing real BP entry is available, without generating/saving an asset"),
			UDCMissionObjectiveWidget::IsEntryClassUsable(Data->HUDStyle.EntryWidgetClass))) { return false; }
		FDCMissionDefinition Defeat; Defeat.MissionId = TEXT("Defeat"); Defeat.DisplayText = FText::FromString(TEXT("악몽 처치하기"));
		Defeat.ProgressSource = EDCMissionProgressSource::CurrentEncounterDefeats;
		FDCMissionDefinition Guide; Guide.MissionId = TEXT("Guide"); Guide.DisplayText = FText::FromString(TEXT("광장으로 이동하기"));
		Guide.TargetKey = TEXT("Goal");
		FDCMissionDefinition External; External.MissionId = TEXT("External"); External.DisplayText = FText::FromString(TEXT("단서 수집하기"));
		External.ProgressSource = EDCMissionProgressSource::External; External.TargetCount = 5;
		Data->Missions = {Defeat, Guide, External}; return true;
	}
	ADCR11DirectorFixture* Source(bool bAdvance)
	{
		ADCR11DirectorFixture* Value = World.GetTestWorld()->SpawnActor<ADCR11DirectorFixture>();
		if (!Value) { return nullptr; }
		FDCStagePhaseDefinition First; First.PhaseType = EDCStagePhaseType::Delay;
		First.PhaseDelay = bAdvance ? .05f : 60.f; First.MissionList = Data.Get(); First.PhaseTitle = FText::FromString(TEXT("First"));
		FDCStagePhaseDefinition Next = First; Next.PhaseDelay = 60.f; Next.PhaseTitle = FText::FromString(TEXT("Next"));
		Value->Configure({First, Next}); Value->StartStage(); return Value;
	}
	FDCMissionTransitionSnapshot Completed(ADCR11DirectorFixture* Source)
	{
		// This is a controlled UI input. The separate core tests prove real deaths
		// create the complete ordered snapshot before the original next-tick advance.
		FDCMissionTransitionSnapshot Value; Value.PhaseIndex = Source->GetCurrentPhaseIndex();
		Value.PhaseTitle = FText::FromString(TEXT("Completed phase")); Value.HUDStyle = Data->HUDStyle;
		Value.Missions = Source->GetMissionProgress();
		for (FDCMissionProgress& Row : Value.Missions)
		{ if (Row.ProgressSource == EDCMissionProgressSource::CurrentEncounterDefeats) { Row.CurrentCount = Row.TargetCount = 1; Row.bCompleted = true; } }
		return Value;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionInPanelHoldTest, "DreamCatcher.UI.MissionHUD.InPanelHoldAndLatestMidpoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionInPanelHoldTest::RunTest(const FString&)
{
	DCMissionInPanelTests::FFixture F; if (!F.Start(*this)) { return false; }
	ADCR11DirectorFixture* Source = F.Source(true);
	if (!TestNotNull(TEXT("Source director"), Source)) { return false; }
	TStrongObjectPtr<UDCMissionObjectiveWidget> W(NewObject<UDCMissionObjectiveWidget>(F.World.GetTestWorld(), NAME_None, RF_Transient));
	W->Initialize(); W->SetLiveMissionStatus(Source, 0, F.Data.Get(), FText::FromString(TEXT("First")), Source->GetMissionProgress());
	TSharedPtr<SWidget> Slate = W->TakeWidget();
	UWidget* FirstPage = FDCMissionHUDTestAccess::Page(W.Get(), 0); UWidget* SecondPage = FDCMissionHUDTestAccess::Page(W.Get(), 1);
	const FDCMissionTransitionSnapshot Snapshot = F.Completed(Source);
	TestEqual(TEXT("Approved completed hold"), Snapshot.HUDStyle.CompletedHoldDuration, .6f);
	TestEqual(TEXT("Original CommonUI total fade duration"), Snapshot.HUDStyle.MissionTransitionDuration, .4f);
	W->ShowCompletedPhase(Source, Snapshot); const int64 Serial = W->GetAnimationSerial();
	TestEqual(TEXT("Completed body begins holding"), W->GetBodyTransitionState(), FName(TEXT("holding")));
	TestTrue(TEXT("Completed count has an actual completion suffix"), UDCMissionEntryWidget::FormatProgress(W->GetDisplayedMissions()[0]).ToString().Contains(TEXT("· 완료")));
	TestFalse(TEXT("Guidance retains its unfinished state"), W->GetDisplayedMissions()[1].bCompleted);
	TestEqual(TEXT("Guidance wording is unchanged"), UDCMissionEntryWidget::FormatProgress(W->GetDisplayedMissions()[1]).ToString(), Snapshot.Missions[1].DisplayText.ToString());
	// Activate pending timers, then use sub-0.4 deltas (WorldSettings clamps larger deltas).
	F.World.TickTestWorld(.01f);
	for (int32 Tick = 0; Tick < 5; ++Tick) { F.World.TickTestWorld(.1f); }
	TestEqual(TEXT("Actual stage advanced while old body holds"), Source->GetCurrentPhaseIndex(), 1);
	Source->UpdateExternalMissionProgress(TEXT("External"), 2);
	W->SetLiveMissionStatus(Source, 1, F.Data.Get(), FText::FromString(TEXT("Earlier next label")), Source->GetMissionProgress());
	TestEqual(TEXT("Incoming poll cannot replace completed hold"), W->GetDisplayedPhaseIndex(), 0);
	TestEqual(TEXT("Still holding after half a second"), W->GetBodyTransitionState(), FName(TEXT("holding")));
	F.World.TickTestWorld(.11f);
	TestEqual(TEXT("Hold timer starts fade out"), W->GetBodyTransitionState(), FName(TEXT("fade_out")));
	Source->UpdateExternalMissionProgress(TEXT("External"), 3);
	int32 FreshReads = 0;
	W->RequestLatestLive.BindLambda([&]
	{
		++FreshReads;
		W->SetLiveMissionStatus(Source, Source->GetCurrentPhaseIndex(), Source->GetCurrentMissionList(),
			FText::FromString(TEXT("Latest midpoint label")), Source->GetMissionProgress());
	});
	FDCMissionHUDTestAccess::Midpoint(W.Get(), Serial);
	TestEqual(TEXT("Midpoint reads its producer once, at callback time"), FreshReads, 1);
	TestEqual(TEXT("Actual new phase is selected"), W->GetDisplayedPhaseIndex(), 1);
	UDCMissionEntryData* MidpointItem = FDCMissionHUDTestAccess::Item(W.Get(), TEXT("External"));
	if (!TestNotNull(TEXT("Latest external row"), MidpointItem)) { return false; }
	TestEqual(TEXT("Midpoint uses latest external count, not cached 0/1"), MidpointItem->GetProgress().CurrentCount, 3);
	TestEqual(TEXT("CommonUI intro is represented separately"), W->GetBodyTransitionState(), FName(TEXT("fade_in")));
	FDCMissionHUDTestAccess::Finish(W.Get(), Serial);
	UDCMissionEntryData* Existing = FDCMissionHUDTestAccess::Item(W.Get(), TEXT("External"));
	Source->UpdateExternalMissionProgress(TEXT("External"), 4);
	W->SetLiveMissionStatus(Source, 1, F.Data.Get(), FText::FromString(TEXT("Live")), Source->GetMissionProgress());
	TestTrue(TEXT("Same-phase refresh reuses item identity"), Existing == FDCMissionHUDTestAccess::Item(W.Get(), TEXT("External")));
	TestEqual(TEXT("Same-phase count updates immediately"), Existing->GetProgress().CurrentCount, 4);
	TestEqual(TEXT("Two stable slots remain"), FDCMissionHUDTestAccess::Slots(W.Get()), 2);
	TestTrue(TEXT("Stable page roots are never removed"), FirstPage == FDCMissionHUDTestAccess::Page(W.Get(), 0) && SecondPage == FDCMissionHUDTestAccess::Page(W.Get(), 1));
	TestTrue(TEXT("Whole panel remains passive"), W->GetVisibility() == ESlateVisibility::HitTestInvisible && !W->IsFocusable());
	FDCMissionHUDTestAccess::Destruct(W.Get()); Slate.Reset(); W->ReleaseSlateResources(true);
	AddInfo(TEXT("Only the hold uses world time here. Explicit midpoint/finish delivery tests state; real .2/.2 Slate fade and inherited paint alpha require PIE."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionInPanelCancelTest, "DreamCatcher.UI.MissionHUD.InPanelLatestCancelAndSourceReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionInPanelCancelTest::RunTest(const FString&)
{
	DCMissionInPanelTests::FFixture F; if (!F.Start(*this)) { return false; }
	ADCR11DirectorFixture* Source = F.Source(true);
	if (!TestNotNull(TEXT("Source director"), Source)) { return false; }
	TStrongObjectPtr<UDCMissionObjectiveWidget> W(NewObject<UDCMissionObjectiveWidget>(F.World.GetTestWorld(), NAME_None, RF_Transient));
	W->Initialize(); W->SetLiveMissionStatus(Source, 0, F.Data.Get(), FText::FromString(TEXT("First")), Source->GetMissionProgress());
	TSharedPtr<SWidget> Slate = W->TakeWidget();
	const FDCMissionTransitionSnapshot Old = F.Completed(Source);
	W->ShowCompletedPhase(Source, Old); const int64 OldSerial = W->GetAnimationSerial();
	F.World.TickTestWorld(.01f); F.World.TickTestWorld(.1f); F.World.TickTestWorld(.1f);
	if (!TestEqual(TEXT("Source advanced before first hold ends"), Source->GetCurrentPhaseIndex(), 1)) { return false; }
	const FDCMissionTransitionSnapshot Latest = F.Completed(Source);
	W->ShowCompletedPhase(Source, Latest); const int64 LatestSerial = W->GetAnimationSerial();
	TestTrue(TEXT("Latest completion replaces the old operation"), LatestSerial > OldSerial);
	TestEqual(TEXT("Latest phase is shown immediately; no FIFO"), W->GetDisplayedPhaseIndex(), 1);
	W->ShowCompletedPhase(Source, Latest);
	TestEqual(TEXT("Same-source same-phase completion is deduplicated"), W->GetAnimationSerial(), LatestSerial);
	W->ShowCompletedPhase(Source, Old);
	FDCMissionHUDTestAccess::StaleMidpoint(W.Get(), OldSerial); FDCMissionHUDTestAccess::Finish(W.Get(), OldSerial);
	TestEqual(TEXT("Old phase/event/serial cannot replace latest hold"), W->GetDisplayedPhaseIndex(), 1);
	TestEqual(TEXT("Latest hold is not finished by an old callback"), W->GetBodyTransitionState(), FName(TEXT("holding")));
	W->ClearMissionStatus();
	TestTrue(TEXT("Cancel clears hold timer, rows and state"), !FDCMissionHUDTestAccess::HoldTimer(W.Get()) && W->GetDisplayedMissions().IsEmpty()
		&& !W->IsMissionTransitionActive());
	ADCR11DirectorFixture* NewSource = F.Source(false);
	if (!TestNotNull(TEXT("New source director"), NewSource)) { return false; }
	W->SetLiveMissionStatus(NewSource, 0, F.Data.Get(), FText::FromString(TEXT("New run")), NewSource->GetMissionProgress());
	W->ShowCompletedPhase(NewSource, F.Completed(NewSource));
	ADCR11DirectorFixture* AnotherSource = F.Source(false);
	if (!TestNotNull(TEXT("Another source director"), AnotherSource)) { return false; }
	W->SetLiveMissionStatus(AnotherSource, 0, F.Data.Get(), FText::FromString(TEXT("Another run")), AnotherSource->GetMissionProgress());
	W->ShowCompletedPhase(AnotherSource, F.Completed(AnotherSource));
	TestEqual(TEXT("New director phase zero is not suppressed by previous director phase zero"), W->GetBodyTransitionState(), FName(TEXT("holding")));
	TestEqual(TEXT("Same asset/IDs remain valid in a new source context"), W->GetDisplayedPhaseIndex(), 0);
	W->ClearMissionStatus();
	FDCMissionHUDTestAccess::StaleMidpoint(W.Get(), LatestSerial);
	TestTrue(TEXT("No post-cancel callback resurrects rows"), W->GetDisplayedMissions().IsEmpty());
	TStrongObjectPtr<UDCMissionTransitionSwitcher> Switcher(NewObject<UDCMissionTransitionSwitcher>(GetTransientPackage(), NAME_None, RF_Transient));
	const uint64 Before = FDCMissionHUDTestAccess::CancelGeneration(Switcher.Get());
	Switcher->SnapToPage(0, 7); Switcher->ReleaseSlateResources(true); Switcher->SnapToPage(0, 7);
	TestTrue(TEXT("Internal cancel generation stays monotonic across serial reuse and Slate release"), FDCMissionHUDTestAccess::CancelGeneration(Switcher.Get()) >= Before + 3);
	FDCMissionHUDTestAccess::Destruct(W.Get()); Slate.Reset(); W->ReleaseSlateResources(true);
	return !HasAnyErrors();
}
#endif

#endif
