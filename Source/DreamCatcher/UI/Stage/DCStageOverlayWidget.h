#pragma once

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "DCStageOverlayWidget.generated.h"

class ADCBossCharacter;
class UDCBossHealthWidget;
class UDCStageResultWidget;

/** Registered only in the R13 HUD extension slot. Owns no gameplay state. */
UCLASS()
class DREAMCATCHER_API UDCStageOverlayWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void Refresh();
	UPROPERTY(Transient) TObjectPtr<UDCStageResultWidget> StageDisplay;
	UPROPERTY(Transient) TObjectPtr<UDCBossHealthWidget> BossDisplay;
	TWeakObjectPtr<ADCBossCharacter> Boss;
	FTimerHandle UpdateTimer;
};
