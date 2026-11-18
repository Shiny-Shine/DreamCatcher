// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/Abilities/DCGameplayAbility.h"

#include "DCLyraGameplayAbility_FromEquipment.generated.h"

class UDCLyraEquipmentInstance;
class UDCInventoryItemInstance;

/**
 * UDCLyraGameplayAbility_FromEquipment
 *
 * An ability granted by and associated with an equipment instance
 */
UCLASS()
class UDCLyraGameplayAbility_FromEquipment : public UDCGameplayAbility
{
	GENERATED_BODY()

public:

	UDCLyraGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Ability")
	UDCLyraEquipmentInstance* GetAssociatedEquipment() const;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	UDCInventoryItemInstance* GetAssociatedItem() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

};
