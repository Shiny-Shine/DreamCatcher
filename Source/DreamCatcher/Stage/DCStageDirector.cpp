#include "Stage/DCStageDirector.h"
#include "Stage/DCEncounterController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DreamCatcher.h"

ADCStageDirector::ADCStageDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADCStageDirector::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStartStage) { StartStage(); }
}

void ADCStageDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	UnbindCurrentEncounter();
	if (RunState == EDCStageRunState::Running && HasAuthority() && !GetWorld()->bIsTearingDown)
	{
		AbortEncounters();
	}
	Super::EndPlay(EndPlayReason);
}

bool ADCStageDirector::ValidatePhases() const
{
	TSet<const ADCEncounterController*> Encounters;
	for (const FDCStagePhaseDefinition& Phase : Phases)
	{
		switch (Phase.PhaseType)
		{
		case EDCStagePhaseType::Delay:
			if (!FMath::IsFinite(Phase.PhaseDelay) || Phase.PhaseDelay < 0.0f) { return false; }
			break;
		case EDCStagePhaseType::Encounter:
		case EDCStagePhaseType::BossEncounter:
			if (!IsValid(Phase.Encounter) || Phase.Encounter->GetWorld() != GetWorld()
				|| Phase.Encounter->IsActorBeingDestroyed() || Encounters.Contains(Phase.Encounter)) { return false; }
			Encounters.Add(Phase.Encounter);
			break;
		case EDCStagePhaseType::Complete:
			break;
		default:
			return false;
		}
	}
	return true;
}

void ADCStageDirector::StartStage()
{
	if (bEndingPlay || RunState != EDCStageRunState::NotStarted || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !HasAuthority() || GetNetMode() == NM_Client) { return; }
	RunState = EDCStageRunState::Running;
	CurrentPhaseIndex = INDEX_NONE;
	if (!ValidatePhases())
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Invalid stage phase configuration: %s"), *GetName());
		FinishStage(false);
		return;
	}
	BP_OnStageStarted();
	if (!bEndingPlay && RunState == EDCStageRunState::Running) { AdvancePhase(); }
}

void ADCStageDirector::QueueNextPhase()
{
	if (bEndingPlay || RunState != EDCStageRunState::Running || bAdvanceQueued) { return; }
	bAdvanceQueued = true;
	UnbindCurrentEncounter();
	// Leave death/clear delegate stacks before starting another encounter or ending the run.
	PhaseTimerHandle = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::AdvancePhase);
}

void ADCStageDirector::AdvancePhase()
{
	if (bEndingPlay || RunState != EDCStageRunState::Running) { return; }
	bAdvanceQueued = false;
	UnbindCurrentEncounter();
	++CurrentPhaseIndex;
	if (!Phases.IsValidIndex(CurrentPhaseIndex)) { FinishStage(true); return; }
	StartCurrentPhase();
}

void ADCStageDirector::StartCurrentPhase()
{
	const FDCStagePhaseDefinition Phase = Phases[CurrentPhaseIndex];
	BP_OnStagePhaseStarted(CurrentPhaseIndex, Phase.PhaseTitle, Phase.PhaseType);
	if (bEndingPlay || RunState != EDCStageRunState::Running) { return; }
	OnStagePhaseStarted.Broadcast(this, CurrentPhaseIndex, Phase.PhaseTitle, Phase.PhaseType);
	if (bEndingPlay || RunState != EDCStageRunState::Running) { return; }

	switch (Phase.PhaseType)
	{
	case EDCStagePhaseType::Delay:
		if (!FMath::IsFinite(Phase.PhaseDelay) || Phase.PhaseDelay < 0.0f) { FinishStage(false); }
		else if (Phase.PhaseDelay == 0.0f) { QueueNextPhase(); }
		else { GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ThisClass::QueueNextPhase, Phase.PhaseDelay, false); }
		break;
	case EDCStagePhaseType::Encounter:
	case EDCStagePhaseType::BossEncounter:
		if (!IsValid(Phase.Encounter) || Phase.Encounter->IsActorBeingDestroyed()
			|| Phase.Encounter->HasEncounterFailure() || Phase.Encounter->IsEncounterAborted()) { FinishStage(false); return; }
		if (Phase.Encounter->IsEncounterCompleted()) { QueueNextPhase(); return; }
		ActiveEncounter = Phase.Encounter;
		Phase.Encounter->OnEncounterCompleted.AddUniqueDynamic(this, &ThisClass::HandleEncounterCompleted);
		Phase.Encounter->OnEncounterFailed.AddUniqueDynamic(this, &ThisClass::HandleEncounterFailed);
		Phase.Encounter->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleEncounterDestroyed);
		Phase.Encounter->StartEncounter();
		break;
	case EDCStagePhaseType::Complete:
		FinishStage(true);
		break;
	default:
		FinishStage(false);
		break;
	}
}

void ADCStageDirector::HandleEncounterCompleted(ADCEncounterController* Encounter)
{
	if (RunState == EDCStageRunState::Running && ActiveEncounter.Get() == Encounter && IsValid(Encounter)
		&& Encounter->IsEncounterCompleted() && !Encounter->HasEncounterFailure() && !Encounter->IsEncounterAborted())
	{
		QueueNextPhase();
	}
}

void ADCStageDirector::HandleEncounterFailed(ADCEncounterController* Encounter)
{
	if (IsValid(Encounter) && ActiveEncounter.Get() == Encounter && Encounter->HasEncounterFailure()) { FailStage(); }
}

void ADCStageDirector::HandleEncounterDestroyed(AActor* Encounter)
{
	if (ActiveEncounter.Get() == Encounter) { FailStage(); }
}

void ADCStageDirector::UnbindCurrentEncounter()
{
	if (ADCEncounterController* Encounter = ActiveEncounter.Get())
	{
		Encounter->OnEncounterCompleted.RemoveDynamic(this, &ThisClass::HandleEncounterCompleted);
		Encounter->OnEncounterFailed.RemoveDynamic(this, &ThisClass::HandleEncounterFailed);
		Encounter->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleEncounterDestroyed);
	}
	ActiveEncounter.Reset();
}

void ADCStageDirector::AbortEncounters()
{
	TSet<TWeakObjectPtr<ADCEncounterController>> Encounters;
	for (const FDCStagePhaseDefinition& Phase : Phases)
	{
		if ((Phase.PhaseType == EDCStagePhaseType::Encounter || Phase.PhaseType == EDCStagePhaseType::BossEncounter)
			&& IsValid(Phase.Encounter) && Phase.Encounter->GetWorld() == GetWorld()) { Encounters.Add(Phase.Encounter); }
	}
	for (const TWeakObjectPtr<ADCEncounterController>& Encounter : Encounters)
	{
		if (ADCEncounterController* Actor = Encounter.Get()) { Actor->AbortEncounter(); }
	}
}

void ADCStageDirector::FailStage()
{
	if (!GetWorld() || !GetWorld()->IsGameWorld() || !HasAuthority() || GetNetMode() == NM_Client) { return; }
	FinishStage(false);
}

void ADCStageDirector::FinishStage(bool bSucceeded)
{
	if (bEndingPlay || RunState == EDCStageRunState::Completed || RunState == EDCStageRunState::Failed) { return; }
	RunState = bSucceeded ? EDCStageRunState::Completed : EDCStageRunState::Failed;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	bAdvanceQueued = false;
	UnbindCurrentEncounter();
	if (!bSucceeded) { AbortEncounters(); }
	if (bEndingPlay) { return; }
	// Deliver the terminal state before a Blueprint completion hook can destroy this actor.
	OnStageEnded.Broadcast(this, bSucceeded);
	if (bEndingPlay) { return; }
	if (bSucceeded) { BP_OnStageCompleted(); }
	else { BP_OnStageFailed(); }
}
