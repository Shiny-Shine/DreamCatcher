// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCAbilityCost_PlayerTagStack.h"

#include "GameFramework/Controller.h"
#include "DCGameplayAbility.h"
#include "Player/DCPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAbilityCost_PlayerTagStack)

UDCAbilityCost_PlayerTagStack::UDCAbilityCost_PlayerTagStack()
{
	Quantity.SetValue(1.0f);
}

bool UDCAbilityCost_PlayerTagStack::CheckCost(const UDCGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (AController* PC = Ability->GetControllerFromActorInfo())
	{
		if (ADCPlayerState* PS = Cast<ADCPlayerState>(PC->PlayerState))
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

			return PS->GetStatTagStackCount(Tag) >= NumStacks;
		}
	}
	return false;
}

void UDCAbilityCost_PlayerTagStack::ApplyCost(const UDCGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (AController* PC = Ability->GetControllerFromActorInfo())
		{
			if (ADCPlayerState* PS = Cast<ADCPlayerState>(PC->PlayerState))
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				PS->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}

