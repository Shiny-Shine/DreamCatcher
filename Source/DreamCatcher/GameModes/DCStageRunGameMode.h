#pragma once

#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Stage/DCStageDirector.h"
#include "DCStageRunGameMode.generated.h"

class UDCAbilitySystemComponent;
class UDCLyraHealthComponent;
class UDCStageResultWidget;

/** Opt-in, standalone R11 stage adapter. Original Lyra match modes are unchanged. */
UCLASS()
class DREAMCATCHER_API ADCStageRunGameMode : public ADCLyraGameMode
{
	GENERATED_BODY()
public:
	ADCStageRunGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	UFUNCTION(BlueprintPure, Category="Stage")
	ADCStageDirector* GetStageDirector() const { return Director.Get(); }
	UFUNCTION(BlueprintPure, Category="Stage")
	APlayerController* GetStagePlayerController() const { return RunPlayer.Get(); }
	UFUNCTION(BlueprintPure, Category="Stage")
	bool HasRunEnded() const { return bRunEnded; }
	UFUNCTION(BlueprintPure, Category="Stage")
	bool WasRunSuccessful() const { return bRunEnded && bRunSucceeded; }
	UFUNCTION(BlueprintPure, Category="Stage")
	UDCStageResultWidget* GetStageWidget() const { return StageWidget; }
	UFUNCTION(BlueprintCallable, Category="Stage")
	bool RequestRestartStage();
	UFUNCTION(BlueprintCallable, Category="Stage")
	bool RequestExitStage();
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PerformLevelTravel(FName LevelName);
	UPROPERTY(EditDefaultsOnly, Category="Stage|Teams", meta=(ClampMin="0", ClampMax="254"))
	int32 PlayerTeamId = 1;
	UPROPERTY(EditDefaultsOnly, Category="Stage|Teams", meta=(ClampMin="0", ClampMax="254"))
	int32 EnemyTeamId = 2;
	UPROPERTY(EditDefaultsOnly, Category="Stage|UI")
	TSubclassOf<UDCStageResultWidget> ResultWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category="Stage|UI")
	TSoftObjectPtr<UWorld> ReturnMap;
	UPROPERTY(EditDefaultsOnly, Category="Stage", meta=(ClampMin="1.0", Units="s"))
	float PlayerReadyTimeout = 15.0f;
private:
	void OnStageExperienceReady(const UDCLyraExperienceDefinition* Experience);
	void ApplyStageTeam(AController* Controller);
	void MaybeStartRun();
	void BindPlayerPawn(APawn* Pawn);
	void EndRun(bool bSucceeded);
	void PresentResult();
	void EnsureStageWidget();
	void CleanupPresentation();
	bool QueueTravel(FName LevelName);
	void ExecuteTravel();
	UFUNCTION()
	void HandlePlayerPawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION()
	void HandlePlayerDeath(AActor* Pawn);
	UFUNCTION()
	void HandlePhaseStarted(ADCStageDirector* Stage, int32 Index, const FText& Title, EDCStagePhaseType Type);
	UFUNCTION()
	void HandleStageEnded(ADCStageDirector* Stage, bool bSucceeded);
	UFUNCTION()
	void HandleDirectorDestroyed(AActor* Stage);
	UFUNCTION()
	void HandleRetryClicked();
	UFUNCTION()
	void HandleExitClicked();
	TWeakObjectPtr<ADCStageDirector> Director;
	TWeakObjectPtr<APlayerController> RunPlayer;
	TWeakObjectPtr<UDCLyraHealthComponent> PlayerHealth;
	TWeakObjectPtr<UDCAbilitySystemComponent> InputBlockedASC;
	UPROPERTY(Transient)
	TObjectPtr<UDCStageResultWidget> StageWidget;
	FText PhaseTitle;
	FName PendingTravel;
	FTimerHandle ReadyTimer;
	FTimerHandle ResultTimer;
	FTimerHandle TravelTimer;
	double ReadyDeadline = 0.0;
	bool bExperienceReady = false;
	bool bStartedRun = false;
	bool bRunEnded = false;
	bool bRunSucceeded = false;
	bool bEndingPlay = false;
	bool bTravelPending = false;
	bool bOwnsInputIgnore = false;
	bool bPreviousMouseCursor = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCStageFlowTestAccess;
#endif
};
