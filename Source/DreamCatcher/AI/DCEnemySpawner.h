#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCEnemySpawner.generated.h"

class UArrowComponent;
class USceneComponent;
class ADCEnemyCharacter;
class ADCEnemySpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCSpawnerClearedSignature, ADCEnemySpawner*, Spawner);

UCLASS()
class DREAMCATCHER_API ADCEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ADCEnemySpawner();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCSpawnerClearedSignature OnSpawnerCleared;

	UFUNCTION(BlueprintCallable, Category="Spawner")
	void ActivateSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	TSubclassOf<ADCEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="1"))
	int32 SpawnCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="1"))
	int32 MaxAliveAtOnce = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="0.0", Units="s"))
	float SpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	bool bActivateOnBeginPlay = false;

private:
	void TrySpawnNext();
	void SpawnEnemy();
	void ScheduleNextSpawn();
	void CompleteSpawner();

	UFUNCTION()
	void HandleSpawnedEnemyDeath(ADCEnemyCharacter* DeadEnemy);

	int32 SpawnedCount = 0;
	int32 AliveCount = 0;
	bool bIsActive = false;
	bool bIsCleared = false;
	FTimerHandle SpawnTimerHandle;
};