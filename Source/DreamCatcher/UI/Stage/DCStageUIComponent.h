#pragma once

#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "DCStageUIComponent.generated.h"

class ADCStageDirector;
class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;
class UDCAbilitySystemComponent;
class UDCStageResultLayer;
class UDCMissionClearWidget;
class UPrimaryGameLayout;

/** Local, opt-in R13 layer/input lifecycle. Existing R11/R12 do not add this component. */
UCLASS()
class DREAMCATCHER_API UDCStageUIComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDCStageUIComponent();
	UFUNCTION(BlueprintCallable, Category="Stage|UI")
	bool ReturnFromMenu();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void ConnectLayers();
	void LayerChanged(UCommonActivatableWidget* Widget);
	void SynchronizeInput();
	void SetMenuInputBlocked(bool bBlock);
	void ReleaseInput();
	void PresentResult();
	void CancelUnappliedSettings();
	UFUNCTION() void StageEnded(ADCStageDirector* Director, bool bSucceeded);
	TWeakObjectPtr<UPrimaryGameLayout> Root;
	TWeakObjectPtr<UCommonActivatableWidgetContainerBase> Menu;
	TWeakObjectPtr<UCommonActivatableWidgetContainerBase> Modal;
	TWeakObjectPtr<ADCStageDirector> Stage;
	TWeakObjectPtr<UDCAbilitySystemComponent> BlockedASC;
	UPROPERTY(Transient) TObjectPtr<UDCStageResultLayer> Result;
	UPROPERTY(Transient) TObjectPtr<UDCMissionClearWidget> MissionClear;
	FDelegateHandle MenuHandle;
	FDelegateHandle ModalHandle;
	FTimerHandle ConnectTimer;
	FTimerHandle ResultTimer;
	bool bOwnsIgnore = false;
	bool bEnding = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCStageUITestAccess;
#endif
};
