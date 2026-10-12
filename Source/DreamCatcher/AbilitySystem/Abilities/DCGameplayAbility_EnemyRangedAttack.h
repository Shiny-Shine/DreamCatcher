// DreamCatcher NPC extension; reuses Lyra GAS damage/death and engine WaitDelay.
#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DCGameplayAbility_EnemyRangedAttack.generated.h"

class ADCGASEnemyCharacter;
class UAbilityTask_WaitDelay;
class UDCLyraHealthComponent;

UENUM(BlueprintType)
enum class EDCEnemyAttackPhase : uint8 { Idle, Windup, Recovery };

/** Authority-only single shot, not a replacement for Lyra's equipped rifle ability.
 * Granted through an AbilitySet; the NPC supplies a target via TryStartRangedAttack.
 * BP children configure data/presentation, not a second ActivateAbility graph.
 */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_EnemyRangedAttack : public UDCGameplayAbility
{
	GENERATED_BODY()
public:
	UDCGameplayAbility_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintPure, Category="Enemy|Attack")
	EDCEnemyAttackPhase GetAttackPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Enemy|Attack")
	FVector GetLockedAimPoint() const { return LockedAimPoint; }

protected:
	/** Instant GE: original DamageExecution with source BaseDamage + SetByCaller.Damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="0.0"))
	float DamageAmount = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="1.0", Units="cm"))
	float AttackRange = 1400.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="0.01", Units="s"))
	float WindupTime = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="0.0", Units="s"))
	float RecoveryTime = 0.75f;

	/** Presentation hooks only; End is paired once with every emitted Intent. */
	UFUNCTION(BlueprintNativeEvent, Category="Enemy|Attack")
	void BP_OnAttackIntent(FVector TraceStart, FVector AimPoint, float DelaySeconds);
	UFUNCTION(BlueprintNativeEvent, Category="Enemy|Attack")
	void BP_OnAttackShot(FVector TraceStart, FVector TraceEnd, AActor* HitActor);
	UFUNCTION(BlueprintNativeEvent, Category="Enemy|Attack")
	void BP_OnAttackEnded(bool bWasCancelled);

private:
	bool HasValidConfiguration() const;
	bool IsSourceReady(const ADCGASEnemyCharacter* Enemy) const;
	bool IsTargetReady(const ADCGASEnemyCharacter* Enemy, AActor* AttackTarget) const;
	bool HasLineOfSight(const ADCGASEnemyCharacter* Enemy, AActor* AttackTarget, const FVector& Origin) const;
	static FVector GetFireOrigin(const ADCGASEnemyCharacter* Enemy);
	static FVector GetAimPoint(const AActor* AttackTarget);
	void CheckContext();
	void AbortAttack();
	void ReleaseWaitTask(TObjectPtr<UAbilityTask_WaitDelay>& Task);
	UFUNCTION()
	void FinishWindup();
	UFUNCTION()
	void FinishRecovery();
	UFUNCTION()
	void HandleActorEndPlay(AActor* EndedActor, EEndPlayReason::Type Reason);
	UFUNCTION()
	void HandleTargetDeath(AActor* DeadActor);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> WindupTask;
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> RecoveryTask;
	TWeakObjectPtr<ADCGASEnemyCharacter> SourceEnemy;
	TWeakObjectPtr<AActor> LockedTarget;
	TWeakObjectPtr<UDCLyraHealthComponent> TargetHealth;
	TWeakObjectPtr<UWorld> AttackWorld;
	FTimerHandle ContextTimer;
	FVector LockedOrigin = FVector::ZeroVector;
	FVector LockedAimPoint = FVector::ZeroVector;
	EDCEnemyAttackPhase Phase = EDCEnemyAttackPhase::Idle;
	uint32 ActivationSerial = 0;
	bool bEndingAttack = false;
	bool bIntentEmitted = false;
};
