#include "AbilitySystem/Attributes/DCResourceSet.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UDCResourceSet::UDCResourceSet() : UltimateCharge(0.0f), MaxUltimateCharge(100.0f)
{
}

float UDCResourceSet::GetUltimateChargeNormalized() const
{
	const float MaximumCharge = GetMaxUltimateCharge();

	return MaximumCharge > 0.0f ? GetUltimateCharge() / MaximumCharge : 0.0f;
}

void UDCResourceSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCResourceSet, UltimateCharge, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCResourceSet, MaxUltimateCharge, COND_OwnerOnly, REPNOTIFY_Always);
}

void UDCResourceSet::OnRep_UltimateCharge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCResourceSet, UltimateCharge, OldValue);

	const float PreviousValue = OldValue.GetCurrentValue();

	const float CurrentValue = GetUltimateCharge();

	OnUltimateChargeChanged.Broadcast(nullptr, nullptr, nullptr, CurrentValue - PreviousValue, PreviousValue,
	                                  CurrentValue);
}

void UDCResourceSet::OnRep_MaxUltimateCharge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCResourceSet, MaxUltimateCharge, OldValue);

	const float PreviousValue = OldValue.GetCurrentValue();

	const float CurrentValue = GetMaxUltimateCharge();

	OnMaxUltimateChargeChanged.Broadcast(nullptr, nullptr, nullptr, CurrentValue - PreviousValue, PreviousValue,
	                                     CurrentValue);
}

void UDCResourceSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UDCResourceSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UDCResourceSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최대 게이지가 감소해 현재 게이지보다 작아지면 현재 게이지도 새 최대치로 제한.
	if (Attribute == GetMaxUltimateChargeAttribute() && GetUltimateCharge() > NewValue)
	{
		if (UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent())
		{
			AbilitySystemComponent->ApplyModToAttribute(GetUltimateChargeAttribute(), EGameplayModOp::Override,
			                                            NewValue);
		}
	}
}

void UDCResourceSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetUltimateChargeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxUltimateCharge());
	}
	else if (Attribute == GetMaxUltimateChargeAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
