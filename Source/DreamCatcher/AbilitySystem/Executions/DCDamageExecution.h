#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "DCDamageExecution.generated.h"

/**
 * 공격자의 CombatSet과 GameplayEffect Spec을 이용해
 * 최종 Damage Meta Attribute 값을 계산.
 *
 * 현재 버전:
 * - SetByCaller.Damage가 있으면 해당 값 사용
 * - 없으면 Source의 BaseDamage 사용
 *
 * 이후 무기 단계에서 거리, 부위, 재질 배율을 추가.
 */
UCLASS()
class DREAMCATCHER_API UDCDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UDCDamageExecution();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParameters,
	                                    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
