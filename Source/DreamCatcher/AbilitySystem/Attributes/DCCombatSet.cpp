// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCCombatSet.h"

#include "AbilitySystem/Attributes/DCAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCCombatSet)

class FLifetimeProperty;


UDCCombatSet::UDCCombatSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UDCCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDCCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UDCCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCCombatSet, BaseDamage, OldValue);
}

void UDCCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCCombatSet, BaseHeal, OldValue);
}
