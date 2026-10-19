#pragma once

#include "Blueprint/UserWidget.h"
#include "AI/Boss/DCBossData.h"
#include "DCBossHealthWidget.generated.h"

class UCanvasPanel;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;

/** Boss presentation only; combat phase and health rules stay on the boss. */
UCLASS()
class DREAMCATCHER_API UDCBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Boss|HUD")
	void SetBossStatus(const UDCBossData* Data, float Health, float MaxHealth);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	static bool IsStyleValid(const FDCBossHUDStyle& Value);
	void Refresh();
	void ApplyBrush(UImage* Image, const FSlateBrush& Source, TObjectPtr<UMaterialInstanceDynamic>& MID,
		TObjectPtr<UMaterialInterface>& SourceMaterial);
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> BarPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UImage> BorderImage;
	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BorderMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FillMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> BorderSource;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> FillSource;
	UPROPERTY(Transient) FDCBossHUDStyle Appearance;
	FText BossName;
	FText BossSubtitle;
	float Fraction = 0.f;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBossHUDTestAccess;
#endif
};
