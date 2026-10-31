#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "DCBossHealthWidget.generated.h"

class UCanvasPanel;
class UDCBossData;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;

/** Designer owns the layout and source brushes; the boss owns combat state. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Boss|HUD")
	void SetBossStatus(const UDCBossData* Data, float Health, float MaxHealth);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	void Refresh();
	void CacheAuthoredBrushes();
	void ApplyBrush(UImage* Image, const FSlateBrush& Source, TObjectPtr<UMaterialInstanceDynamic>& MID,
		TObjectPtr<UMaterialInterface>& SourceMaterial);
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> BossPanel;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> BossBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BossName;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BossSubtitle;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> BossBarBorder;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> BossBarFill;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BorderMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FillMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> BorderSource;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> FillSource;
	UPROPERTY(Transient) FSlateBrush AuthoredBorderBrush;
	UPROPERTY(Transient) FSlateBrush AuthoredFillBrush;
	FVector2D AuthoredFillScale = FVector2D(1.f, 1.f);
	FText CurrentBossName;
	FText CurrentBossSubtitle;
	float Fraction = 0.f;
	bool bBrushesCached = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBossHUDTestAccess;
#endif
};
