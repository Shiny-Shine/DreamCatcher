// Transient UI checks. Encounter/stage tests own progress and completion rules.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
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
	static UTextBlock* Label(const UDCMissionEntryWidget* Row) { return Row->MissionText; }
	static const USizeBox* CheckboxBox(const UDCMissionEntryWidget* Row) { return Row->MissionCheckboxBox; }
	static const UImage* Checkbox(const UDCMissionEntryWidget* Row) { return Row->MissionCheckbox; }
	static float CheckboxGap(const UDCMissionEntryWidget* Row)
	{ return CastChecked<UHorizontalBoxSlot>(Row->MissionCheckboxBox->Slot)->GetPadding().Right; }
	static const UTextBlock* StageTitle(const UDCMissionObjectiveWidget* W) { return W->PhaseText; }
	static FDCMissionHUDStyle Resolve(const FDCMissionHUDStyle& Source) { return UDCMissionObjectiveWidget::ResolveStyle(Source); }
	static bool PanelCollapsed(const UDCMissionObjectiveWidget* Panel) { return Panel->MissionPanel && Panel->MissionPanel->GetVisibility() == ESlateVisibility::Collapsed; }
	static void Midpoint(UDCMissionObjectiveWidget* W, int64 Serial)
	{
		// Deliver an explicit engine midpoint for state-machine unit checks;
		// world ticks do not advance CommonUI's Slate animation clock.
		if (W->MissionBodySwitcher) { W->MissionBodySwitcher->SnapToPage(W->IncomingPage, Serial); }
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
	static int32 Slots(UDCMissionObjectiveWidget* W) { return W->MissionBodySwitcher->GetChildrenCount(); }
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
template <typename T>
T* Child(UWidgetTree* Tree, const TCHAR* Name, UPanelWidget* Parent)
{
	T* W = Tree->ConstructWidget<T>(T::StaticClass(), Name);
	if (Parent) { Parent->AddChild(W); }
	return W;
}

void BuildRow(UDCMissionEntryWidget* Row, UFont* Font = nullptr)
{
	Row->Initialize();
	if (!Row->WidgetTree) { Row->WidgetTree = NewObject<UWidgetTree>(Row, TEXT("WidgetTree"), RF_Transient); }
	UWidgetTree* Tree = Row->WidgetTree;
	USizeBox* Root = Child<USizeBox>(Tree, TEXT("MissionRow"), nullptr); Tree->RootWidget = Root;
	Root->SetMinDesiredHeight(40);
	UHorizontalBox* Body = Child<UHorizontalBox>(Tree, TEXT("RowContents"), Root);
	USizeBox* IconBox = Child<USizeBox>(Tree, TEXT("MissionCheckboxBox"), Body);
	IconBox->SetWidthOverride(20); IconBox->SetHeightOverride(20);
	CastChecked<UHorizontalBoxSlot>(IconBox->Slot)->SetPadding(FMargin(0, 0, 15, 0));
	UImage* Icon = Child<UImage>(Tree, TEXT("MissionCheckbox"), IconBox);
	FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/DreamCatcher/Screens/InGame_UI/Check_False.Check_False")));
	Icon->SetBrush(Brush);
	Brush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/DreamCatcher/Screens/InGame_UI/Check_True.Check_True")));
	Row->CompletedBrush = Brush;
	UTextBlock* Label = Child<UTextBlock>(Tree, TEXT("MissionText"), Body);
	Label->SetFont(Font ? FSlateFontInfo(Font, 12.f, FName(TEXT("Default"))) : FCoreStyle::GetDefaultFontStyle("Regular", 12));
	Label->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(.3f, .5f, .7f, 1.f)));
	CastChecked<UHorizontalBoxSlot>(Label->Slot)->SetPadding(FMargin(0, 6, 0, 6));
	CastChecked<UHorizontalBoxSlot>(Label->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

TSubclassOf<UUserWidget> EntryClass(FAutomationTestBase& Test)
{
	TSubclassOf<UUserWidget> Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/DreamCatcher/UI/HUD/W_DC_MissionEntry.W_DC_MissionEntry_C"));
	Test.TestTrue(TEXT("The existing mission entry must be authored/compiled in Designer first"),
		UDCMissionObjectiveWidget::IsEntryClassUsable(Class));
	return Class;
}

void BuildPanel(UDCMissionObjectiveWidget* Panel, TSubclassOf<UUserWidget> Class)
{
	Panel->Initialize();
	if (!Panel->WidgetTree) { Panel->WidgetTree = NewObject<UWidgetTree>(Panel, TEXT("WidgetTree"), RF_Transient); }
	UWidgetTree* Tree = Panel->WidgetTree;
	UCanvasPanel* Root = Child<UCanvasPanel>(Tree, TEXT("MissionRoot"), nullptr); Tree->RootWidget = Root;
	USizeBox* Size = Child<USizeBox>(Tree, TEXT("MissionPanelSize"), Root);
	Size->SetWidthOverride(373); Size->SetMinDesiredHeight(205); Size->SetMaxDesiredHeight(392);
	UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Size->Slot);
	Slot->SetAnchors(FAnchors(1, 0)); Slot->SetAlignment(FVector2D(1, 0)); Slot->SetPosition(FVector2D(-64, 288)); Slot->SetAutoSize(true);
	UBorder* Border = Child<UBorder>(Tree, TEXT("MissionPanel"), Size); Border->SetPadding(FMargin(21));
	UVerticalBox* Body = Child<UVerticalBox>(Tree, TEXT("MissionContents"), Border);
	Child<USizeBox>(Tree, TEXT("MissionHeaderSpace"), Body)->SetHeightOverride(43);
	UDCMissionTransitionSwitcher* Switcher = Child<UDCMissionTransitionSwitcher>(Tree, TEXT("MissionBodySwitcher"), Body);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UVerticalBox* Page = Child<UVerticalBox>(Tree, *FString::Printf(TEXT("MissionBodyPage%d"), Index), Switcher);
		UTextBlock* Title = Child<UTextBlock>(Tree, *FString::Printf(TEXT("MissionStageTitle%d"), Index), Page);
		Title->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14)); Title->SetVisibility(ESlateVisibility::Collapsed);
		CastChecked<UVerticalBoxSlot>(Title->Slot)->SetPadding(FMargin(0, 0, 0, 8));
		USizeBox* Area = Child<USizeBox>(Tree, *FString::Printf(TEXT("MissionListArea%d"), Index), Page);
		UDCMissionListView* List = Child<UDCMissionListView>(Tree, *FString::Printf(TEXT("MissionList%d"), Index), Area);
		List->Configure(Class);
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionRowLifecycleTest, "DreamCatcher.UI.MissionHUD.PooledEntryLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionRowLifecycleTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	TStrongObjectPtr<UDCMissionEntryWidget> Row(NewObject<UDCMissionEntryWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	DCMissionHUDTests::BuildRow(Row.Get()); TSharedPtr<SWidget> Slate = Row->TakeWidget();
	TStrongObjectPtr<UDCMissionEntryData> First(NewObject<UDCMissionEntryData>(GetTransientPackage()));
	FDCMissionProgress A = DCMissionHUDTests::Progress(TEXT("First"), TEXT("첫 목표"), EDCMissionProgressSource::CurrentEncounterDefeats);
	First->Update(A, 331, 40); FDCMissionHUDTestAccess::Bind(Row.Get(), First.Get());
	TestEqual(TEXT("Initial data is visible"), Row->GetDisplayedMissionId(), A.MissionId);
	TestTrue(TEXT("Count text uses progress"), Row->GetDisplayedText().ToString().Contains(TEXT("2/5")));
	A.bCompleted = true; A.CurrentCount = 5; First->Update(A, 331, 40);
	TestTrue(TEXT("Completed entry receives authored art"), FDCMissionHUDTestAccess::Checkbox(Row.Get())->GetBrush().GetResourceObject() == Row->CompletedBrush.GetResourceObject());
	FDCMissionHUDTestAccess::Release(Row.Get());
	A.DisplayText = FText::FromString(TEXT("Old source")); First->Update(A, 331, 40);
	TestTrue(TEXT("Released row cannot receive old source updates"), Row->GetDisplayedText().IsEmpty());
	TStrongObjectPtr<UDCMissionEntryData> Second(NewObject<UDCMissionEntryData>(GetTransientPackage()));
	FDCMissionProgress B = DCMissionHUDTests::Progress(TEXT("Second"), TEXT("다음 목표"), EDCMissionProgressSource::Guidance);
	Second->Update(B, 331, 40); FDCMissionHUDTestAccess::Bind(Row.Get(), Second.Get());
	A.CurrentCount = 9; First->Update(A, 331, 40);
	TestEqual(TEXT("Pooled row owns only the new item"), Row->GetDisplayedMissionId(), B.MissionId);
	TestEqual(TEXT("Guidance remains exact text"), Row->GetDisplayedText().ToString(), B.DisplayText.ToString());
	FDCMissionHUDTestAccess::Destruct(Row.Get()); Slate.Reset(); Row->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionHUDSafetyTest, "DreamCatcher.UI.MissionHUD.ConfigurationSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionHUDSafetyTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	TestFalse(TEXT("Null class is rejected"), UDCMissionObjectiveWidget::IsEntryClassUsable(nullptr));
	TestFalse(TEXT("Native class cannot replace a saved Designer entry"), UDCMissionObjectiveWidget::IsEntryClassUsable(UDCMissionEntryWidget::StaticClass()));
	FDCMissionHUDStyle Bad; Bad.CompletedHoldDuration = std::numeric_limits<float>::quiet_NaN(); Bad.MissionTransitionDuration = 0;
	Bad.EntryWidgetClass = UDCMissionEntryWidget::StaticClass();
	const FDCMissionHUDStyle Safe = FDCMissionHUDTestAccess::Resolve(Bad);
	TestTrue(TEXT("Timing fallback is finite"), Safe.IsValid());
	TestEqual(TEXT("Hold fallback"), Safe.CompletedHoldDuration, .6f);
	TestEqual(TEXT("Fade fallback"), Safe.MissionTransitionDuration, .4f);
	TestTrue(TEXT("Class selection is preserved, not silently replaced"), Safe.EntryWidgetClass == Bad.EntryWidgetClass);
	TestTrue(TEXT("Source data is not repaired in place"), FMath::IsNaN(Bad.CompletedHoldDuration));
	const TSubclassOf<UUserWidget> Class = DCMissionHUDTests::EntryClass(*this);
	if (!UDCMissionObjectiveWidget::IsEntryClassUsable(Class)) { return false; }
	TStrongObjectPtr<UDCMissionObjectiveWidget> Panel(NewObject<UDCMissionObjectiveWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	DCMissionHUDTests::BuildPanel(Panel.Get(), Class);
	UWidget* AuthoredRoot = Panel->WidgetTree->RootWidget;
	TSharedPtr<SWidget> Slate = Panel->TakeWidget();
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>(GetTransientPackage()));
	Data->HUDStyle.EntryWidgetClass = UDCMissionEntryWidget::StaticClass();
	AddExpectedError(TEXT("Mission HUD requires unique mission IDs"), EAutomationExpectedErrorFlags::Contains, 1);
	Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {});
	TestTrue(TEXT("Invalid data hides only presentation"), FDCMissionHUDTestAccess::PanelCollapsed(Panel.Get()));
	TestTrue(TEXT("Invalid data never replaces the authored tree"), Panel->WidgetTree->RootWidget == AuthoredRoot);
	TestEqual(TEXT("Both authored pages survive"), FDCMissionHUDTestAccess::Slots(Panel.Get()), 2);
	FDCMissionHUDTestAccess::Destruct(Panel.Get()); Slate.Reset(); Panel->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionHUDWrapTest, "DreamCatcher.UI.MissionHUD.WrappingAndPassiveInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionHUDWrapTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	TStrongObjectPtr<UDCMissionEntryWidget> Row(NewObject<UDCMissionEntryWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	DCMissionHUDTests::BuildRow(Row.Get());
	TStrongObjectPtr<UTextBlock> Measure(UDCMissionEntryWidget::CreateMeasurementText(Row->WidgetTree, GetTransientPackage()));
	FDCMissionProgress Short = DCMissionHUDTests::Progress(TEXT("Guide"), TEXT("광장구역으로 이동하기"), EDCMissionProgressSource::Guidance);
	FDCMissionProgress Long = Short; Long.DisplayText = FText::FromString(TEXT("광장구역으로 이동하고 주변을 탐색해서 다음 목표에 관한 단서를 찾은 다음 안전한 목표 지점으로 이동하기"));
	const float Small = UDCMissionEntryWidget::MeasureRowHeight(Short, Row->WidgetTree, Measure.Get(), 331);
	const float Large = UDCMissionEntryWidget::MeasureRowHeight(Long, Row->WidgetTree, Measure.Get(), 331);
	TestTrue(TEXT("Designer minimum is respected"), Small >= 40);
	TestTrue(TEXT("Long Korean text expands before overflow"), Large > Small);
	TestTrue(TEXT("Narrow width does not lose lines"), UDCMissionEntryWidget::MeasureRowHeight(Long, Row->WidgetTree, Measure.Get(), 200) >= Large);
	TStrongObjectPtr<UDCMissionListView> List(NewObject<UDCMissionListView>(GetTransientPackage(), NAME_None, RF_Transient));
	TestTrue(TEXT("Existing passive list input is preserved"), FDCMissionHUDTestAccess::Passive(List.Get()));
	TestTrue(TEXT("List remains excluded from hit testing"), List->GetVisibility() == ESlateVisibility::HitTestInvisible);
	Measure->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMissionArtTypographyTest, "DreamCatcher.UI.MissionHUD.ArtTypographyAndLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCMissionArtTypographyTest::RunTest(const FString&)
{
	if (!DCMissionHUDTests::Context(*this)) { return false; }
	UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/DreamCatcher/UI/Font/Paperlogy-6SemiBold_Font.Paperlogy-6SemiBold_Font"));
	if (!TestNotNull(TEXT("Existing font family"), Font)) { return false; }
	TStrongObjectPtr<UDCMissionEntryWidget> Row(NewObject<UDCMissionEntryWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	DCMissionHUDTests::BuildRow(Row.Get(), Font); TSharedPtr<SWidget> Slate = Row->TakeWidget();
	UTextBlock* Label = FDCMissionHUDTestAccess::Label(Row.Get());
	const FSlateFontInfo AuthoredFont = Label->GetFont(); const FSlateColor AuthoredColor = Label->GetColorAndOpacity();
	TStrongObjectPtr<UTextBlock> Measure(UDCMissionEntryWidget::CreateMeasurementText(Row->WidgetTree, GetTransientPackage()));
	TestTrue(TEXT("Measurement copies complete Designer font"), Measure->GetFont() == AuthoredFont);
	TestEqual(TEXT("Approved pixel 16 is authored as point 12"), AuthoredFont.Size, 12.f);
	TStrongObjectPtr<UDCMissionEntryData> Item(NewObject<UDCMissionEntryData>(GetTransientPackage()));
	FDCMissionProgress Progress = DCMissionHUDTests::Progress(TEXT("Defeat"), TEXT("악몽 처치하기"), EDCMissionProgressSource::CurrentEncounterDefeats);
	const float Height = UDCMissionEntryWidget::MeasureRowHeight(Progress, Row->WidgetTree, Measure.Get(), 331);
	Item->Update(Progress, 331, Height); FDCMissionHUDTestAccess::Bind(Row.Get(), Item.Get());
	TestEqual(TEXT("Short row keeps the approved pitch"), Height, 40.f);
	TestTrue(TEXT("Data refresh preserves Designer font/color"), Label->GetFont() == AuthoredFont && Label->GetColorAndOpacity() == AuthoredColor);
	TestEqual(TEXT("Designer checkbox width"), FDCMissionHUDTestAccess::CheckboxBox(Row.Get())->GetWidthOverride(), 20.f);
	TestEqual(TEXT("Designer gap"), FDCMissionHUDTestAccess::CheckboxGap(Row.Get()), 15.f);
	TestEqual(TEXT("Runtime wrap uses authored icon/padding"), Label->GetWrapTextAt(), 296.f);
	const TSubclassOf<UUserWidget> Class = DCMissionHUDTests::EntryClass(*this);
	if (!UDCMissionObjectiveWidget::IsEntryClassUsable(Class)) { return false; }
	TStrongObjectPtr<UDCMissionObjectiveWidget> Panel(NewObject<UDCMissionObjectiveWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCMissionListData> Data(NewObject<UDCMissionListData>(GetTransientPackage()));
	Data->HUDStyle.EntryWidgetClass = Class;
	DCMissionHUDTests::BuildPanel(Panel.Get(), Class); TSharedPtr<SWidget> PanelSlate = Panel->TakeWidget();
	Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {Progress});
	const float HiddenTitleHeight = Panel->GetRequiredContentHeight();
	UTextBlock* Title = const_cast<UTextBlock*>(FDCMissionHUDTestAccess::StageTitle(Panel.Get()));
	TestEqual(TEXT("Designer-collapsed stage title stays collapsed"), Title->GetVisibility(), ESlateVisibility::Collapsed);
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->SetMissionStatus(Data.Get(), FText::FromString(TEXT("전투 1")), {Progress});
	TestTrue(TEXT("Designer-visible title participates in sizing"), Panel->GetRequiredContentHeight() > HiddenTitleHeight);
	const USizeBox* PanelSize = Cast<USizeBox>(Panel->GetWidgetFromName(TEXT("MissionPanelSize")));
	TestTrue(TEXT("Refresh preserves authored panel placement"), CastChecked<UCanvasPanelSlot>(PanelSize->Slot)->GetPosition().Equals(FVector2D(-64, 288)));
	FDCMissionHUDTestAccess::Destruct(Panel.Get()); PanelSlate.Reset(); Panel->ReleaseSlateResources(true);
	FDCMissionHUDTestAccess::Destruct(Row.Get()); Slate.Reset(); Row->ReleaseSlateResources(true);
	Measure->ReleaseSlateResources(true);
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
	DCMissionHUDTests::BuildPanel(W.Get(), F.Data->HUDStyle.EntryWidgetClass); W->SetLiveMissionStatus(Source, 0, F.Data.Get(), FText::FromString(TEXT("First")), Source->GetMissionProgress());
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
	DCMissionHUDTests::BuildPanel(W.Get(), F.Data->HUDStyle.EntryWidgetClass); W->SetLiveMissionStatus(Source, 0, F.Data.Get(), FText::FromString(TEXT("First")), Source->GetMissionProgress());
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
