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
#include "UI/Boss/DCBossHealthWidget.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

struct FDCBossHUDTestAccess
{
	static float Fraction(const UDCBossHealthWidget* W) { return W->Fraction; }
	static FDCBossHUDStyle Style(const UDCBossHealthWidget* W) { return W->Appearance; }
	static UTextBlock* Name(const UDCBossHealthWidget* W) { return W->Title; }
	static UTextBlock* Subtitle(const UDCBossHealthWidget* W) { return W->SubtitleText; }
	static UImage* Fill(const UDCBossHealthWidget* W) { return W->FillImage; }
	static UMaterialInstanceDynamic* FillMID(const UDCBossHealthWidget* W) { return W->FillMID; }
	static UMaterialInstanceDynamic* BorderMID(const UDCBossHealthWidget* W) { return W->BorderMID; }
	static void Destruct(UDCBossHealthWidget* W) { W->NativeDestruct(); }
	static void Construct(UDCBossHealthWidget* W) { W->NativeConstruct(); }
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
	W->Initialize();
	W->SetBossStatus(Data.Get(), 300.f, 600.f);
	TSharedPtr<SWidget> Slate = W->TakeWidget();
	if (!TestNotNull(TEXT("Rebuilt name widget"), FDCBossHUDTestAccess::Name(W.Get()))
		|| !TestNotNull(TEXT("Rebuilt subtitle widget"), FDCBossHUDTestAccess::Subtitle(W.Get()))) { return false; }
	TestEqual(TEXT("Exact approved name"), FDCBossHUDTestAccess::Name(W.Get())->GetText().ToString(), FString(TEXT("춘식이")));
	TestEqual(TEXT("Exact approved subtitle"), FDCBossHUDTestAccess::Subtitle(W.Get())->GetText().ToString(), FString(TEXT("나스닥의 수호자")));
	TestEqual(TEXT("Boss health fraction"), FDCBossHUDTestAccess::Fraction(W.Get()), .5f);
	Data->DisplayName = FText::FromString(TEXT("Edited boss")); Data->Subtitle = FText::FromString(TEXT("Edited subtitle"));
	Data->HUDStyle.PanelSize = FVector2D(800, 100); Data->HUDStyle.BarSize = FVector2D(680, 20);
	Data->HUDStyle.PanelOffset = FVector2D(6, 96);
	W->SetBossStatus(Data.Get(), 2000, 600);
	TestEqual(TEXT("Editor name update reaches display"), FDCBossHUDTestAccess::Name(W.Get())->GetText().ToString(), FString(TEXT("Edited boss")));
	TestEqual(TEXT("Editor subtitle update reaches display"), FDCBossHUDTestAccess::Subtitle(W.Get())->GetText().ToString(), FString(TEXT("Edited subtitle")));
	TestTrue(TEXT("Layout data reaches display"), FDCBossHUDTestAccess::Style(W.Get()).PanelSize.Equals(FVector2D(800, 100)));
	TestEqual(TEXT("Overheal bounded"), FDCBossHUDTestAccess::Fraction(W.Get()), 1.f);
	for (const FVector2D Values : {FVector2D(-10, 600), FVector2D(100, 0), FVector2D(100, -1),
		FVector2D(std::numeric_limits<float>::quiet_NaN(), 600), FVector2D(100, std::numeric_limits<float>::infinity())})
	{
		W->SetBossStatus(Data.Get(), static_cast<float>(Values.X), static_cast<float>(Values.Y));
		TestEqual(TEXT("Invalid/negative health safely shows empty fill"), FDCBossHUDTestAccess::Fraction(W.Get()), 0.f);
	}
	Data->HUDStyle.PanelSize.X = std::numeric_limits<double>::quiet_NaN();
	W->SetBossStatus(Data.Get(), 300, 600);
	TestTrue(TEXT("Bad UI layout falls back without poisoning geometry"), FDCBossHUDTestAccess::Style(W.Get()).PanelSize.Equals(FVector2D(720, 86)));
	TestTrue(TEXT("Bad UI settings do not fail combat data validation"), Data->IsValidConfiguration());
	TestEqual(TEXT("Bad UI settings do not alter actual MaxHealth"), Data->MaxHealth, 600.f);
	Slate.Reset(); W->ReleaseSlateResources(true);
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
	if (!TestNotNull(TEXT("Existing Lyra border material"), Border) || !TestNotNull(TEXT("Existing Lyra fill material"), Fill)) { return false; }
	TStrongObjectPtr<UDCBossData> Data(NewObject<UDCBossData>(GetTransientPackage(), NAME_None, RF_Transient));
	Data->HUDStyle.BarBorderBrush.SetResourceObject(Border); Data->HUDStyle.BarFillBrush.SetResourceObject(Fill);
	TStrongObjectPtr<UDCBossHealthWidget> W(NewObject<UDCBossHealthWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	W->Initialize(); W->SetBossStatus(Data.Get(), 150, 600);
	TSharedPtr<SWidget> Slate = W->TakeWidget();
	UMaterialInstanceDynamic* MID = FDCBossHUDTestAccess::FillMID(W.Get());
	if (!TestNotNull(TEXT("Runtime MID"), MID)) { return false; }
	TestTrue(TEXT("MID is transient"), MID->HasAnyFlags(RF_Transient));
	TestEqual(TEXT("Original current parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), .25f);
	TestEqual(TEXT("Original updated parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Updated")), .25f);
	TestTrue(TEXT("Material fill avoids a second clipping/scale"), FDCBossHUDTestAccess::Fill(W.Get())->GetRenderTransform().Scale.Equals(FVector2D(1, 1)));
	W->SetBossStatus(Data.Get(), 300, 600);
	TestTrue(TEXT("Health refresh reuses runtime MID"), FDCBossHUDTestAccess::FillMID(W.Get()) == MID);
	TestEqual(TEXT("Health refresh updates original parameter"), MID->K2_GetScalarParameterValue(TEXT("Health_Current")), .5f);
	TestTrue(TEXT("Source brush resources stay original"), Data->HUDStyle.BarBorderBrush.GetResourceObject() == Border
		&& Data->HUDStyle.BarFillBrush.GetResourceObject() == Fill);
	FLinearColor OriginalColor;
	TestTrue(TEXT("Original health color parameter exists"), Fill->GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA"), OriginalColor));
	const FLinearColor Tint(.5f, .25f, .75f, .5f);
	Data->HUDStyle.BarFillBrush.TintColor = FSlateColor(Tint);
	W->SetBossStatus(Data.Get(), 300, 600);
	TestTrue(TEXT("Editor brush tint reaches actual original material parameter"), MID->K2_GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA")).Equals(OriginalColor * Tint));
	W->SetBossStatus(Data.Get(), 150, 600);
	TestTrue(TEXT("Repeated refresh never compounds tint"), MID->K2_GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA")).Equals(OriginalColor * Tint));
	FLinearColor SourceAfter;
	Fill->GetVectorParameterValue(TEXT("Bar_Health_Fill_RGBA"), SourceAfter);
	TestTrue(TEXT("Tint leaves source color unchanged"), SourceAfter.Equals(OriginalColor));
	FDCBossHUDTestAccess::Destruct(W.Get());
	TestNull(TEXT("Destruct clears runtime fill reference"), FDCBossHUDTestAccess::FillMID(W.Get()));
	TestNull(TEXT("Destruct clears runtime border reference"), FDCBossHUDTestAccess::BorderMID(W.Get()));
	TestNull(TEXT("Destruct releases image MID brush"), FDCBossHUDTestAccess::Fill(W.Get())->GetBrush().GetResourceObject());
	FDCBossHUDTestAccess::Construct(W.Get());
	TestNotNull(TEXT("Reattachment restores material"), FDCBossHUDTestAccess::FillMID(W.Get()));
	Slate.Reset(); W->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

#endif
