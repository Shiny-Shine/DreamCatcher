#pragma once

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Stage/DCStageDirector.h"
#include "DCStageOverlayWidget.generated.h"

class ADCBossCharacter;
class ADCStageRunGameMode;
class APawn;
class APlayerController;
class UDCBossHealthWidget;
class UDCMissionObjectiveWidget;
class UDCRadarWidget;
class UDCStageResultWidget;
class USceneComponent;

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
	void RefreshMissions(const ADCStageRunGameMode* Mode);
	UDCRadarWidget* FindOwningHUDRadar() const;
	void ClearRadarBinding();
	void ConnectMissionSource(ADCStageDirector* Director);
	void DisconnectMissionSource();
	void MissionPhaseCompleted(ADCStageDirector* Director, const FDCMissionTransitionSnapshot& Snapshot);
	UFUNCTION() void MissionPhaseStarted(ADCStageDirector* Director, int32 Index, const FText& Title, EDCStagePhaseType Type);
	UFUNCTION() void MissionStageEnded(ADCStageDirector* Director, bool bSucceeded);
	UFUNCTION() void MissionSourceDestroyed(AActor* Director);
	UPROPERTY(Transient) TObjectPtr<UDCStageResultWidget> StageDisplay;
	UPROPERTY(Transient) TObjectPtr<UDCBossHealthWidget> BossDisplay;
	UPROPERTY(Transient) TObjectPtr<UDCMissionObjectiveWidget> MissionDisplay;
	TWeakObjectPtr<ADCBossCharacter> Boss;
	TWeakObjectPtr<UDCRadarWidget> ConnectedRadar;
	TWeakObjectPtr<USceneComponent> ConnectedObjective;
	TWeakObjectPtr<UWorld> MissionWorld;
	TWeakObjectPtr<ADCStageRunGameMode> MissionMode;
	TWeakObjectPtr<APlayerController> MissionController;
	TWeakObjectPtr<APawn> MissionPawn;
	TWeakObjectPtr<ADCStageDirector> MissionSource;
	FDelegateHandle MissionCompletedHandle;
	bool bOwnsRadarBinding = false;
	FTimerHandle UpdateTimer;
};
