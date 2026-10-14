#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "DCGameplayAbility_Aim.generated.h"

class UAbilityTask_WaitDelay;
class UAbilityTask_WaitInputRelease;
class UDCAbilitySystemComponent;
class UGameplayEffect;

/**
 * 우클릭 하나로 Scope와 Shoulder 조준을 처리하는 Ability.
 *
 * Hip 짧은 클릭:
 *   Scope Effect 적용 후 Ability 종료.
 *
 * Hip Hold:
 *   일정 시간 후 Shoulder Effect 적용.
 *   입력 해제 시 Effect 제거 후 Ability 종료.
 *
 * Scope에서 입력:
 *   Scope Effect를 즉시 제거하고 Ability 종료.
 */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_Aim : public UDCGameplayAbility
{
	GENERATED_BODY()

public:
	UDCGameplayAbility_Aim(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	// 이 시간 전에 해제하면 Scope, 이 시간이 지나면 Shoulder.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Aim",
		meta = (ClampMin = "0.05", Units = "s"))
	float ShoulderHoldThreshold = 0.18f;

	// Scope 상태 태그를 부여하는 Infinite GameplayEffect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Aim")
	TSubclassOf<UGameplayEffect> ScopeEffectClass;

	// Shoulder 상태 태그를 부여하는 Infinite GameplayEffect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Aim")
	TSubclassOf<UGameplayEffect> ShoulderEffectClass;

private:
	UFUNCTION()
	void HandleInputReleased(float TimeHeld);

	UFUNCTION()
	void HandleShoulderThresholdReached();

	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const;

	FActiveGameplayEffectHandle ApplyAimEffect(TSubclassOf<UGameplayEffect> EffectClass);

	void RemoveShoulderEffect();

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitInputRelease> WaitInputReleaseTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> ShoulderDelayTask;

	FActiveGameplayEffectHandle ActiveShoulderEffectHandle;

	bool bShoulderAimActive = false;
};
