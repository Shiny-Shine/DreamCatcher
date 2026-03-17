#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCEncounterController.generated.h"

class ADCEnemySpawner;
class ADCEncounterController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCEncounterCompletedSignature, ADCEncounterController*, Encounter);

UCLASS()
class DREAMCATCHER_API ADCEncounterController : public AActor
{
	GENERATED_BODY()

public:
	ADCEncounterController();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCEncounterCompletedSignature OnEncounterCompleted;

	UFUNCTION(BlueprintCallable, Category="Encounter")
	void StartEncounter();

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

	UFUNCTION()
	void HandleSpawnerCleared(ADCEnemySpawner* ClearedSpawner);

	int32 RemainingSpawners = 0;
	bool bStarted = false;
	bool bCompleted = false;
	FTimerHandle StartTimerHandle;
};