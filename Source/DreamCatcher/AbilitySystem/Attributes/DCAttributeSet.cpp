// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCAttributeSet.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAttributeSet)

class UWorld;


UDCAttributeSet::UDCAttributeSet()
{
}

UWorld* UDCAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UDCAbilitySystemComponent* UDCAttributeSet::GetDCAbilitySystemComponent() const
{
	return Cast<UDCAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
