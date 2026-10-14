// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCInventoryItemDefinition.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCInventoryItemDefinition)

//////////////////////////////////////////////////////////////////////
// UDCInventoryItemDefinition

UDCInventoryItemDefinition::UDCInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UDCInventoryItemFragment* UDCInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UDCInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UDCInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// UDCInventoryItemDefinition

const UDCInventoryItemFragment* UDCInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UDCInventoryItemDefinition> ItemDef, TSubclassOf<UDCInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UDCInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}
