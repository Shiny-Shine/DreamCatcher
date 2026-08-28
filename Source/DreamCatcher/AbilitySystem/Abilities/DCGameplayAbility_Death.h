#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "DCGameplayAbility_Death.generated.h"

struct FGameplayAbilityActorInfo;
struct FGameplayEventData;

// GameplayEvent.Death에 의해 자동으로 활성화되는 사망 Ability.
UCLASS(Abstract)
class DREAMCATCHER_API UDCGameplayAbility_Death : public UDCGameplayAbility
{
	GENERATED_BODY()

public:
	UDCGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo
		ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo
		ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Death")
	void StartDeath();

	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Death")
	void FinishDeath();

	// 활성화되자마자 StartDeath를 호출. 향후 사망 연출 전에 별도 절차가 필요하면 Blueprint에서 false로 바꿀 수 있음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Death")
	bool bAutoStartDeath = true;
};