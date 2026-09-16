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
class UDCEquipmentDefinition;


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

	// 과도기 필드: 현재 Character의 견착 카메라 선택에 사용합니다.
	// 원본 카메라 연결의 대체 검증 이후 제거합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Legacy")
	TSubclassOf<UDCCameraMode> ShoulderCameraMode;

	// 과도기 필드: 현재 Character의 Scope 카메라 선택에 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Legacy")
	TSubclassOf<UDCCameraMode> ScopeCameraMode;

	// 과도기 필드: 현재 EquipmentManager의 초기 장착에 사용합니다.
	// 원본 Inventory / QuickBar / Equipment 연결 검증 이후 제거합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Legacy")
	TSubclassOf<UDCEquipmentDefinition> DefaultWeaponDefinition;
};

#undef UE_API
