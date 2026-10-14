// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "Inventory/DCInventoryItemDefinition.h"

#include "DCInventoryFragment_SetStats.generated.h"

class UDCInventoryItemInstance;
class UObject;
struct FGameplayTag;

UCLASS()
class UDCInventoryFragment_SetStats : public UDCInventoryItemFragment
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;

public:
	virtual void OnInstanceCreated(UDCInventoryItemInstance* Instance) const override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
};
