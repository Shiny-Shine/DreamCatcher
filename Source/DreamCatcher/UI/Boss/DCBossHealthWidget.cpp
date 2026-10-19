#include "UI/Boss/DCBossHealthWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "Styling/WidgetStyle.h"

TSharedRef<SWidget> UDCBossHealthWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BossRoot"));
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		SetIsFocusable(false);
		Panel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BossPanel"));
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
		PanelSlot->SetAnchors(FAnchors(.5f, 0.f)); PanelSlot->SetAlignment(FVector2D(.5f, 0.f));
		Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BossName"));
		SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BossSubtitle"));
		for (UTextBlock* Text : { Title.Get(), SubtitleText.Get() })
		{
			UCanvasPanelSlot* TextSlot = Panel->AddChildToCanvas(Text);
			TextSlot->SetAnchors(FAnchors(.5f, 0.f)); TextSlot->SetAlignment(FVector2D(.5f, 0.f));
			Text->SetJustification(ETextJustify::Center); Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		BarPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BossBar"));
		UCanvasPanelSlot* BarSlot = Panel->AddChildToCanvas(BarPanel);
		BarSlot->SetAnchors(FAnchors(.5f, 0.f)); BarSlot->SetAlignment(FVector2D(.5f, 0.f));
		BorderImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossBarBorder"));
		FillImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossBarFill"));
		for (UImage* Image : { BorderImage.Get(), FillImage.Get() })
		{
			UCanvasPanelSlot* ImageSlot = BarPanel->AddChildToCanvas(Image);
			ImageSlot->SetAnchors(FAnchors(0, 0, 1, 1)); ImageSlot->SetOffsets(FMargin(0));
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		FillImage->SetRenderTransformPivot(FVector2D(0, .5f));
	}
	Refresh();
	return Super::RebuildWidget();
}

bool UDCBossHealthWidget::IsStyleValid(const FDCBossHUDStyle& Value)
{
	auto FiniteColor = [](const FLinearColor& C)
	{ return FMath::IsFinite(C.R) && FMath::IsFinite(C.G) && FMath::IsFinite(C.B) && FMath::IsFinite(C.A); };
	return !Value.PanelSize.ContainsNaN() && Value.PanelSize.X > 0 && Value.PanelSize.Y > 0
		&& !Value.PanelOffset.ContainsNaN() && !Value.NameOffset.ContainsNaN()
		&& !Value.SubtitleOffset.ContainsNaN() && !Value.BarOffset.ContainsNaN()
		&& !Value.BarSize.ContainsNaN() && Value.BarSize.X > 0 && Value.BarSize.Y > 0
		&& Value.BarSize.X <= Value.PanelSize.X && Value.BarSize.Y <= Value.PanelSize.Y
		&& Value.NameFontSize >= 1 && Value.NameFontSize <= 200
		&& Value.SubtitleFontSize >= 1 && Value.SubtitleFontSize <= 200
		&& FiniteColor(Value.NameColor) && FiniteColor(Value.SubtitleColor);
}

void UDCBossHealthWidget::SetBossStatus(const UDCBossData* Data, float Health, float MaxHealth)
{
	const UDCBossData* Defaults = Data ? Data : GetDefault<UDCBossData>();
	BossName = Defaults->DisplayName; BossSubtitle = Defaults->Subtitle;
	Appearance = IsStyleValid(Defaults->HUDStyle) ? Defaults->HUDStyle : FDCBossHUDStyle();
	Fraction = FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) && MaxHealth > 0.f
		? static_cast<float>(FMath::Clamp(static_cast<double>(Health) / MaxHealth, 0.0, 1.0)) : 0.f;
	Refresh();
}

void UDCBossHealthWidget::ApplyBrush(UImage* Image, const FSlateBrush& Source,
	TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& SourceMaterial)
{
	FSlateBrush DrawBrush = Source;
	UMaterialInterface* Material = Cast<UMaterialInterface>(Source.GetResourceObject());
	if (Material)
	{
		if (!MID || SourceMaterial != Material)
		{
			MID = UMaterialInstanceDynamic::Create(Material, this);
			MID->SetFlags(RF_Transient); SourceMaterial = Material;
		}
		// Original W_Healthbar UImage/MID contract. No ProgressBar fill clipping.
		MID->SetScalarParameterValue(TEXT("Health_Current"), Fraction);
		MID->SetScalarParameterValue(TEXT("Health_Updated"), Fraction);
		// UI material emissive/opacity do not automatically receive a final
		// VertexColor multiply. These are the actual original health-bar vector
		// names. Read the unchanged source each time, avoiding cumulative tint.
		const FLinearColor Tint = Source.GetTint(FWidgetStyle());
		for (const TCHAR* Name : { TEXT("Bar_Health_Fill_RGBA"), TEXT("Bar_Health_Highlight_RGBA"),
			TEXT("Bar_Damage_Fill_RGBA"), TEXT("Bar_Damage_Highlight_RGBA"), TEXT("Glow_Health_RGBA"),
			TEXT("Glow_Damage_RGBA"), TEXT("Outline_Damaged_RGBA"), TEXT("Glow_Healing_RGBA"),
			TEXT("Bar_Healing_Fill_RGBA"), TEXT("Bar_Healing_Highlight_RGBA"), TEXT("Outline_Healing_RGBA"),
			TEXT("Glow_Simple_Active_RGBA"), TEXT("Glow_Simple_Inactive_RGBA"), TEXT("Outline_RGB"), TEXT("Fill_RGB") })
		{
			FLinearColor Original;
			if (Material->GetVectorParameterValue(Name, Original)) { MID->SetVectorParameterValue(Name, Original * Tint); }
		}
		DrawBrush.SetResourceObject(MID);
		DrawBrush.TintColor = FSlateColor(FLinearColor::White);
	}
	else { MID = nullptr; SourceMaterial = nullptr; }
	Image->SetBrush(DrawBrush);
}

void UDCBossHealthWidget::Refresh()
{
	if (!Panel || !BarPanel || !Title || !SubtitleText || !BorderImage || !FillImage) { return; }
	UCanvasPanelSlot* PanelSlot = CastChecked<UCanvasPanelSlot>(Panel->Slot);
	PanelSlot->SetPosition(Appearance.PanelOffset); PanelSlot->SetSize(Appearance.PanelSize);
	UCanvasPanelSlot* BarSlot = CastChecked<UCanvasPanelSlot>(BarPanel->Slot);
	BarSlot->SetPosition(Appearance.BarOffset); BarSlot->SetSize(Appearance.BarSize);
	auto SetText = [this](UTextBlock* Text, const FText& Value, int32 FontSize, FLinearColor Color, FVector2D Offset)
	{
		Text->SetText(Value); Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FontSize));
		Text->SetColorAndOpacity(FSlateColor(Color));
		UCanvasPanelSlot* TextSlot = CastChecked<UCanvasPanelSlot>(Text->Slot);
		TextSlot->SetPosition(Offset); TextSlot->SetSize(FVector2D(Appearance.PanelSize.X, FontSize + 8));
	};
	SetText(Title, BossName, Appearance.NameFontSize, Appearance.NameColor, Appearance.NameOffset);
	SetText(SubtitleText, BossSubtitle, Appearance.SubtitleFontSize, Appearance.SubtitleColor, Appearance.SubtitleOffset);
	ApplyBrush(BorderImage, Appearance.BarBorderBrush, BorderMID, BorderSource);
	ApplyBrush(FillImage, Appearance.BarFillBrush, FillMID, FillSource);
	// A material always gets the full area and performs its own fill. An
	// untextured/texture fallback can still show a safe fraction without an MID.
	FillImage->SetRenderScale(FVector2D(FillMID ? 1.f : Fraction, 1.f));
}

void UDCBossHealthWidget::NativeDestruct()
{
	if (BorderImage) { BorderImage->SetBrush(FSlateBrush()); }
	if (FillImage) { FillImage->SetBrush(FSlateBrush()); }
	BorderMID = nullptr; FillMID = nullptr; BorderSource = nullptr; FillSource = nullptr;
	Super::NativeDestruct();
}

void UDCBossHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

int32 UDCBossHealthWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	static const bool bDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("DCBossHUDPaintDiagnostics"));
	if (!bDiagnostics || GFrameCounter % 8 != 0 || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !Panel || !BarPanel || !Title || !SubtitleText || !BorderImage || !FillImage) { return Layer; }
	const FGeometry PanelGeometry = Panel->GetPaintSpaceGeometry();
	const FGeometry BarGeometry = BarPanel->GetPaintSpaceGeometry();
	const FVector2D RootSize = Geometry.GetLocalSize(), PanelSize = PanelGeometry.GetLocalSize(), BarSize = BarGeometry.GetLocalSize();
	const FVector2D BorderSize = BorderImage->GetPaintSpaceGeometry().GetLocalSize();
	const FVector2D FillSize = FillImage->GetPaintSpaceGeometry().GetLocalSize();
	if (RootSize.ContainsNaN() || PanelSize.ContainsNaN() || BarSize.ContainsNaN() || RootSize.X <= 0 || RootSize.Y <= 0
		|| PanelSize.X <= 0 || PanelSize.Y <= 0 || BarSize.X <= 0 || BarSize.Y <= 0 || BorderSize.X <= 0 || FillSize.X <= 0) { return Layer; }
	const FVector2D PanelOrigin = PanelGeometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D Position = Geometry.AbsoluteToLocal(PanelOrigin);
	const FVector2D BarPosition = PanelGeometry.AbsoluteToLocal(BarGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	auto Escape = [](const FString& Value)
	{ return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")); };
	UE_LOG(LogTemp, Display, TEXT("DCBOSS_HUD_PAINT {\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"root_size\":[%.6f,%.6f],\"panel_position\":[%.6f,%.6f],\"panel_size\":[%.6f,%.6f],\"bar_position\":[%.6f,%.6f],\"bar_size\":[%.6f,%.6f],\"border_size\":[%.6f,%.6f],\"fill_size\":[%.6f,%.6f],\"fraction\":%.6f,\"name\":\"%s\",\"subtitle\":\"%s\",\"border_material\":\"%s\",\"fill_material\":\"%s\"}"),
		GFrameCounter, *Escape(GetWorld()->GetPathName()), *Escape(GetPathName()), RootSize.X, RootSize.Y,
		Position.X, Position.Y, PanelSize.X, PanelSize.Y, BarPosition.X, BarPosition.Y, BarSize.X, BarSize.Y,
		BorderSize.X, BorderSize.Y, FillSize.X, FillSize.Y, Fraction, *Escape(Title->GetText().ToString()),
		*Escape(SubtitleText->GetText().ToString()), *Escape(GetPathNameSafe(BorderSource)), *Escape(GetPathNameSafe(FillSource)));
	return Layer;
}
