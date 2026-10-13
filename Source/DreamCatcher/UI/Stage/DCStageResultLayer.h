#pragma once

#include "UI/Lyra/DCLyraActivatableWidget.h"
#include "DCStageResultLayer.generated.h"

class ADCStageRunGameMode;
class UDCStageResultWidget;

/** R13 CommonUI layer wrapper; uses the accepted result buttons and stage travel API. */
UCLASS()
class DREAMCATCHER_API UDCStageResultLayer : public UDCLyraActivatableWidget
{
	GENERATED_BODY()
public:
	UDCStageResultLayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void BindStage(ADCStageRunGameMode* InMode);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override { return true; }
	virtual void NativeOnInitialized() override;
private:
	void Refresh();
	void ConsumeEscape() {}
	UFUNCTION() void Retry();
	UFUNCTION() void ExitStage();
	TWeakObjectPtr<ADCStageRunGameMode> Mode;
	UPROPERTY(Transient)
	TObjectPtr<UDCStageResultWidget> Content;
};
