#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "TimerManager.h"
#include "DCGameplayAbility_BossGroundStrike.generated.h"

class ADCBossCharacter;
class ADCBossTelegraph;
class UAbilityTask_WaitDelay;

/** New boss rule: lock one ground point, telegraph, then one GAS damage application per eligible actor. */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_BossGroundStrike : public UDCGameplayAbility
{
	GENERATED_BODY()
public:
	UDCGameplayAbility_BossGroundStrike(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UFUNCTION(BlueprintPure, Category="Boss")
	EDCEnemyAttackPhase GetAttackPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Boss")
	FVector GetLockedGroundPoint() const { return LockedGround; }
	UFUNCTION(BlueprintPure, Category="Boss")
	float GetStrikeRadius() const { return Radius; }
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1"))
	float DamageAmount = 18.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1", Units="cm"))
	float Radius = 220.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1", Units="cm"))
	float AttackRange = 2500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1", Units="cm"))
	float VerticalTolerance = 120.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float WindupTime = 1.4f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.01", Units="s"))
	float RecoveryTime = .7f;
private:
	bool ValidConfiguration() const;
	bool ValidContext(const ADCBossCharacter* Source, AActor* Target) const;
	bool FindGround(const ADCBossCharacter* Source, AActor* Target, FVector& OutGround) const;
	bool IsInsideStrike(AActor* Target) const;
	bool HasBlastLineOfSight(AActor* Target, FHitResult& Hit) const;
	void CheckContext();
	void ReleaseTask(TObjectPtr<UAbilityTask_WaitDelay>& Task);
	void Abort();
	UFUNCTION()
	void Detonate();
	UFUNCTION()
	void FinishRecovery();
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> WindupTask;
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> RecoveryTask;
	TWeakObjectPtr<ADCBossCharacter> Boss;
	TWeakObjectPtr<AActor> LockedTarget;
	TWeakObjectPtr<ADCBossTelegraph> Warning;
	TWeakObjectPtr<UWorld> AttackWorld;
	FVector LockedGround = FVector::ZeroVector;
	FTimerHandle ContextTimer;
	EDCEnemyAttackPhase Phase = EDCEnemyAttackPhase::Idle;
	uint32 Serial = 0;
	bool bEnding = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBossTestAccess;
#endif
};
