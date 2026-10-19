#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/Stage/DCMissionListData.h"
#include "DCStageDirector.generated.h"

class ADCEncounterController;
class ADCStageDirector;
class USceneComponent;

UENUM(BlueprintType)
enum class EDCStageRunState : uint8
{
	NotStarted,
	Running,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EDCStagePhaseType : uint8
{
	Delay,
	Encounter,
	BossEncounter,
	Complete
};

USTRUCT(BlueprintType)
struct FDCMissionTargetBinding
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName TargetKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	TObjectPtr<AActor> TargetActor = nullptr;
};

USTRUCT(BlueprintType)
struct FDCStagePhaseDefinition
{
	GENERATED_BODY()

	// 컷신/전환용 지연, 일반 전투, 보스 전투, 종료 중 하나.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
	EDCStagePhaseType PhaseType = EDCStagePhaseType::Delay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
	FText PhaseTitle;

	// Delay 단계일 때 다음 단계로 넘어가기 전 대기 시간.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage", meta=(ClampMin="0.0", Units="s"))
	float PhaseDelay = 1.0f;

	// Encounter/BossEncounter 단계일 때 사용할 전투 컨트롤러.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
	TObjectPtr<ADCEncounterController> Encounter = nullptr;
	/** Optional presentation data; never changes phase completion rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage|Mission")
	TObjectPtr<UDCMissionListData> MissionList = nullptr;
	/** Level actor bindings live on this placed director, not the reusable data asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage|Mission", meta=(TitleProperty="TargetKey"))
	TArray<FDCMissionTargetBinding> MissionTargets;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FDCStagePhaseStartedSignature, ADCStageDirector*, Director,
	int32, PhaseIndex, const FText&, PhaseTitle, EDCStagePhaseType, PhaseType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCStageEndedSignature, ADCStageDirector*, Director, bool, bSucceeded);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDCMissionPhaseCompletedSignature, ADCStageDirector*, const FDCMissionTransitionSnapshot&);

UCLASS()
class DREAMCATCHER_API ADCStageDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCStageDirector();

	UFUNCTION(BlueprintCallable, Category="Stage")
	void StartStage();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Stage")
	void FailStage();
	UFUNCTION(BlueprintPure, Category="Stage")
	EDCStageRunState GetRunState() const { return RunState; }
	UFUNCTION(BlueprintPure, Category="Stage")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }
	UFUNCTION(BlueprintPure, Category="Stage|Mission")
	UDCMissionListData* GetCurrentMissionList() const;
	UFUNCTION(BlueprintPure, Category="Stage|Mission")
	TArray<FDCMissionProgress> GetMissionProgress() const;
	UFUNCTION(BlueprintPure, Category="Stage|Mission")
	USceneComponent* GetTrackedMissionTargetComponent() const;
	/** Read-only authoring diagnostics; never participates in ValidatePhases. */
	UFUNCTION(BlueprintPure, Category="Stage|Mission")
	bool ValidateMissionBindings(TArray<FText>& OutErrors) const;
	UFUNCTION(BlueprintCallable, Category="Stage|Mission")
	bool SetTrackedMission(FName MissionId);
	UFUNCTION(BlueprintCallable, Category="Stage|Mission")
	bool UpdateExternalMissionProgress(FName MissionId, int32 CurrentCount);
	UFUNCTION(BlueprintCallable, Category="Stage|Mission")
	bool SetExternalMissionCompleted(FName MissionId, bool bCompleted);
	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCStagePhaseStartedSignature OnStagePhaseStarted;
	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCStageEndedSignature OnStageEnded;
	/** Receivers copy the final phase view; this director retains no presentation history. */
	FDCMissionPhaseCompletedSignature OnMissionPhaseCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
	bool bAutoStartStage = true;

	// 배열 순서가 "시작 > 컷신 > 전투1 > 전투2 > 컷신 > 보스전" 흐름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
	TArray<FDCStagePhaseDefinition> Phases;

	UFUNCTION(BlueprintImplementableEvent, Category="Stage")
	void BP_OnStageStarted();

	UFUNCTION(BlueprintImplementableEvent, Category="Stage")
	void BP_OnStagePhaseStarted(int32 PhaseIndex, const FText& PhaseTitle, EDCStagePhaseType PhaseType);

	UFUNCTION(BlueprintImplementableEvent, Category="Stage")
	void BP_OnStageCompleted();
	UFUNCTION(BlueprintImplementableEvent, Category="Stage")
	void BP_OnStageFailed();

private:
	void AdvancePhase();
	void StartCurrentPhase();
	void QueueNextPhase();
	void FinishStage(bool bSucceeded);
	void UnbindCurrentEncounter();
	void AbortEncounters();
	bool ValidatePhases() const;
	void ResetRuntimeMissions() const;
	void ReconcileRuntimeMissions() const;

	UFUNCTION()
	void HandleEncounterCompleted(ADCEncounterController* CompletedEncounter);
	UFUNCTION()
	void HandleEncounterFailed(ADCEncounterController* FailedEncounter);
	UFUNCTION()
	void HandleEncounterDestroyed(AActor* DestroyedEncounter);

	int32 CurrentPhaseIndex = INDEX_NONE;
	EDCStageRunState RunState = EDCStageRunState::NotStarted;
	TWeakObjectPtr<ADCEncounterController> ActiveEncounter;
	bool bAdvanceQueued = false;
	bool bEndingPlay = false;
	FTimerHandle PhaseTimerHandle;
	// Presentation state only. Never written into MissionList or phase bindings.
	mutable TWeakObjectPtr<UDCMissionListData> RuntimeMissionList;
	mutable TMap<FName, int32> ExternalMissionCounts;
	mutable FName TrackedMissionId;
	mutable bool bMissionContextInitialized = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCStageFlowTestAccess;
	friend struct FDCMissionTestAccess;
#endif
};
