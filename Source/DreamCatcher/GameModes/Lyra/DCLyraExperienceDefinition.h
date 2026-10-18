// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "DCLyraExperienceDefinition.generated.h"

class UGameFeatureAction;
class UDCPawnData;
class UDCLyraExperienceActionSet;

/**
 * Definition of an experience
 */
UCLASS(BlueprintType, Const)
class DREAMCATCHER_API UDCLyraExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDCLyraExperienceDefinition();

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif
	//~End of UPrimaryDataAsset interface

public:
	// List of Game Feature Plugins this experience wants to have active
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	/** The default pawn class to spawn for players */
	//@TODO: Make soft?
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TObjectPtr<const UDCPawnData> DefaultPawnData;

	/** Opt in to separate initial combat attributes for human players and player bots. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	bool bUseRoleBasedBaseDamage = false;

	/** Initial player CombatSet.BaseDamage. The weapon's execution modifier still applies. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage", meta = (EditCondition = "bUseRoleBasedBaseDamage", EditConditionHides, ClampMin = "0.0", UIMin = "0.0"))
	float PlayerBaseDamage = 0.0f;

	/** Initial player-bot CombatSet.BaseDamage. Rifle bonus is excluded by its source-tag condition. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage", meta = (EditCondition = "bUseRoleBasedBaseDamage", EditConditionHides, ClampMin = "0.0", UIMin = "0.0"))
	float EnemyBaseDamage = 12.0f;

	// List of actions to perform as this experience is loaded/activated/deactivated/unloaded
	UPROPERTY(EditDefaultsOnly, Instanced, Category="Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	// List of additional action sets to compose into this experience
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<UDCLyraExperienceActionSet>> ActionSets;
};
