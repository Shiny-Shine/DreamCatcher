// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "DCPawnData.generated.h"

#define UE_API DREAMCATCHER_API

class APawn;
class UDCAbilitySet;
class UDCAbilityTagRelationshipMapping;
class UDCCameraMode;
class UDCInputConfig;
class UObject;

/**
 * UDCPawnData
 *
 *	Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(MinimalAPI, BlueprintType, Const,
	Meta = (DisplayName = "DC Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class UDCPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API UDCPawnData(const FObjectInitializer& ObjectInitializer);

public:
	// Class to instantiate for this pawn (should usually derive from ADCPawn or ADreamCatcherCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DC|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DC|Abilities")
	TArray<TObjectPtr<UDCAbilitySet>> AbilitySets;

	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DC|Abilities")
	TObjectPtr<UDCAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DC|Input")
	TObjectPtr<UDCInputConfig> InputConfig;

	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DC|Camera")
	TSubclassOf<UDCCameraMode> DefaultCameraMode;

};

#undef UE_API
