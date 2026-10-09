// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Inventory/DCInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "DCInventoryFragment_EquippableItem.generated.h"

class UDCLyraEquipmentDefinition;
class UObject;

UCLASS()
class UDCInventoryFragment_EquippableItem : public UDCInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Lyra)
	TSubclassOf<UDCLyraEquipmentDefinition> EquipmentDefinition;
};
