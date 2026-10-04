// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "Components/GameStateComponent.h"

#include "DCAimAssistTargetManagerComponent.generated.h"

#define UE_API DREAMCATCHER_API

enum class ECommonInputType : uint8;

class AActor;
class APlayerController;
class UObject;
struct FDCAimAssistFilter;
struct FDCAimAssistOwnerViewData;
struct FDCAimAssistSettings;
struct FDCAimAssistTargetOptions;
struct FCollisionQueryParams;
struct FDCAimAssistTarget;

/**
 * The Aim Assist Target Manager Component is used to gather all aim assist targets that are within
 * a given player's view. Targets must implement the DCAimAssistTargetInterface and be on the
 * collision channel that is set in the DCShooterCoreRuntimeSettings. 
 */
UCLASS(MinimalAPI, Blueprintable)
class UDCAimAssistTargetManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:

	/** Gets all visible active targets based on the given local player and their ViewTransform */
	UE_API void GetVisibleTargets(const FDCAimAssistFilter& Filter, const FDCAimAssistSettings& Settings, const FDCAimAssistOwnerViewData& OwnerData, const TArray<FDCAimAssistTarget>& OldTargets, OUT TArray<FDCAimAssistTarget>& OutNewTargets);

	/** Get a Player Controller's FOV scaled based on their current input type. */
	static UE_API float GetFOVScale(const APlayerController* PC, ECommonInputType InputType);

	/** Get the collision channel that should be used to find targets within the player's view. */
	UE_API ECollisionChannel GetAimAssistChannel() const;
	
protected:

	/**
	 * Returns true if the given target passes the filter based on the current player owner data.
	 * False if the given target should be excluded from aim assist calculations 
	 */
	UE_API bool DoesTargetPassFilter(const FDCAimAssistOwnerViewData& OwnerData, const FDCAimAssistFilter& Filter, const FDCAimAssistTargetOptions& Target, const float AcceptableRange) const;

	/** Determine if the given target is visible based on our current view data. */
	UE_API void DetermineTargetVisibility(FDCAimAssistTarget& Target, const FDCAimAssistSettings& Settings, const FDCAimAssistFilter& Filter, const FDCAimAssistOwnerViewData& OwnerData);
	
	/** Setup CollisionQueryParams to ignore a set of actors based on filter settings. Such as Ignoring Requester or Instigator. */
	UE_API void InitTargetSelectionCollisionParams(FCollisionQueryParams& OutParams, const AActor& RequestedBy, const FDCAimAssistFilter& Filter) const;
};

#undef UE_API
