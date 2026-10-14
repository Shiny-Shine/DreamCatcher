// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCInventoryFragment_SetStats.h"

#include "Inventory/DCInventoryItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCInventoryFragment_SetStats)

void UDCInventoryFragment_SetStats::OnInstanceCreated(UDCInventoryItemInstance* Instance) const
{
	for (const auto& KVP : InitialItemStats)
	{
		Instance->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

int32 UDCInventoryFragment_SetStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}
