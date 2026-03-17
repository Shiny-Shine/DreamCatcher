#include "Stage/DCEncounterController.h"
#include "AI/DCEnemySpawner.h"
#include "TimerManager.h"

ADCEncounterController::ADCEncounterController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADCEncounterController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartOnBeginPlay)
	{
		StartEncounter();
	}
}

void ADCEncounterController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimerHandle);

	for (ADCEnemySpawner* Spawner : Spawners)
	{
		if (Spawner)
		{
			Spawner->OnSpawnerCleared.RemoveDynamic(this, &ADCEncounterController::HandleSpawnerCleared);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ADCEncounterController::StartEncounter()
{
	if (bStarted || bCompleted)
	{
		return;
	}

	if (StartDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(StartTimerHandle, this, &ADCEncounterController::StartEncounterInternal, StartDelay, false);
	}
	else
	{
		StartEncounterInternal();
	}
}

void ADCEncounterController::StartEncounterInternal()
{
	if (bStarted || bCompleted)
	{
		return;
	}

	bStarted = true;
	RemainingSpawners = 0;

	for (ADCEnemySpawner* Spawner : Spawners)
	{
		if (!Spawner)
		{
			continue;
		}

		Spawner->OnSpawnerCleared.RemoveDynamic(this, &ADCEncounterController::HandleSpawnerCleared);
		Spawner->OnSpawnerCleared.AddDynamic(this, &ADCEncounterController::HandleSpawnerCleared);

		++RemainingSpawners;
		Spawner->ActivateSpawner();
	}

	BP_OnEncounterStarted();

	// 연결된 스포너가 하나도 없으면 즉시 완료 처리.
	if (RemainingSpawners == 0)
	{
		bCompleted = true;
		BP_OnEncounterCompleted();
		OnEncounterCompleted.Broadcast(this);
	}
}

void ADCEncounterController::HandleSpawnerCleared(ADCEnemySpawner* ClearedSpawner)
{
	if (ClearedSpawner)
	{
		ClearedSpawner->OnSpawnerCleared.RemoveDynamic(this, &ADCEncounterController::HandleSpawnerCleared);
	}

	RemainingSpawners = FMath::Max(0, RemainingSpawners - 1);

	if (RemainingSpawners == 0 && !bCompleted)
	{
		bCompleted = true;
		BP_OnEncounterCompleted();
		OnEncounterCompleted.Broadcast(this);
	}
}