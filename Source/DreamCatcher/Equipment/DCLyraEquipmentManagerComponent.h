// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/DCAbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "DCLyraEquipmentManagerComponent.generated.h"

#define UE_API DREAMCATCHER_API

class UActorComponent;
class UDCAbilitySystemComponent;
class UDCLyraEquipmentDefinition;
class UDCLyraEquipmentInstance;
class UDCLyraEquipmentManagerComponent;
class UObject;
struct FFrame;
struct FDCLyraEquipmentList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/** A single piece of applied equipment */
USTRUCT(BlueprintType)
struct FDCLyraAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FDCLyraAppliedEquipmentEntry()
	{}

	FString GetDebugString() const;

private:
	friend FDCLyraEquipmentList;
	friend UDCLyraEquipmentManagerComponent;

	// The equipment class that got equipped
	UPROPERTY()
	TSubclassOf<UDCLyraEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<UDCLyraEquipmentInstance> Instance = nullptr;

	// Authority-only list of granted handles
	UPROPERTY(NotReplicated)
	FDCAbilitySet_GrantedHandles GrantedHandles;
};

/** List of applied equipment */
USTRUCT(BlueprintType)
struct FDCLyraEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FDCLyraEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FDCLyraEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDCLyraAppliedEquipmentEntry, FDCLyraEquipmentList>(Entries, DeltaParms, *this);
	}

	UDCLyraEquipmentInstance* AddEntry(TSubclassOf<UDCLyraEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(UDCLyraEquipmentInstance* Instance);

private:
	UDCAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UDCLyraEquipmentManagerComponent;

private:
	// Replicated list of equipment entries
	UPROPERTY()
	TArray<FDCLyraAppliedEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FDCLyraEquipmentList> : public TStructOpsTypeTraitsBase2<FDCLyraEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};










/**
 * Manages equipment applied to a pawn
 */
UCLASS(MinimalAPI, BlueprintType, Const)
class UDCLyraEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UE_API UDCLyraEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UE_API UDCLyraEquipmentInstance* EquipItem(TSubclassOf<UDCLyraEquipmentDefinition> EquipmentDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UE_API void UnequipItem(UDCLyraEquipmentInstance* ItemInstance);

	//~UObject interface
	UE_API virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	//~End of UObject interface

	//~UActorComponent interface
	//virtual void EndPlay() override;
	UE_API virtual void InitializeComponent() override;
	UE_API virtual void UninitializeComponent() override;
	UE_API virtual void ReadyForReplication() override;
	//~End of UActorComponent interface

	/** Returns the first equipped instance of a given type, or nullptr if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UE_API UDCLyraEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UDCLyraEquipmentInstance> InstanceType);

 	/** Returns all equipped instances of a given type, or an empty array if none are found */
 	UFUNCTION(BlueprintCallable, BlueprintPure)
	UE_API TArray<UDCLyraEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UDCLyraEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

private:
	UPROPERTY(Replicated)
	FDCLyraEquipmentList EquipmentList;
};

#undef UE_API
