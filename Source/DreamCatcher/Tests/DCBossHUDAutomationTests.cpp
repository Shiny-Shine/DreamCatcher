// Unsaved, scoped presentation tests. Existing R12 tests cover combat and phase rules.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AI/Boss/DCBossData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "UI/Boss/DCBossHealthWidget.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

struct FDCBossHUDTestAccess
{
	static float Fraction(const UDCBossHealthWidget* W) { return W->Fraction; }
	static UTextBlock* Name(const UDCBossHealthWidget* W) { return W->BossName; }
	static UTextBlock* Subtitle(const UDCBossHealthWidget* W) { return W->BossSubtitle; }
	static UImage* Fill(const UDCBossHealthWidget* W) { return W->BossBarFill; }
	static UCanvasPanel* Panel(const UDCBossHealthWidget* W) { return W->BossPanel; }
	static UMaterialInstanceDynamic* FillMID(const UDCBossHealthWidget* W) { return W->FillMID; }
	static UMaterialInstanceDynamic* BorderMID(const UDCBossHealthWidget* W) { return W->BorderMID; }
	static void Destruct(UDCBossHealthWidget* W) { W->NativeDestruct(); }
	static void Construct(UDCBossHealthWidget* W) { W->NativeConstruct(); }
	static void BuildDesignerFixture(UDCBossHealthWidget* W, UMaterialInterface* Border = nullptr, UMaterialInterface* Fill = nullptr)
	{
		W->Initialize();
		if (!W->WidgetTree) { W->WidgetTree = NewObject<UWidgetTree>(W, TEXT("WidgetTree"), RF_Transient); }
		UWidgetTree* Tree = W->WidgetTree;
		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(); Tree->RootWidget = Root;
		W->BossPanel = Tree->ConstructWidget<UCanvasPanel>();
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(W->BossPanel);
		PanelSlot->SetPosition(FVector2D(13, 29)); PanelSlot->SetSize(FVector2D(701, 91));
		W->BossName = Tree->ConstructWidget<UTextBlock>(); W->BossPanel->AddChild(W->BossName);
		W->BossName->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 27));
		W->BossName->SetColorAndOpacity(FSlateColor(FLinearColor(.2f, .3f, .4f, 1.f)));
		W->BossSubtitle = Tree->ConstructWidget<UTextBlock>(); W->BossPanel->AddChild(W->BossSubtitle);
		W->BossBar = Tree->ConstructWidget<UCanvasPanel>(); W->BossPanel->AddChild(W->BossBar);
		W->BossBarBorder = Tree->ConstructWidget<UImage>(); W->BossBar->AddChild(W->BossBarBorder);
		W->BossBarFill = Tree->ConstructWidget<UImage>(); W->BossBar->AddChild(W->BossBarFill);
		FSlateBrush Brush; Brush.SetResourceObject(Border); W->BossBarBorder->SetBrush(Brush);
		Brush.SetResourceObject(Fill); W->BossBarFill->SetBrush(Brush);
	}
};

namespace DCBossHUDTests
{
bool Context(FAutomationTestBase& Test)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCBossHUDIsolatedAutomation")))
	{
		Test.AddError(TEXT("Use a separate unattended editor with -DCBossHUDIsolatedAutomation.")); return false;
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossHUDSafetyTest, "DreamCatcher.UI.BossHUD.PresentationSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossHUDSafetyTest::RunTest(const FString&)
{
	if (!DCBossHUDTests::Context(*this)) { return false; }
	TStrongObjectPtr<UDCBossData> Data(NewObject<UDCBossData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCBossHealthWidget> W(NewObject<UDCBossHealthWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	FDCBossHUDTestAccess::BuildDesignerFixture(W.Get());
	const FSlateFontInfo Font = FDCBossHUDTestAccess::Name(W.Get())->GetFont();
	const FSlateColor Color = FDCBossHUDTestAccess::Name(W.Get())->GetColorAndOpacity();
	W->SetBossStatus(Data.Get(), 300.f, 600.f);
	TSharedPtr<SWidget> Slate = W->TakeWidget();
	TestEqual(TEXT("Approved name"), FDCBossHUDTestAccess::Name(W.Get())->GetText().ToString(), FString(TEXT("춘식이")));
	TestEqual(TEXT("Approved subtitle"), FDCBossHUDTestAccess::Subtitle(W.Get())->GetText().ToString(), FString(TEXT("나스닥의 수호자")));
	TestEqual(TEXT("Health fraction"), FDCBossHUDTestAccess::Fraction(W.Get()), .5f);
	Data->DisplayName = FText::FromString(TEXT("Edited boss")); Data->Subtitle = FText::FromString(TEXT("Edited subtitle"));
	W->SetBossStatus(Data.Get(), 2000, 600);
	TestEqual(TEXT("Data name still reaches display"), FDCBossHUDTestAccess::Name(W.Get())->GetText().ToString(), FString(TEXT("Edited boss")));
	TestEqual(TEXT("Overheal bounded"), FDCBossHUDTestAccess::Fraction(W.Get()), 1.f);
	for (const FVector2D Values : {FVector2D(-10, 600), FVector2D(100, 0), FVector2D(100, -1),
		FVector2D(std::numeric_limits<float>::quiet_NaN(), 600), FVector2D(100, std::numeric_limits<float>::infinity())})
	{
		W->SetBossStatus(Data.Get(), static_cast<float>(Values.X), static_cast<float>(Values.Y));
		TestEqual(TEXT("Invalid health safely shows empty fill"), FDCBossHUDTestAccess::Fraction(W.Get()), 0.f);
	}
	const UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(FDCBossHUDTestAccess::Panel(W.Get())->Slot);
	TestTrue(TEXT("Updates preserve Designer placement"), Slot->GetPosition().Equals(FVector2D(13, 29)) && Slot->GetSize().Equals(FVector2D(701, 91)));
	TestTrue(TEXT("Updates preserve Designer font/color"), FDCBossHUDTestAccess::Name(W.Get())->GetFont() == Font
		&& FDCBossHUDTestAccess::Name(W.Get())->GetColorAndOpacity() == Color);
	TestTrue(TEXT("UI never changes combat configuration"), Data->IsValidConfiguration() && Data->MaxHealth == 600.f);
	FDCBossHUDTestAccess::Destruct(W.Get()); Slate.Reset(); W->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCBossHUDMaterialTest, "DreamCatcher.UI.BossHUD.LyraMaterialContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCBossHUDMaterialTest::RunTest(const FString&)
{
	if (!DCBossHUDTests::Context(*this)) { return false; }
	const TCHAR* Art = TEXT("/Game/DreamCatcher/Lyra/UI/Shared/Equipment/LyraStarterGame/UI/Hud/Art/");
	UMaterialInterface* Border = LoadObject<UMaterialInterface>(nullptr, *(FString(Art) + TEXT("MI_UI_HealthBar_BarBorder.MI_UI_HealthBar_BarBorder")));
	UMaterialInterface* Fill = LoadObject<UMaterialInterface>(nullptr, *(FString(Art) + TEXT("MI_UI_HealthBar_Fill.MI_UI_HealthBar_Fill")));
	if (!TestNotNull(TEXT("Existing border material"), Border) || !TestNotNull(TEXT("Existing fill material"), Fill)) { return false; }
	TStrongObjectPtr<UDCBossData> Data(NewObject<UDCBossData>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCBossHealthWidget> W(NewObject<UDCBossHealthWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	FDCBossHUDTestAccess::BuildDesignerFixture(W.Get(), Border, Fill);
	W->SetBossStatus(Data.Get(), 150, 600); TSharedPtr<SWidget> Slate = W->TakeWidget();
	UMaterialInstanceDynamic* MID = FDCBossHUDTestAccess::FillMID(W.Get());
	if (!TestNotNull(TEXT("Runtime MID"), MID)) { return false; }
	TestTrue(TEXT("MID is transient"), MID->HasAnyFlags(RF_Transient));
	TestEqual(TEXT("Original current parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), .25f);
	TestEqual(TEXT("Original updated parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Updated")), .25f);
	TestTrue(TEXT("Material fill avoids a second scale"), FDCBossHUDTestAccess::Fill(W.Get())->GetRenderTransform().Scale.Equals(FVector2D(1, 1)));
	W->SetBossStatus(Data.Get(), 300, 600);
	TestTrue(TEXT("Health refresh reuses MID"), FDCBossHUDTestAccess::FillMID(W.Get()) == MID);
	TestEqual(TEXT("Health refresh updates parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), .5f);
	FLinearColor OriginalColor;
	TestTrue(TEXT("Original color exists"), Fill->GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA"), OriginalColor));
	FDCBossHUDTestAccess::Destruct(W.Get());
	TestTrue(TEXT("Destruct restores authored brush"), FDCBossHUDTestAccess::Fill(W.Get())->GetBrush().GetResourceObject() == Fill);
	TestNull(TEXT("Destruct releases MID"), FDCBossHUDTestAccess::FillMID(W.Get()));
	FSlateBrush Brush = FDCBossHUDTestAccess::Fill(W.Get())->GetBrush();
	const FLinearColor Tint(.5f, .25f, .75f, .5f); Brush.TintColor = FSlateColor(Tint);
	FDCBossHUDTestAccess::Fill(W.Get())->SetBrush(Brush);
	FDCBossHUDTestAccess::Construct(W.Get()); MID = FDCBossHUDTestAccess::FillMID(W.Get());
	if (!TestNotNull(TEXT("Reentry recreates material from authored brush"), MID)) { return false; }
	TestTrue(TEXT("Designer tint reaches the material"), MID->K2_GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA")).Equals(OriginalColor * Tint));
	W->SetBossStatus(Data.Get(), 150, 600);
	TestTrue(TEXT("Refresh never compounds tint"), MID->K2_GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA")).Equals(OriginalColor * Tint));
	FLinearColor SourceAfter; Fill->GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA"), SourceAfter);
	TestTrue(TEXT("Source material is preserved"), SourceAfter.Equals(OriginalColor));
	FDCBossHUDTestAccess::Destruct(W.Get()); Slate.Reset(); W->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

#endif
