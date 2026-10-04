// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Templates/SubclassOf.h"

#include "DCInventoryManagerComponent.generated.h"

#define UE_API DREAMCATCHER_API

class UDCInventoryItemDefinition;
class UDCInventoryItemInstance;
class UDCInventoryManagerComponent;
class UObject;
struct FFrame;
struct FDCInventoryList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/** A message when an item is added to the inventory */
USTRUCT(BlueprintType)
struct FDCInventoryChangeMessage
{
	GENERATED_BODY()

	//@TODO: Tag based names+owning actors for inventories instead of directly exposing the component?
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UDCInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/** A single entry in an inventory */
USTRUCT(BlueprintType)
struct FDCInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FDCInventoryEntry()
	{}

	FString GetDebugString() const;

private:
	friend FDCInventoryList;
	friend UDCInventoryManagerComponent;

	UPROPERTY()
	TObjectPtr<UDCInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

/** List of inventory items */
USTRUCT(BlueprintType)
struct FDCInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FDCInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FDCInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UDCInventoryItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDCInventoryEntry, FDCInventoryList>(Entries, DeltaParms, *this);
	}

	UDCInventoryItemInstance* AddEntry(TSubclassOf<UDCInventoryItemDefinition> ItemClass, int32 StackCount);
	void AddEntry(UDCInventoryItemInstance* Instance);

	void RemoveEntry(UDCInventoryItemInstance* Instance);

private:
	void BroadcastChangeMessage(FDCInventoryEntry& Entry, int32 OldCount, int32 NewCount);

private:
	friend UDCInventoryManagerComponent;

private:
	// Replicated list of items
	UPROPERTY()
	TArray<FDCInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FDCInventoryList> : public TStructOpsTypeTraitsBase2<FDCInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};










/**
 * Manages an inventory
 */
UCLASS(MinimalAPI, BlueprintType)
class UDCInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UE_API UDCInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API bool CanAddItemDefinition(TSubclassOf<UDCInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API UDCInventoryItemInstance* AddItemDefinition(TSubclassOf<UDCInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	// Preserved Lyra limitation: this reaches unimplemented(). Use AddItemDefinition for now.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API void AddItemInstance(UDCInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API void RemoveItemInstance(UDCInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	UE_API TArray<UDCInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UE_API UDCInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UDCInventoryItemDefinition> ItemDef) const;

	// Original semantics: counts matching instances, not the sum of entry StackCount values.
	UE_API int32 GetTotalItemCountByDefinition(TSubclassOf<UDCInventoryItemDefinition> ItemDef) const;
	// Original semantics: removes whole entries; a failure does not roll back prior removals.
	UE_API bool ConsumeItemsByDefinition(TSubclassOf<UDCInventoryItemDefinition> ItemDef, int32 NumToConsume);

	//~UObject interface
	UE_API virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	UE_API virtual void ReadyForReplication() override;
	//~End of UObject interface

private:
	UPROPERTY(Replicated)
	FDCInventoryList InventoryList;
};

#undef UE_API
