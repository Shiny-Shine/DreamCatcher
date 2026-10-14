// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "DCInventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class UDCInventoryItemInstance;
struct FFrame;

//////////////////////////////////////////////////////////////////////

// Represents a fragment of an item definition
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UDCInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UDCInventoryItemInstance* Instance) const {}
};

//////////////////////////////////////////////////////////////////////

/**
 * UDCInventoryItemDefinition
 */
UCLASS(Blueprintable, Const, Abstract)
class UDCInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UDCInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UDCInventoryItemFragment>> Fragments;

public:
	const UDCInventoryItemFragment* FindFragmentByClass(TSubclassOf<UDCInventoryItemFragment> FragmentClass) const;
};

//@TODO: Make into a subsystem instead?
UCLASS()
class UDCInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UDCInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UDCInventoryItemDefinition> ItemDef, TSubclassOf<UDCInventoryItemFragment> FragmentClass);
};
