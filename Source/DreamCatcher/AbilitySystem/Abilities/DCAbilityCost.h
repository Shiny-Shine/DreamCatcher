// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"

#include "DCAbilityCost.generated.h"

class UDCGameplayAbility;

/**
 * Base class for additional costs of a gameplay ability.
 * Examples include ammunition or charges.
 */
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UDCAbilityCost : public UObject
{
	GENERATED_BODY()

public:
	UDCAbilityCost()
	{
	}

	/**
	 * Checks whether this cost can be paid.
	 * OptionalRelevantTags can receive a failure reason.
	 */
	virtual bool CheckCost(const UDCGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	                       const FGameplayAbilityActorInfo* ActorInfo,
	                       FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	/**
	 * Applies the cost.
	 * The caller handles ShouldOnlyApplyCostOnHit().
	 */
	virtual void ApplyCost(const UDCGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	                       const FGameplayAbilityActorInfo* ActorInfo,
	                       const FGameplayAbilityActivationInfo ActivationInfo)
	{
	}

	/** If true, this cost should only be applied if this ability hits successfully */
	bool ShouldOnlyApplyCostOnHit() const { return bOnlyApplyCostOnHit; }

protected:
	/** If true, this cost should only be applied if this ability hits successfully */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
	bool bOnlyApplyCostOnHit = false;
};
