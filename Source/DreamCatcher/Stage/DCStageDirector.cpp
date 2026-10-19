#include "Stage/DCStageDirector.h"
#include "Stage/DCEncounterController.h"
#include "Components/SceneComponent.h"
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
	ResetRuntimeMissions();
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
	ResetRuntimeMissions();
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
	ResetRuntimeMissions();
	UnbindCurrentEncounter();
	// Leave death/clear delegate stacks before starting another encounter or ending the run.
	PhaseTimerHandle = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::AdvancePhase);
}

void ADCStageDirector::AdvancePhase()
{
	if (bEndingPlay || RunState != EDCStageRunState::Running) { return; }
	bAdvanceQueued = false;
	ResetRuntimeMissions();
	UnbindCurrentEncounter();
	++CurrentPhaseIndex;
	if (!Phases.IsValidIndex(CurrentPhaseIndex)) { FinishStage(true); return; }
	StartCurrentPhase();
}

void ADCStageDirector::StartCurrentPhase()
{
	const FDCStagePhaseDefinition Phase = Phases[CurrentPhaseIndex];
	ReconcileRuntimeMissions();
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
		if (bEndingPlay || bAdvanceQueued || IsActorBeingDestroyed() || !GetWorld() || GetWorld()->bIsTearingDown) { return; }
		FDCMissionTransitionSnapshot Snapshot;
		Snapshot.PhaseIndex = CurrentPhaseIndex;
		if (Phases.IsValidIndex(CurrentPhaseIndex))
		{
			Snapshot.PhaseTitle = Phases[CurrentPhaseIndex].PhaseTitle;
			if (const UDCMissionListData* List = GetCurrentMissionList())
			{
				Snapshot.HUDStyle = List->HUDStyle;
				Snapshot.Missions = GetMissionProgress();
			}
		}
		const bool bHasCompletedDefeats = Snapshot.Missions.ContainsByPredicate([](const FDCMissionProgress& Row)
		{
			return Row.ProgressSource == EDCMissionProgressSource::CurrentEncounterDefeats
				&& Row.bCompleted && Row.TargetCount > 0 && Row.CurrentCount >= Row.TargetCount;
		});
		// Commit the existing reset/unbind/next-tick advance before a presentation
		// receiver can reenter stage code. UI transition time never delays gameplay.
		QueueNextPhase();
		if (bHasCompletedDefeats && !bEndingPlay && !IsActorBeingDestroyed() && GetWorld() && !GetWorld()->bIsTearingDown
			&& RunState == EDCStageRunState::Running && bAdvanceQueued && CurrentPhaseIndex == Snapshot.PhaseIndex)
		{
			OnMissionPhaseCompleted.Broadcast(this, Snapshot);
			// Do not access stage state after Broadcast: the receiver may end or destroy this actor.
		}
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
	ResetRuntimeMissions();
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

UDCMissionListData* ADCStageDirector::GetCurrentMissionList() const
{
	UDCMissionListData* List = !bEndingPlay && !bAdvanceQueued && RunState == EDCStageRunState::Running && Phases.IsValidIndex(CurrentPhaseIndex)
		? Phases[CurrentPhaseIndex].MissionList.Get() : nullptr;
	return IsValid(List) ? List : nullptr;
}

void ADCStageDirector::ResetRuntimeMissions() const
{
	RuntimeMissionList.Reset();
	ExternalMissionCounts.Reset();
	TrackedMissionId = NAME_None;
	bMissionContextInitialized = false;
}

void ADCStageDirector::ReconcileRuntimeMissions() const
{
	UDCMissionListData* List = GetCurrentMissionList();
	if (!IsValid(List)) { ResetRuntimeMissions(); return; }
	if (!bMissionContextInitialized || RuntimeMissionList.Get() != List)
	{
		ResetRuntimeMissions();
		RuntimeMissionList = List;
		bMissionContextInitialized = true;
		TrackedMissionId = List->FindEnabledMission(List->DefaultTrackedMissionId) ? List->DefaultTrackedMissionId : NAME_None;
	}
	for (auto It = ExternalMissionCounts.CreateIterator(); It; ++It)
	{
		const FDCMissionDefinition* Definition = List->FindEnabledMission(It.Key());
		if (!Definition || Definition->ProgressSource != EDCMissionProgressSource::External) { It.RemoveCurrent(); }
		else { It.Value() = FMath::Clamp(It.Value(), 0, Definition->TargetCount); }
	}
	// Do not reapply the default after explicit untracking or definition removal.
	if (!TrackedMissionId.IsNone() && !List->FindEnabledMission(TrackedMissionId)) { TrackedMissionId = NAME_None; }
}

TArray<FDCMissionProgress> ADCStageDirector::GetMissionProgress() const
{
	ReconcileRuntimeMissions();
	UDCMissionListData* List = RuntimeMissionList.Get();
	TArray<FDCMissionProgress> Result;
	if (!List) { return Result; }
	int32 Defeated = 0, Planned = 0;
	const FDCStagePhaseDefinition& Phase = Phases[CurrentPhaseIndex];
	if ((Phase.PhaseType == EDCStagePhaseType::Encounter || Phase.PhaseType == EDCStagePhaseType::BossEncounter)
		&& IsValid(Phase.Encounter) && !Phase.Encounter->IsActorBeingDestroyed() && Phase.Encounter->GetWorld() == GetWorld())
	{
		Phase.Encounter->GetMissionEnemyProgress(Defeated, Planned);
	}
	for (const FDCMissionDefinition& Definition : List->Missions)
	{
		if (List->FindEnabledMission(Definition.MissionId) != &Definition) { continue; }
		FDCMissionProgress& Row = Result.AddDefaulted_GetRef();
		Row.MissionId = Definition.MissionId; Row.DisplayText = Definition.DisplayText;
		Row.ProgressSource = Definition.ProgressSource; Row.bTracked = Definition.MissionId == TrackedMissionId;
		if (Definition.ProgressSource == EDCMissionProgressSource::CurrentEncounterDefeats)
		{
			Row.CurrentCount = Defeated; Row.TargetCount = Planned;
			Row.bCompleted = Planned > 0 && Defeated >= Planned;
		}
		else if (Definition.ProgressSource == EDCMissionProgressSource::External)
		{
			Row.CurrentCount = ExternalMissionCounts.FindRef(Definition.MissionId); Row.TargetCount = Definition.TargetCount;
			Row.bCompleted = Row.CurrentCount >= Row.TargetCount;
		}
	}
	return Result;
}

bool ADCStageDirector::SetTrackedMission(FName MissionId)
{
	ReconcileRuntimeMissions();
	if (MissionId.IsNone()) { TrackedMissionId = NAME_None; return true; }
	UDCMissionListData* List = RuntimeMissionList.Get();
	if (!List || !List->FindEnabledMission(MissionId)) { return false; }
	TrackedMissionId = MissionId;
	return true;
}

bool ADCStageDirector::UpdateExternalMissionProgress(FName MissionId, int32 CurrentCount)
{
	ReconcileRuntimeMissions();
	UDCMissionListData* List = RuntimeMissionList.Get();
	const FDCMissionDefinition* Definition = List ? List->FindEnabledMission(MissionId) : nullptr;
	if (!Definition || Definition->ProgressSource != EDCMissionProgressSource::External) { return false; }
	ExternalMissionCounts.Add(MissionId, FMath::Clamp(CurrentCount, 0, Definition->TargetCount));
	return true;
}

bool ADCStageDirector::SetExternalMissionCompleted(FName MissionId, bool bCompleted)
{
	ReconcileRuntimeMissions();
	UDCMissionListData* List = RuntimeMissionList.Get();
	const FDCMissionDefinition* Definition = List ? List->FindEnabledMission(MissionId) : nullptr;
	return Definition && Definition->ProgressSource == EDCMissionProgressSource::External
		&& UpdateExternalMissionProgress(MissionId, bCompleted ? Definition->TargetCount : 0);
}

USceneComponent* ADCStageDirector::GetTrackedMissionTargetComponent() const
{
	ReconcileRuntimeMissions();
	const UDCMissionListData* List = RuntimeMissionList.Get();
	const FDCMissionDefinition* Definition = List ? List->FindEnabledMission(TrackedMissionId) : nullptr;
	if (!Definition || Definition->TargetKey.IsNone()) { return nullptr; }
	const FDCMissionTargetBinding* Found = nullptr;
	for (const FDCMissionTargetBinding& Binding : Phases[CurrentPhaseIndex].MissionTargets)
	{
		if (Binding.TargetKey != Definition->TargetKey) { continue; }
		if (Found) { return nullptr; }
		Found = &Binding;
	}
	AActor* Target = Found ? Found->TargetActor.Get() : nullptr;
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target->GetWorld() != GetWorld()) { return nullptr; }
	USceneComponent* Root = Target->GetRootComponent();
	return IsValid(Root) && !Root->IsBeingDestroyed() && Root->GetWorld() == GetWorld()
		&& !Root->GetComponentLocation().ContainsNaN() ? Root : nullptr;
}

bool ADCStageDirector::ValidateMissionBindings(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();
	for (int32 Index = 0; Index < Phases.Num(); ++Index)
	{
		const FDCStagePhaseDefinition& Phase = Phases[Index];
		TSet<FName> Seen;
		for (const FDCMissionTargetBinding& Binding : Phase.MissionTargets)
		{
			if (Binding.TargetKey.IsNone() || Seen.Contains(Binding.TargetKey))
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("Phase[%d] mission binding requires a non-empty unique TargetKey (%s)."), Index, *Binding.TargetKey.ToString())));
			}
			Seen.Add(Binding.TargetKey);
			const AActor* Target = Binding.TargetActor.Get();
			const USceneComponent* Root = IsValid(Target) ? Target->GetRootComponent() : nullptr;
			if (!GetWorld() || !IsValid(Target) || Target->IsActorBeingDestroyed() || Target->GetWorld() != GetWorld()
				|| !IsValid(Root) || Root->IsBeingDestroyed() || Root->GetWorld() != GetWorld() || Root->GetComponentLocation().ContainsNaN())
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("Phase[%d] mission target %s requires a live same-world actor and finite root component."), Index, *Binding.TargetKey.ToString())));
			}
		}
		const UDCMissionListData* List = Phase.MissionList.Get();
		if (!IsValid(List)) { continue; }
		for (const FDCMissionDefinition& Definition : List->Missions)
		{
			if (!Definition.bEnabled || Definition.TargetKey.IsNone()) { continue; }
			int32 Matches = 0;
			for (const FDCMissionTargetBinding& Binding : Phase.MissionTargets)
			{ if (Binding.TargetKey == Definition.TargetKey) { ++Matches; } }
			if (Matches != 1)
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("Phase[%d] enabled mission %s requires exactly one binding for TargetKey %s."), Index, *Definition.MissionId.ToString(), *Definition.TargetKey.ToString())));
			}
		}
	}
	return OutErrors.IsEmpty();
}
