// Transient Designer contract tests; no saved assets, gameplay attributes, viewport or PIE.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "CommonNumericTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "UI/Boss/DCBossHealthWidget.h"
#include "UI/Player/DCPlayerStatusWidget.h"
#include "UObject/StrongObjectPtr.h"

struct FDCPlayerStatusDesignerTestAccess
{
	static bool Bind(UDCPlayerStatusWidget* W) { return W->BindDesignerWidgets(); }
	static void Layout(UDCPlayerStatusWidget* W) { W->ApplyLayout(); }
	static void Release(UDCPlayerStatusWidget* W) { W->ReleaseHealthWidget(); }
	static void Ensure(UDCPlayerStatusWidget* W) { W->EnsureHealthWidget(); }
	static void Destruct(UDCPlayerStatusWidget* W) { W->NativeDestruct(); }
	static void RefreshWithoutPawn(UDCPlayerStatusWidget* W) { W->bConstructed = true; W->RefreshPlayerStatus(); W->bConstructed = false; }
	static const FDCPlayerArmorSegment& Segment(const UDCPlayerStatusWidget* W) { return W->ArmorSegments[0]; }
	static bool HasTimer(const UDCPlayerStatusWidget* W) { return W->RefreshTimer.IsValid() || W->TimerWorld.IsValid(); }
	static bool Released(const UDCPlayerStatusWidget* W) { return W->bAuthoredHealthReleased; }
};

namespace DCPlayerStatusDesignerTests
{
bool Context(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Use a separate unattended editor with -DCPlayerStatusDesignerAutomation"),
		FApp::IsUnattended() && FParse::Param(FCommandLine::Get(), TEXT("DCPlayerStatusDesignerAutomation")));
}

template <typename T>
T* Child(UWidgetTree* Tree, const TCHAR* Name, UPanelWidget* Parent)
{
	T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), Name);
	if (Parent) { Parent->AddChild(Widget); }
	return Widget;
}

struct FFixture
{
	TStrongObjectPtr<UDCPlayerStatusWidget> Status{NewObject<UDCPlayerStatusWidget>(GetTransientPackage(), NAME_None, RF_Transient)};
	UCanvasPanel* Root = nullptr;
	UCanvasPanel* Bars = nullptr;
	UTextBlock* Shield = nullptr;
	USizeBox* Host = nullptr;
	UUserWidget* Health = nullptr;
	UCommonNumericTextBlock* Number = nullptr;
	USizeBox* HealthSize = nullptr;
	UImage* Fill = nullptr;
	explicit FFixture(bool bDesignTime = true)
	{
		Status->Initialize(); Status->bUseDesignerLayout = true;
		if (bDesignTime) { Status->SetDesignerFlags(EWidgetDesignFlags::Designing); }
		if (!Status->WidgetTree) { Status->WidgetTree = NewObject<UWidgetTree>(Status.Get(), TEXT("WidgetTree"), RF_Transient); }
		UWidgetTree* Tree = Status->WidgetTree;
		Root = Child<UCanvasPanel>(Tree, TEXT("PlayerStatusRoot"), nullptr); Tree->RootWidget = Root;
		Bars = Child<UCanvasPanel>(Tree, TEXT("PlayerStatusBars"), Root);
		CastChecked<UCanvasPanelSlot>(Bars->Slot)->SetPosition(FVector2D(17, 23));
		CastChecked<UCanvasPanelSlot>(Bars->Slot)->SetSize(FVector2D(451, 79));
		UCanvasPanel* Armor = Child<UCanvasPanel>(Tree, TEXT("PlayerArmorBar"), Bars);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Segment = Child<UCanvasPanel>(Tree, *FString::Printf(TEXT("PlayerArmorSegment%d"), Index), Armor);
			Child<UImage>(Tree, *FString::Printf(TEXT("PlayerArmorBorder%d"), Index), Segment);
			UImage* Image = Child<UImage>(Tree, *FString::Printf(TEXT("PlayerArmorFill%d"), Index), Segment);
			if (Index == 0) { Fill = Image; }
		}
		Shield = Child<UTextBlock>(Tree, TEXT("ShieldNumber"), Armor);
		Shield->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 27));
		Shield->SetColorAndOpacity(FSlateColor(FLinearColor(.2f, .3f, .4f, .9f)));
		CastChecked<UCanvasPanelSlot>(Shield->Slot)->SetPosition(FVector2D(11, 13));
		Host = Child<USizeBox>(Tree, TEXT("PlayerHealthHost"), Bars);
		Host->SetWidthOverride(449); Host->SetHeightOverride(42);
		// A concrete transient UUserWidget shell is enough: no production BP, graphs or health source.
		Health = NewObject<UDCBossHealthWidget>(Tree, TEXT("OriginalHealthbar"), RF_Transient);
		Health->Initialize(); Status->HealthWidgetClass = Health->GetClass(); Host->SetContent(Health);
		if (!Health->WidgetTree) { Health->WidgetTree = NewObject<UWidgetTree>(Health, TEXT("WidgetTree"), RF_Transient); }
		HealthSize = Child<USizeBox>(Health->WidgetTree, TEXT("AnimBoundSB"), nullptr);
		Health->WidgetTree->RootWidget = HealthSize;
		HealthSize->SetWidthOverride(449); HealthSize->SetHeightOverride(42);
		UCanvasPanel* Canvas = Child<UCanvasPanel>(Health->WidgetTree, TEXT("HealthLabelCanvas"), HealthSize);
		Number = Child<UCommonNumericTextBlock>(Health->WidgetTree, TEXT("HealthNumber"), Canvas);
		Number->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 31));
		Number->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		CastChecked<UCanvasPanelSlot>(Number->Slot)->SetPosition(FVector2D(29, 19));
		Child<UTextBlock>(Health->WidgetTree, TEXT("DCMaxHealthText"), Canvas)->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanel* Signature = Child<UCanvasPanel>(Tree, TEXT("PlayerSignature"), Root);
		Child<UImage>(Tree, TEXT("PlayerSignatureStar"), Signature);
		Child<UImage>(Tree, TEXT("PlayerSignatureRing"), Signature);
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPlayerStatusDesignerLayoutTest, "DreamCatcher.UI.PlayerStatus.Designer.AuthoredLayoutAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCPlayerStatusDesignerLayoutTest::RunTest(const FString&)
{
	if (!DCPlayerStatusDesignerTests::Context(*this)) { return false; }
	DCPlayerStatusDesignerTests::FFixture F;
	if (!TestTrue(TEXT("Typed authored hierarchy binds"), FDCPlayerStatusDesignerTestAccess::Bind(F.Status.Get()))) { return false; }
	const FSlateFontInfo ShieldFont = F.Shield->GetFont(), HealthFont = F.Number->GetFont();
	const FSlateColor ShieldColor = F.Shield->GetColorAndOpacity(), HealthColor = F.Number->GetColorAndOpacity();
	F.Status->StatusStyle.BarWidth = 999; F.Status->StatusStyle.FontSize = 65;
	for (const FVector2D Values : {FVector2D(100, 150), FVector2D(50, 75), FVector2D(0, 0), FVector2D(100, 150)})
	{
		F.Status->PreviewHealth = static_cast<float>(Values.X); F.Status->PreviewArmor = static_cast<float>(Values.Y);
		F.Status->SynchronizeProperties(); FDCPlayerStatusDesignerTestAccess::Layout(F.Status.Get());
		F.Status->RefreshPlayerStatus();
		TestEqual(TEXT("Preview current HP"), F.Number->GetTargetValue(), static_cast<float>(Values.X));
		TestEqual(TEXT("Preview armor text"), F.Shield->GetText().ToString(), FString::FromInt(FMath::RoundToInt(Values.Y)));
		TestFalse(TEXT("Preview never becomes gameplay context"), F.Status->HasPlayerStatus());
		TestFalse(TEXT("Preview never starts refresh timer"), FDCPlayerStatusDesignerTestAccess::HasTimer(F.Status.Get()));
		TestNull(TEXT("Preview has no Pawn"), F.Status->GetStatusPawn());
		TestNull(TEXT("Preview has no ASC"), F.Status->GetStatusAbilitySystem());
	}
	TestTrue(TEXT("Authored bar position retained"), CastChecked<UCanvasPanelSlot>(F.Bars->Slot)->GetPosition().Equals(FVector2D(17, 23)));
	TestTrue(TEXT("Authored bar size retained"), CastChecked<UCanvasPanelSlot>(F.Bars->Slot)->GetSize().Equals(FVector2D(451, 79)));
	TestTrue(TEXT("Shield font/color retained"), F.Shield->GetFont() == ShieldFont && F.Shield->GetColorAndOpacity() == ShieldColor);
	TestTrue(TEXT("Health font/color retained"), F.Number->GetFont() == HealthFont && F.Number->GetColorAndOpacity() == HealthColor);
	TestTrue(TEXT("Health direct XY retained"), CastChecked<UCanvasPanelSlot>(F.Number->Slot)->GetPosition().Equals(FVector2D(29, 19)));
	TestEqual(TEXT("Authored nested HP height retained"), F.HealthSize->GetHeightOverride(), 42.f);
	// UE adds designer flags with bitwise OR; SetDesignerFlags(None) does not clear
	// Designing. Runtime widgets are separate instances, as they are in the real HUD.
	DCPlayerStatusDesignerTests::FFixture Runtime(false);
	TestFalse(TEXT("Runtime fixture starts outside Designer"), Runtime.Status->IsDesignTime());
	TestTrue(TEXT("Runtime authored hierarchy binds"), FDCPlayerStatusDesignerTestAccess::Bind(Runtime.Status.Get()));
	FDCPlayerStatusDesignerTestAccess::RefreshWithoutPawn(Runtime.Status.Get());
	TestFalse(TEXT("NoPawn never shows preview as gameplay"), Runtime.Status->HasPlayerStatus());
	TestEqual(TEXT("NoPawn hides bars"), Runtime.Bars->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("NoPawn zeros presentation"), Runtime.Status->GetDisplayedHealth(), 0.f);
	TestTrue(TEXT("NoPawn preserves saved child"), Runtime.Host->GetContent() == Runtime.Health);
	FDCPlayerStatusDesignerTestAccess::Destruct(Runtime.Status.Get());
	FDCPlayerStatusDesignerTestAccess::Destruct(F.Status.Get());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPlayerStatusDesignerLifecycleTest, "DreamCatcher.UI.PlayerStatus.Designer.AuthoredChildAndBrushLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCPlayerStatusDesignerLifecycleTest::RunTest(const FString&)
{
	if (!DCPlayerStatusDesignerTests::Context(*this)) { return false; }
	DCPlayerStatusDesignerTests::FFixture F;
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/DreamCatcher/UI/HUD/Art/MI_DC_HUD_ArmorArt_Left.MI_DC_HUD_ArmorArt_Left"));
	if (!TestNotNull(TEXT("Existing shield material"), Material)) { return false; }
	FSlateBrush Brush = F.Fill->GetBrush(); Brush.SetResourceObject(Material); F.Fill->SetBrush(Brush);
	F.Status->SynchronizeProperties();
	TSharedPtr<SWidget> Slate = F.Status->TakeWidget();
	TestTrue(TEXT("Slate rebuild retains the authored tree and child"), F.Status->WidgetTree->RootWidget == F.Root && F.Host->GetContent() == F.Health);
	UMaterialInstanceDynamic* MID = FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).FillMID;
	if (!TestNotNull(TEXT("Preview has transient material"), MID)) { return false; }
	TestTrue(TEXT("MID is transient"), MID->HasAnyFlags(RF_Transient));
	F.Status->PreviewArmor = 25; F.Status->SynchronizeProperties();
	TestTrue(TEXT("Resync reuses MID, never captures it as authored"), FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).FillMID == MID
		&& FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).AuthoredFillBrush.GetResourceObject() == Material);
	TestEqual(TEXT("Partial segment receives actual fill"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), .5f);
	FDCPlayerStatusDesignerTestAccess::Release(F.Status.Get()); FDCPlayerStatusDesignerTestAccess::Ensure(F.Status.Get());
	TestTrue(TEXT("Release/ensure preserves exact saved child"), F.Host->GetContent() == F.Health && F.Status->GetOriginalHealthWidget() == F.Health);
	TestTrue(TEXT("NoPawn release retains a pending rebind even without a live controller"), FDCPlayerStatusDesignerTestAccess::Released(F.Status.Get()));
	FDCPlayerStatusDesignerTestAccess::Destruct(F.Status.Get());
	TestTrue(TEXT("Destruct restores authored source brush"), F.Fill->GetBrush().GetResourceObject() == Material);
	TestNull(TEXT("Destruct drops MID"), FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).FillMID.Get());
	TestTrue(TEXT("Destruct retains child and parent slot"), F.Host->GetContent() == F.Health && F.Health->GetParent() == F.Host);
	Slate.Reset(); F.Status->ReleaseSlateResources(true);
	Slate = F.Status->TakeWidget();
	F.Status->SynchronizeProperties();
	TestNotNull(TEXT("Reentry can recreate material from authored brush"), FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).FillMID.Get());
	TestTrue(TEXT("Reentry still uses authored source"), FDCPlayerStatusDesignerTestAccess::Segment(F.Status.Get()).AuthoredFillBrush.GetResourceObject() == Material);
	FDCPlayerStatusDesignerTestAccess::Destruct(F.Status.Get());
	Slate.Reset(); F.Status->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPlayerStatusDesignerInvalidTreeTest, "DreamCatcher.UI.PlayerStatus.Designer.IncompleteTreePreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCPlayerStatusDesignerInvalidTreeTest::RunTest(const FString&)
{
	if (!DCPlayerStatusDesignerTests::Context(*this)) { return false; }
	DCPlayerStatusDesignerTests::FFixture F;
	F.Shield->RemoveFromParent();
	AddExpectedError(TEXT("Player status Designer hierarchy is incomplete"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Missing named shield label refuses binding"), FDCPlayerStatusDesignerTestAccess::Bind(F.Status.Get()));
	F.Status->SynchronizeProperties();
	TestTrue(TEXT("Invalid authored root is never replaced by native fallback"), F.Status->WidgetTree->RootWidget == F.Root);
	TestTrue(TEXT("Invalid tree preserves authored nested child"), F.Host->GetContent() == F.Health);
	return !HasAnyErrors();
}

#endif
