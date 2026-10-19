#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCEnemySpawner.generated.h"

class UArrowComponent;
class USceneComponent;
class ADCGASEnemyCharacter;
class ADCEnemySpawner;
class ADCLyraPlayerBotController;
class AController;
class UDCLyraExperienceDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCSpawnerClearedSignature, ADCEnemySpawner*, Spawner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCSpawnerFailedSignature, ADCEnemySpawner*, Spawner);

UCLASS()
class DREAMCATCHER_API ADCEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ADCEnemySpawner();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCSpawnerClearedSignature OnSpawnerCleared;
	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCSpawnerFailedSignature OnSpawnerFailed;

	UFUNCTION(BlueprintCallable, Category="Spawner")
	void ActivateSpawner();

	UFUNCTION(BlueprintPure, Category="Spawner")
	int32 GetAliveCount() const { return LivingEnemies.Num(); }
	UFUNCTION(BlueprintPure, Category="Spawner")
	int32 GetSpawnedCount() const { return SpawnedCount; }
	UFUNCTION(BlueprintPure, Category="Spawner|Mission")
	int32 GetDefeatedCount() const { return DefeatedCount; }
	/** Includes not-yet-spawned enemies; an unconfigured optional spawner plans zero. */
	UFUNCTION(BlueprintPure, Category="Spawner|Mission")
	int32 GetPlannedEnemyCount() const;
	UFUNCTION(BlueprintPure, Category="Spawner")
	bool IsSpawnerCleared() const { return bIsCleared; }
	UFUNCTION(BlueprintPure, Category="Spawner")
	bool HasSpawnFailure() const { return bSpawnFailed; }
	UFUNCTION(BlueprintPure, Category="Spawner|Lyra")
	int32 GetPendingLyraCleanupCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> SpawnPoint;

	/** GAS NPC route, used by the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner|GAS")
	TSubclassOf<ADCGASEnemyCharacter> GASEnemyClass;

	/** R12 opt-in: retain GAS NPC ownership after counting death, and clean up on abort. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner|GAS")
	bool bCleanupOwnedGASNPCs = false;

	/** Finite Lyra route, ahead of the GAS NPC route. PawnData comes from the Experience.
	 * Reuses the original bot controller BP (Inventory/QuickBar/BT); no respawn policy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner|Lyra")
	TSubclassOf<ADCLyraPlayerBotController> LyraBotControllerClass;

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
	void TrackEnemy(AActor* Enemy);
	void UnbindEnemy(AActor* Enemy);
	void ConsumeEnemyDeath(AActor* Enemy);
	void FailSpawner(const TCHAR* Reason);
	void OnLyraExperienceLoaded(const UDCLyraExperienceDefinition* Experience);
	void SpawnLyraEnemy();
	void TrackLyraEnemy(AActor* Enemy, ADCLyraPlayerBotController* Controller);
	void ReleaseLyraController(ADCLyraPlayerBotController* Controller);
	void RemoveLyraEnemy(AActor* Enemy, bool bDestroyPawn);
	void CleanupOwnedLyraBots();
	void CleanupOwnedGASNPCs();
	UFUNCTION()
	void HandleOwnedGASDeathFinished(AActor* Enemy);

	UFUNCTION()
	void HandleGASEnemyDeath(ADCGASEnemyCharacter* DeadEnemy);
	UFUNCTION()
	void HandleTrackedEnemyDestroyed(AActor* Enemy);
	UFUNCTION()
	void HandleLyraDeathStarted(AActor* Enemy);
	UFUNCTION()
	void HandleLyraDeathFinished(AActor* Enemy);
	UFUNCTION()
	void HandleLyraControllerDestroyed(AActor* Controller);

	int32 SpawnedCount = 0;
	int32 DefeatedCount = 0;
	TSet<TWeakObjectPtr<AActor>> LivingEnemies;
	// Death consumes LivingEnemies; ownership survives until the original Pawn cleanup ends.
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<ADCLyraPlayerBotController>> LyraEnemies;
	TSet<TWeakObjectPtr<AActor>> FinishedLyraDeaths;
	TSet<TWeakObjectPtr<ADCLyraPlayerBotController>> OwnedLyraControllers;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AController>> OwnedGASNPCs;
	TSet<TWeakObjectPtr<AActor>> FinishedGASDeaths;
	bool bWaitingForLyraExperience = false;
	bool bIsActive = false;
	bool bIsCleared = false;
	bool bSpawnFailed = false;
	bool bSpawningEnemy = false;
	bool bEndingPlay = false;
	FTimerHandle SpawnTimerHandle;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCLyraFiniteSpawnerTestAccess;
	friend struct FDCMissionTestAccess;
#endif
};
