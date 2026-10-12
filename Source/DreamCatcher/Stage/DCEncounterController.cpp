#include "Stage/DCEncounterController.h"
#include "AI/DCEnemySpawner.h"
#include "Engine/World.h"
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
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(StartTimerHandle);
	for (const TWeakObjectPtr<ADCEnemySpawner>& Spawner : PendingSpawners)
	{
		UnbindSpawner(Spawner.Get());
	}
	PendingSpawners.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADCEncounterController::StartEncounter()
{
	if (bStartRequested || bStarted || bCompleted || bFailed || bEndingPlay || bAborted)
	{
		return;
	}

	bStartRequested = true;
	if (!FMath::IsFinite(StartDelay) || StartDelay < 0.0f) { FailEncounter(); return; }
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
	if (bStarted || bCompleted || bFailed || bEndingPlay || bAborted)
	{
		return;
	}

	bStarted = true;
	bStartingSpawners = true;
	PendingSpawners.Reset();

	// Register the complete, unique set before activating any spawner. Activation
	// can synchronously clear an empty spawner or report a configuration failure.
	for (ADCEnemySpawner* Spawner : Spawners)
	{
		if (!Spawner) { continue; } // Preserve intentionally empty array entries.
		if (!IsValid(Spawner) || Spawner->GetWorld() != GetWorld() || Spawner->IsActorBeingDestroyed() || Spawner->HasSpawnFailure())
		{
			FailEncounter();
			bStartingSpawners = false;
			return;
		}
		if (Spawner->IsSpawnerCleared() || PendingSpawners.Contains(Spawner)) { continue; }
		PendingSpawners.Add(Spawner);
		Spawner->OnSpawnerCleared.AddUniqueDynamic(this, &ThisClass::HandleSpawnerCleared);
		Spawner->OnSpawnerFailed.AddUniqueDynamic(this, &ThisClass::HandleSpawnerFailed);
		Spawner->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleSpawnerDestroyed);
	}

	// Snapshot configuration order; callbacks can modify the pending set.
	TSet<TWeakObjectPtr<ADCEnemySpawner>> Activated;
	const TArray<TObjectPtr<ADCEnemySpawner>> ConfiguredSpawners = Spawners;
	for (ADCEnemySpawner* Spawner : ConfiguredSpawners)
	{
		if (bEndingPlay || bFailed) { break; }
		if (IsValid(Spawner) && PendingSpawners.Contains(Spawner) && !Activated.Contains(Spawner))
		{
			Activated.Add(Spawner);
			Spawner->ActivateSpawner();
		}
	}
	if (!bEndingPlay && !bFailed) { BP_OnEncounterStarted(); }
	bStartingSpawners = false;
	TryCompleteEncounter();
}

void ADCEncounterController::HandleSpawnerCleared(ADCEnemySpawner* ClearedSpawner)
{
	if (!bStarted || bCompleted || bFailed || bEndingPlay || bAborted || !IsValid(ClearedSpawner)
		|| !ClearedSpawner->IsSpawnerCleared() || ClearedSpawner->HasSpawnFailure()
		|| PendingSpawners.Remove(ClearedSpawner) == 0)
	{
		return;
	}
	UnbindSpawner(ClearedSpawner);
	TryCompleteEncounter();
}

void ADCEncounterController::TryCompleteEncounter()
{
	if (!bStarted || bCompleted || bFailed || bEndingPlay || bAborted || bStartingSpawners || PendingSpawners.Num() != 0) { return; }
	bCompleted = true;
	BP_OnEncounterCompleted();
	if (!bEndingPlay) { OnEncounterCompleted.Broadcast(this); }
}

void ADCEncounterController::HandleSpawnerFailed(ADCEnemySpawner* FailedSpawner)
{
	if (FailedSpawner && FailedSpawner->HasSpawnFailure() && PendingSpawners.Contains(FailedSpawner)) { FailEncounter(); }
}

void ADCEncounterController::HandleSpawnerDestroyed(AActor* DestroyedSpawner)
{
	if (PendingSpawners.Contains(Cast<ADCEnemySpawner>(DestroyedSpawner))) { FailEncounter(); }
}

void ADCEncounterController::UnbindSpawner(ADCEnemySpawner* Spawner)
{
	if (Spawner)
	{
		Spawner->OnSpawnerCleared.RemoveDynamic(this, &ThisClass::HandleSpawnerCleared);
		Spawner->OnSpawnerFailed.RemoveDynamic(this, &ThisClass::HandleSpawnerFailed);
		Spawner->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnerDestroyed);
	}
}

void ADCEncounterController::FailEncounter()
{
	if (bCompleted || bFailed || bEndingPlay || GetWorld()->bIsTearingDown) { return; }
	bFailed = true;
	GetWorldTimerManager().ClearTimer(StartTimerHandle);
	for (const TWeakObjectPtr<ADCEnemySpawner>& Spawner : PendingSpawners) { UnbindSpawner(Spawner.Get()); }
	PendingSpawners.Reset();
	OnEncounterFailed.Broadcast(this);
}

void ADCEncounterController::AbortEncounter()
{
	if (bEndingPlay || bAborted || !GetWorld() || !GetWorld()->IsGameWorld() || !HasAuthority() || GetNetMode() == NM_Client) { return; }
	bAborted = true;
	GetWorldTimerManager().ClearTimer(StartTimerHandle);
	// Completed spawners can still own Pawns finishing the death sequence.
	TSet<TWeakObjectPtr<ADCEnemySpawner>> Owned;
	for (ADCEnemySpawner* Spawner : Spawners)
	{
		if (IsValid(Spawner) && Spawner->GetWorld() == GetWorld()) { UnbindSpawner(Spawner); Owned.Add(Spawner); }
	}
	PendingSpawners.Reset();
	const bool bNotifyFailure = !bCompleted && !bFailed;
	if (!bCompleted) { bFailed = true; }
	for (const TWeakObjectPtr<ADCEnemySpawner>& Spawner : Owned)
	{
		if (ADCEnemySpawner* Actor = Spawner.Get(); Actor && !Actor->IsActorBeingDestroyed())
		{
			// Finite Lyra EndPlay defers only its own Pawn/Controller cleanup safely.
			Actor->Destroy();
		}
	}
	if (bNotifyFailure && !bEndingPlay && !GetWorld()->bIsTearingDown) { OnEncounterFailed.Broadcast(this); }
}
