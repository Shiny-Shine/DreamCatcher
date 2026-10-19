#pragma once

#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "DCBossData.generated.h"

class UMaterialInterface;

/** Presentation only: the HUD handles bad values without failing boss combat. */
USTRUCT(BlueprintType)
struct FDCBossHUDStyle
{
	GENERATED_BODY()
	FDCBossHUDStyle()
	{
		// Original W_Healthbar BarBorder / BarFill brush geometry.
		BarBorderBrush.DrawAs = ESlateBrushDrawType::Box;
		BarBorderBrush.SetImageSize(FVector2D(24, 24));
		BarBorderBrush.Margin = FMargin(.5f);
		BarFillBrush.DrawAs = ESlateBrushDrawType::Box;
		BarFillBrush.SetImageSize(FVector2D(12, 12));
		BarFillBrush.Margin = FMargin(.5f);
	}
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D PanelSize = FVector2D(720, 86);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D PanelOffset = FVector2D(0, 88);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D NameOffset = FVector2D(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D SubtitleOffset = FVector2D(0, 32);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D BarOffset = FVector2D(0, 62);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	FVector2D BarSize = FVector2D(640, 18);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text", meta=(ClampMin="1", ClampMax="200"))
	int32 NameFontSize = 24;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text", meta=(ClampMin="1", ClampMax="200"))
	int32 SubtitleFontSize = 12;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FLinearColor NameColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FLinearColor SubtitleColor = FLinearColor(.8f, .8f, .8f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bar")
	FSlateBrush BarBorderBrush;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bar")
	FSlateBrush BarFillBrush;
};

/** R12 tuning, not final BIG BROTHER design. Attack geometry lives on the granted GAs. */
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCBossData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss")
	FText DisplayName = NSLOCTEXT("DCBoss", "WireframeName", "춘식이");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|HUD")
	FText Subtitle = NSLOCTEXT("DCBoss", "WireframeSubtitle", "나스닥의 수호자");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|HUD")
	FDCBossHUDStyle HUDStyle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1"))
	float MaxHealth = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.01", ClampMax="0.99"))
	float PhaseTwoHealthRatio = .5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float OpeningDelay = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float TransitionDuration = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float PhaseOneGap = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float PhaseTwoGap = .35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Presentation")
	TObjectPtr<UMaterialInterface> WarningMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Presentation")
	TObjectPtr<UMaterialInterface> ImpactMaterial;

	bool IsValidConfiguration() const;
};
