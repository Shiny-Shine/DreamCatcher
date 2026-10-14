#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCEncounterController.generated.h"

class ADCEnemySpawner;
class ADCEncounterController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCEncounterCompletedSignature, ADCEncounterController*, Encounter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCEncounterFailedSignature, ADCEncounterController*, Encounter);

UCLASS()
class DREAMCATCHER_API ADCEncounterController : public AActor
{
	GENERATED_BODY()

public:
	ADCEncounterController();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCEncounterCompletedSignature OnEncounterCompleted;
	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCEncounterFailedSignature OnEncounterFailed;

	UFUNCTION(BlueprintCallable, Category="Encounter")
	void StartEncounter();
	/** Explicit stage cancellation; existing EndPlay behavior stays unchanged. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Encounter")
	void AbortEncounter();
	UFUNCTION(BlueprintPure, Category="Encounter")
	bool IsEncounterAborted() const { return bAborted; }
	UFUNCTION(BlueprintPure, Category="Encounter")
	bool IsEncounterCompleted() const { return bCompleted; }
	UFUNCTION(BlueprintPure, Category="Encounter")
	bool HasEncounterFailure() const { return bFailed; }
	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetRemainingSpawnerCount() const { return PendingSpawners.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	bool bAutoStartOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="0.0", Units="s"))
	float StartDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<TObjectPtr<ADCEnemySpawner>> Spawners;

	UFUNCTION(BlueprintImplementableEvent, Category="Encounter")
	void BP_OnEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category="Encounter")
	void BP_OnEncounterCompleted();

private:
	void StartEncounterInternal();
	void TryCompleteEncounter();
	void FailEncounter();
	void UnbindSpawner(ADCEnemySpawner* Spawner);

	UFUNCTION()
	void HandleSpawnerCleared(ADCEnemySpawner* ClearedSpawner);
	UFUNCTION()
	void HandleSpawnerFailed(ADCEnemySpawner* FailedSpawner);
	UFUNCTION()
	void HandleSpawnerDestroyed(AActor* DestroyedSpawner);

	TSet<TWeakObjectPtr<ADCEnemySpawner>> PendingSpawners;
	bool bStartRequested = false;
	bool bStarted = false;
	bool bCompleted = false;
	bool bFailed = false;
	bool bStartingSpawners = false;
	bool bEndingPlay = false;
	bool bAborted = false;
	FTimerHandle StartTimerHandle;
};
