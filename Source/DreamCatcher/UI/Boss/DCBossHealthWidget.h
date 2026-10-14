#pragma once

#include "Blueprint/UserWidget.h"
#include "DCBossHealthWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** R12 boss-target display; values come from the existing Lyra health delegates. */
UCLASS()
class DREAMCATCHER_API UDCBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetBossStatus(const FText& Name, float Health, float MaxHealth, int32 Phase, bool bTransitioning);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	void Refresh();
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Title;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> Bar;
	FText Caption;
	float Fraction = 1.f;
};
