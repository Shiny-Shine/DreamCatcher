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
#include <limits>

struct FDCPlayerStatusDesignerTestAccess
{
	static bool Bind(UDCPlayerStatusWidget* W) { return W->BindDesignerWidgets(); }
	static void Release(UDCPlayerStatusWidget* W) { W->ReleaseHealthWidget(); }
	static void Ensure(UDCPlayerStatusWidget* W) { W->EnsureHealthWidget(); }
	static void Destruct(UDCPlayerStatusWidget* W) { W->NativeDestruct(); }
	static void RefreshWithoutPawn(UDCPlayerStatusWidget* W) { W->bConstructed = true; W->RefreshPlayerStatus(); W->bConstructed = false; }
	static const FDCPlayerArmorSegment& Segment(const UDCPlayerStatusWidget* W, int32 Index = 0) { return W->ArmorSegments[Index]; }
	static void PresentArmor(UDCPlayerStatusWidget* W, float Value, float Maximum = 150.f)
	{
		// UI state only: no Pawn, ASC, gameplay attributes or timers are created.
		W->bConstructed = true; W->bHasContext = true;
		W->DisplayedArmor = Value; W->DisplayedMaxArmor = Maximum;
		W->UpdateArmorPresentation();
	}
	static void AdvanceArmor(UDCPlayerStatusWidget* W, float DeltaTime) { W->AdvanceArmorAnimation(DeltaTime); }
	static void Unbind(UDCPlayerStatusWidget* W) { W->UnbindSource(); }
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
		Status->Initialize();
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

bool BindArmorMaterials(FAutomationTestBase& Test, FFixture& Fixture)
{
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/DreamCatcher/UI/HUD/Art/MI_DC_HUD_ArmorArt_Left.MI_DC_HUD_ArmorArt_Left"));
	if (!Test.TestNotNull(TEXT("Saved shield material supplies the animation parameters"), Material)) { return false; }
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UImage* ArmorImage = Cast<UImage>(Fixture.Status->GetWidgetFromName(*FString::Printf(TEXT("PlayerArmorFill%d"), Index)));
		if (!Test.TestNotNull(TEXT("Named shield image"), ArmorImage)) { return false; }
		FSlateBrush Brush = ArmorImage->GetBrush(); Brush.SetResourceObject(Material); ArmorImage->SetBrush(Brush);
	}
	return Test.TestTrue(TEXT("Runtime authored hierarchy binds"), FDCPlayerStatusDesignerTestAccess::Bind(Fixture.Status.Get()));
}
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
	for (const FVector2D Values : {FVector2D(100, 150), FVector2D(50, 75), FVector2D(0, 0), FVector2D(100, 150)})
	{
		F.Status->PreviewHealth = static_cast<float>(Values.X); F.Status->PreviewArmor = static_cast<float>(Values.Y);
		F.Status->SynchronizeProperties();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPlayerStatusArmorAnimationTest, "DreamCatcher.UI.PlayerStatus.Designer.ArmorAnimationTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCPlayerStatusArmorAnimationTest::RunTest(const FString&)
{
	if (!DCPlayerStatusDesignerTests::Context(*this)) { return false; }
	DCPlayerStatusDesignerTests::FFixture F(false);
	if (!DCPlayerStatusDesignerTests::BindArmorMaterials(*this, F)) { return false; }
	using Access = FDCPlayerStatusDesignerTestAccess;
	Access::PresentArmor(F.Status.Get(), 150.f);
	TArray<ESlateVisibility> AuthoredVisibility;
	for (int32 Index = 0; Index < 3; ++Index) { AuthoredVisibility.Add(Access::Segment(F.Status.Get(), Index).Fill->GetVisibility()); }
	UMaterialInstanceDynamic* MID = Access::Segment(F.Status.Get(), 2).FillMID.Get();
	if (!TestNotNull(TEXT("Third segment MID"), MID)) { return false; }
	TestFalse(TEXT("Initial status snaps without damage feedback"), Access::Segment(F.Status.Get(), 2).bAnimating);
	Access::PresentArmor(F.Status.Get(), 100.f);
	TestEqual(TEXT("Numeric shield updates immediately"), F.Shield->GetText().ToString(), FString(TEXT("100")));
	TestEqual(TEXT("Damage starts with old full segment"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), 1.f);
	TestEqual(TEXT("Damage records empty target separately"), MID->K2_GetScalarParameterValue(TEXT("Health_Updated")), 0.f);
	TestFalse(TEXT("Unaffected left segment stays settled"), Access::Segment(F.Status.Get(), 0).bAnimating);
	TestFalse(TEXT("Unaffected middle segment stays settled"), Access::Segment(F.Status.Get(), 1).bAnimating);
	Access::AdvanceArmor(F.Status.Get(), .06f);
	TestTrue(TEXT("Saved zero-tangent cubic at quarter of 0.24 seconds"),
		FMath::IsNearlyEqual(MID->K2_GetScalarParameterValue(TEXT("Animate_HealthNewToUpdated")), .15625f, 1.e-4f));
	const float Elapsed = Access::Segment(F.Status.Get(), 2).AnimationElapsed;
	Access::PresentArmor(F.Status.Get(), 100.f);
	TestEqual(TEXT("Periodic refresh does not restart damage"), Access::Segment(F.Status.Get(), 2).AnimationElapsed, Elapsed);
	TestTrue(TEXT("Refresh retains the same MID"), Access::Segment(F.Status.Get(), 2).FillMID.Get() == MID);
	Access::AdvanceArmor(F.Status.Get(), .18f);
	TestTrue(TEXT("Transition reaches its target at 0.24 seconds"),
		FMath::IsNearlyEqual(MID->K2_GetScalarParameterValue(TEXT("Animate_HealthNewToUpdated")), 1.f, 1.e-4f));
	TestEqual(TEXT("Damage fade holds until 0.4 seconds"), MID->K2_GetScalarParameterValue(TEXT("Animate_UpdatedFade")), 0.f);
	Access::AdvanceArmor(F.Status.Get(), .36f);
	TestTrue(TEXT("Damage is halfway faded at 0.6 seconds"),
		FMath::IsNearlyEqual(MID->K2_GetScalarParameterValue(TEXT("Animate_UpdatedFade")), .5f, 1.e-4f));
	Access::AdvanceArmor(F.Status.Get(), .21f);
	TestFalse(TEXT("Damage animation finishes"), Access::Segment(F.Status.Get(), 2).bAnimating);
	TestEqual(TEXT("Settled current equals empty shield"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), 0.f);
	TestEqual(TEXT("Settled fade remains complete"), MID->K2_GetScalarParameterValue(TEXT("Animate_UpdatedFade")), 1.f);
	TestTrue(TEXT("Material-backed segment retains full authored geometry"), Access::Segment(F.Status.Get(), 2).Fill->GetRenderTransform().Scale.Equals(FVector2D(1.f, 1.f)));
	TestTrue(TEXT("Authored bar position retained during animation"), CastChecked<UCanvasPanelSlot>(F.Bars->Slot)->GetPosition().Equals(FVector2D(17, 23)));
	TestTrue(TEXT("Authored bar size retained during animation"), CastChecked<UCanvasPanelSlot>(F.Bars->Slot)->GetSize().Equals(FVector2D(451, 79)));
	Access::PresentArmor(F.Status.Get(), 0.f);
	Access::AdvanceArmor(F.Status.Get(), .8f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FDCPlayerArmorSegment& Segment = Access::Segment(F.Status.Get(), Index);
		TestEqual(TEXT("Zero armor clears every fill target"), Segment.Fraction, 0.f);
		TestFalse(TEXT("Zero armor settles each segment"), Segment.bAnimating);
		TestEqual(TEXT("Empty segment retains authored visibility for the gray material"), Segment.Fill->GetVisibility(), AuthoredVisibility[Index]);
	}
	TestFalse(TEXT("UI simulation creates no gameplay timer"), Access::HasTimer(F.Status.Get()));
	TestNull(TEXT("UI simulation creates no ASC"), F.Status->GetStatusAbilitySystem());
	Access::Destruct(F.Status.Get());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPlayerStatusArmorRetargetTest, "DreamCatcher.UI.PlayerStatus.Designer.ArmorAnimationRetargetAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCPlayerStatusArmorRetargetTest::RunTest(const FString&)
{
	if (!DCPlayerStatusDesignerTests::Context(*this)) { return false; }
	DCPlayerStatusDesignerTests::FFixture F(false);
	if (!DCPlayerStatusDesignerTests::BindArmorMaterials(*this, F)) { return false; }
	using Access = FDCPlayerStatusDesignerTestAccess;
	Access::PresentArmor(F.Status.Get(), 150.f);
	Access::PresentArmor(F.Status.Get(), 125.f);
	Access::AdvanceArmor(F.Status.Get(), .6f);
	Access::PresentArmor(F.Status.Get(), 110.f);
	const FDCPlayerArmorSegment& Last = Access::Segment(F.Status.Get(), 2);
	TestEqual(TEXT("Repeated hit starts at the prior actual value, not the old full value"), Last.PreviousFraction, .5f);
	TestTrue(TEXT("Repeated hit uses latest actual target"), FMath::IsNearlyEqual(Last.Fraction, .2f));
	TestEqual(TEXT("New damage starts a fresh animation"), Last.AnimationElapsed, 0.f);
	Access::AdvanceArmor(F.Status.Get(), std::numeric_limits<float>::quiet_NaN());
	Access::AdvanceArmor(F.Status.Get(), -1.f);
	TestEqual(TEXT("Invalid deltas cannot corrupt animation state"), Last.AnimationElapsed, 0.f);
	Access::PresentArmor(F.Status.Get(), 0.f);
	Access::AdvanceArmor(F.Status.Get(), .8f);
	Access::PresentArmor(F.Status.Get(), 75.f);
	Access::AdvanceArmor(F.Status.Get(), .12f);
	const FDCPlayerArmorSegment& Middle = Access::Segment(F.Status.Get(), 1);
	TestTrue(TEXT("Recovery uses the healing material path"), Middle.bAnimating && Middle.bHealing);
	TestEqual(TEXT("Recovery reaches the actual partial-segment target"), Middle.Fraction, .5f);
	Access::PresentArmor(F.Status.Get(), 60.f);
	TestTrue(TEXT("Interrupted recovery starts from the visible halfway fill"), FMath::IsNearlyEqual(Middle.PreviousFraction, .25f));
	TestTrue(TEXT("Interrupted recovery switches to the latest lower target"), !Middle.bHealing && FMath::IsNearlyEqual(Middle.Fraction, .2f));
	Access::PresentArmor(F.Status.Get(), 100.f, 300.f);
	TestFalse(TEXT("Capacity changes snap rather than looking like damage"), Middle.bAnimating);
	TestEqual(TEXT("New capacity determines the segment target"), Middle.Fraction, 0.f);
	Access::Unbind(F.Status.Get());
	TestFalse(TEXT("Losing the source clears gameplay context"), F.Status->HasPlayerStatus());
	Access::PresentArmor(F.Status.Get(), 60.f);
	TestFalse(TEXT("New source initializes without an old damage trail"), Access::Segment(F.Status.Get(), 1).bAnimating);
	TestTrue(TEXT("New source has matching current and target fill"),
		FMath::IsNearlyEqual(Access::Segment(F.Status.Get(), 1).PreviousFraction, .2f));
	Access::PresentArmor(F.Status.Get(), 30.f);
	TestTrue(TEXT("Later damage can animate again"), Access::Segment(F.Status.Get(), 0).bAnimating);
	Access::Destruct(F.Status.Get());
	Access::AdvanceArmor(F.Status.Get(), .8f);
	TestFalse(TEXT("Destruct cancels the pending animation"), Access::Segment(F.Status.Get(), 0).bAnimating);
	TestNull(TEXT("Destruct releases the animated MID"), Access::Segment(F.Status.Get(), 0).FillMID.Get());
	TestTrue(TEXT("Rebind retains the Designer hierarchy"), Access::Bind(F.Status.Get()));
	Access::PresentArmor(F.Status.Get(), 150.f);
	TestFalse(TEXT("Reentry snaps to the new full status"), Access::Segment(F.Status.Get(), 0).bAnimating);
	TestEqual(TEXT("Reentry is full"), Access::Segment(F.Status.Get(), 0).Fraction, 1.f);
	Access::Destruct(F.Status.Get());
	return !HasAnyErrors();
}

#endif
