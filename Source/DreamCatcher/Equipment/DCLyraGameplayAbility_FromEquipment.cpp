// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraGameplayAbility_FromEquipment.h"
#include "DCLyraEquipmentInstance.h"
#include "Inventory/DCInventoryItemInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraGameplayAbility_FromEquipment)

UDCLyraGameplayAbility_FromEquipment::UDCLyraGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UDCLyraEquipmentInstance* UDCLyraGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UDCLyraEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UDCInventoryItemInstance* UDCLyraGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (UDCLyraEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UDCInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}


#if WITH_EDITOR
EDataValidationResult UDCLyraGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	{
		Context.AddError(NSLOCTEXT("Lyra", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif
