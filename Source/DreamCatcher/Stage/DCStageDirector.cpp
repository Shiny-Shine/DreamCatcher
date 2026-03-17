#include "Stage/DCStageDirector.h"
#include "Stage/DCEncounterController.h"
#include "TimerManager.h"

ADCStageDirector::ADCStageDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADCStageDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartStage)
	{
		StartStage();
	}
}

void ADCStageDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	for (const FDCStagePhaseDefinition& Phase : Phases)
	{
		if (Phase.Encounter)
		{
			Phase.Encounter->OnEncounterCompleted.RemoveDynamic(this, &ADCStageDirector::HandleEncounterCompleted);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ADCStageDirector::StartStage()
{
	if (bStageStarted)
	{
		return;
	}

	bStageStarted = true;
	CurrentPhaseIndex = INDEX_NONE;

	BP_OnStageStarted();
	AdvancePhase();
}

void ADCStageDirector::AdvancePhase()
{
	// 이전 단계의 Encounter 바인딩 정리.
	if (Phases.IsValidIndex(CurrentPhaseIndex))
	{
		if (ADCEncounterController* PreviousEncounter = Phases[CurrentPhaseIndex].Encounter)
		{
			PreviousEncounter->OnEncounterCompleted.RemoveDynamic(this, &ADCStageDirector::HandleEncounterCompleted);
		}
	}

	++CurrentPhaseIndex;

	if (!Phases.IsValidIndex(CurrentPhaseIndex))
	{
		BP_OnStageCompleted();
		return;
	}

	StartCurrentPhase();
}

void ADCStageDirector::StartCurrentPhase()
{
	const FDCStagePhaseDefinition& Phase = Phases[CurrentPhaseIndex];

	BP_OnStagePhaseStarted(CurrentPhaseIndex, Phase.PhaseTitle, Phase.PhaseType);

	switch (Phase.PhaseType)
	{
	case EDCStagePhaseType::Delay:
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADCStageDirector::AdvancePhase, Phase.PhaseDelay, false);
		break;

	case EDCStagePhaseType::Encounter:
	case EDCStagePhaseType::BossEncounter:
		if (Phase.Encounter)
		{
			Phase.Encounter->OnEncounterCompleted.RemoveDynamic(this, &ADCStageDirector::HandleEncounterCompleted);
			Phase.Encounter->OnEncounterCompleted.AddDynamic(this, &ADCStageDirector::HandleEncounterCompleted);
			Phase.Encounter->StartEncounter();
		}
		else
		{
			AdvancePhase();
		}
		break;

	case EDCStagePhaseType::Complete:
		BP_OnStageCompleted();
		break;

	default:
		break;
	}
}

void ADCStageDirector::HandleEncounterCompleted(ADCEncounterController* CompletedEncounter)
{
	if (CompletedEncounter)
	{
		CompletedEncounter->OnEncounterCompleted.RemoveDynamic(this, &ADCStageDirector::HandleEncounterCompleted);
	}

	AdvancePhase();
}