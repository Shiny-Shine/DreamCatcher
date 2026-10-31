#include "UI/Boss/DCBossHealthWidget.h"
#include "AI/Boss/DCBossData.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/WidgetStyle.h"

void UDCBossHealthWidget::SetBossStatus(const UDCBossData* Data, float Health, float MaxHealth)
{
	const UDCBossData* Defaults = Data ? Data : GetDefault<UDCBossData>();
	CurrentBossName = Defaults->DisplayName; CurrentBossSubtitle = Defaults->Subtitle;
	Fraction = FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) && MaxHealth > 0.f
		? static_cast<float>(FMath::Clamp(static_cast<double>(Health) / MaxHealth, 0.0, 1.0)) : 0.f;
	Refresh();
}

void UDCBossHealthWidget::CacheAuthoredBrushes()
{
	if (bBrushesCached || !BossBarBorder || !BossBarFill) { return; }
	AuthoredBorderBrush = BossBarBorder->GetBrush();
	AuthoredFillBrush = BossBarFill->GetBrush();
	AuthoredFillScale = BossBarFill->GetRenderTransform().Scale;
	bBrushesCached = true;
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
	if (!BossPanel || !BossBar || !BossName || !BossSubtitle || !BossBarBorder || !BossBarFill) { return; }
	CacheAuthoredBrushes();
	BossName->SetText(CurrentBossName); BossSubtitle->SetText(CurrentBossSubtitle);
	ApplyBrush(BossBarBorder, AuthoredBorderBrush, BorderMID, BorderSource);
	ApplyBrush(BossBarFill, AuthoredFillBrush, FillMID, FillSource);
	BossBarFill->SetRenderScale(FVector2D(AuthoredFillScale.X * (FillMID ? 1.f : Fraction), AuthoredFillScale.Y));
}

void UDCBossHealthWidget::NativeDestruct()
{
	if (bBrushesCached)
	{
		if (BossBarBorder) { BossBarBorder->SetBrush(AuthoredBorderBrush); }
		if (BossBarFill) { BossBarFill->SetBrush(AuthoredFillBrush); BossBarFill->SetRenderScale(AuthoredFillScale); }
	}
	BorderMID = nullptr; FillMID = nullptr; BorderSource = nullptr; FillSource = nullptr;
	bBrushesCached = false;
	Super::NativeDestruct();
}

void UDCBossHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible);
	Refresh();
}

int32 UDCBossHealthWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	static const bool bDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("DCBossHUDPaintDiagnostics"));
	if (!bDiagnostics || GFrameCounter % 8 != 0 || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !BossPanel || !BossBar || !BossName || !BossSubtitle || !BossBarBorder || !BossBarFill) { return Layer; }
	const FGeometry PanelGeometry = BossPanel->GetPaintSpaceGeometry();
	const FGeometry BarGeometry = BossBar->GetPaintSpaceGeometry();
	const FVector2D RootSize = Geometry.GetLocalSize(), PanelSize = PanelGeometry.GetLocalSize(), BarSize = BarGeometry.GetLocalSize();
	const FVector2D BorderSize = BossBarBorder->GetPaintSpaceGeometry().GetLocalSize();
	const FVector2D FillSize = BossBarFill->GetPaintSpaceGeometry().GetLocalSize();
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
		BorderSize.X, BorderSize.Y, FillSize.X, FillSize.Y, Fraction, *Escape(BossName->GetText().ToString()),
		*Escape(BossSubtitle->GetText().ToString()), *Escape(GetPathNameSafe(BorderSource)), *Escape(GetPathNameSafe(FillSource)));
	return Layer;
}
