// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCAbilitySystemGlobals.h"

#include "DCGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAbilitySystemGlobals)

struct FGameplayEffectContext;

UDCAbilitySystemGlobals::UDCAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UDCAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	UE_LOG(LogTemp, Log, TEXT("[R1-2] Allocating FDCGameplayEffectContext"));

	return new FDCGameplayEffectContext();
}

