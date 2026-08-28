#include "AbilitySystem/Executions/DCDamageExecution.h"

#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "GameplayEffectExtension.h"

namespace
{
	struct FDCDamageStatics
	{
		FGameplayEffectAttributeCaptureDefinition BaseDamageDefinition;

		FDCDamageStatics() : BaseDamageDefinition(UDCCombatSet::GetBaseDamageAttribute(),
		                                          EGameplayEffectAttributeCaptureSource::Source, true)
		{
		}
	};

	const FDCDamageStatics& GetDamageStatics()
	{
		static FDCDamageStatics DamageStatics;

		return DamageStatics;
	}
}

UDCDamageExecution::UDCDamageExecution()
{
	RelevantAttributesToCapture.Add(GetDamageStatics().BaseDamageDefinition);
}

void UDCDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParameters,
                                                FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& EffectSpec = ExecutionParameters.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();

	const FGameplayTagContainer* TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float CapturedBaseDamage = 0.0f;

	ExecutionParameters.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().BaseDamageDefinition,
	                                                               EvaluationParameters, CapturedBaseDamage);

	/**
	 * -1은 SetByCaller.Damage가 제공되지 않았다는 표시.
	 *
	 * 명시적으로 0을 전달한 경우에는 0 데미지로 처리할 수 있음.
	 */
	const float SetByCallerDamage = EffectSpec.
		GetSetByCallerMagnitude(DCGameplayTags::SetByCaller_Damage, false, -1.0f);

	const float RequestedDamage = SetByCallerDamage >= 0.0f ? SetByCallerDamage : CapturedBaseDamage;

	const float FinalDamage = FMath::Max(RequestedDamage, 0.0f);

	if (FinalDamage <= 0.0f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UDCHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
#endif
}