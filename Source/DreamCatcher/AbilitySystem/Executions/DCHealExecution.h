// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "DCHealExecution.generated.h"

class UObject;

/**
 * UDCHealExecution
 *
 * Execution used by gameplay effects to apply healing
 * to the health attributes.
 */
UCLASS()
class UDCHealExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UDCHealExecution();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};