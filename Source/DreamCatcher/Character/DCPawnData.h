#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DCPawnData.generated.h"

class APawn;
class UDCAbilitySet;
class UDCInputConfig;
class UDCCameraMode;
class UDCEquipmentDefinition;

/**
 * Pawn 하나를 구성하는 데이터 묶음.
 *
 * 초기 단계에서는 Pawn 클래스와 기본 AbilitySet만 보관.
 * InputConfig, CameraMode, TagRelationshipMapping은 각 클래스가
 * 실제로 구현되는 단계에서 추가.
 */
UCLASS(BlueprintType, Const,
	Meta = ( DisplayName = "DreamCatcher Pawn Data", ShortTooltip = "Data asset used to configure a DreamCatcher Pawn."
	))
class DREAMCATCHER_API UDCPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDCPawnData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 이 PawnData를 사용할 Pawn 또는 Character 클래스.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Pawn")
	TSubclassOf<APawn> PawnClass;

	// 이 Pawn이 기본적으로 받을 AbilitySet 목록.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Abilities")
	TArray<TObjectPtr<UDCAbilitySet>> AbilitySets;

	// 이 Pawn이 사용할 InputAction → InputTag 설정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Input")
	TObjectPtr<UDCInputConfig> InputConfig;

	// 이 Pawn이 기본적으로 사용할 CameraMode.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera")
	TSubclassOf<UDCCameraMode> DefaultCameraMode;

	// Shoulder 조준 상태에서 사용할 CameraMode.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera")
	TSubclassOf<UDCCameraMode> ShoulderCameraMode;

	// Scope 조준 상태에서 사용할 CameraMode.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera")
	TSubclassOf<UDCCameraMode> ScopeCameraMode;
	
	// ASC 준비가 끝나면 자동으로 장착할 기본 무기. None이면 자동 장착 X.
	// Inventory와 QuickBar를 구현하기 전까지 사용하는 초기 장비 설정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Equipment")
	TSubclassOf<UDCEquipmentDefinition> DefaultWeaponDefinition;
};
