#pragma once

#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "TimerManager.h"
#include "DCBossPhaseComponent.generated.h"

class ADCBossCharacter;
class ADCStageDirector;
class UDCLyraHealthComponent;
class UDCBossHealthWidget;

UENUM(BlueprintType)
enum class EDCBossRunState : uint8 { Waiting, Fighting, Transitioning, Stopped };

/** Only coordinates two granted abilities. Damage/telegraph lifetimes belong to those GAs. */
UCLASS()
class DREAMCATCHER_API UDCBossPhaseComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDCBossPhaseComponent();
	void InitializeBoss();
	void ShutdownBoss();
	UFUNCTION(BlueprintPure, Category="Boss")
	int32 GetBossPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Boss")
	EDCBossRunState GetBossRunState() const { return State; }
	UFUNCTION(BlueprintPure, Category="Boss")
	bool IsGroundPatternNext() const { return bGroundNext; }
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void UpdateCombat();
	void FinishTransition();
	void CancelPatterns();
	void RefreshHUD();
	void FailSetup(const TCHAR* Reason);
	UFUNCTION()
	void HealthChanged(UDCLyraHealthComponent* Health, float OldValue, float NewValue, AActor* Instigator);
	UFUNCTION()
	void DeathStarted(AActor* Actor);
	UFUNCTION()
	void StageEnded(ADCStageDirector* Director, bool bSucceeded);
	TWeakObjectPtr<ADCBossCharacter> Boss;
	TWeakObjectPtr<ADCStageDirector> Stage;
	UPROPERTY(Transient)
	TObjectPtr<UDCBossHealthWidget> HUD;
	FGameplayAbilitySpecHandle ActivePattern;
	FTimerHandle PollTimer;
	FTimerHandle TransitionTimer;
	double NextAttackAt = 0;
	int32 Phase = 0;
	EDCBossRunState State = EDCBossRunState::Waiting;
	bool bGroundNext = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBossTestAccess;
#endif
};
