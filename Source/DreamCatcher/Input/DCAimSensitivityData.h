// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "DCAimSensitivityData.generated.h"

#define UE_API DREAMCATCHER_API

enum class EDCGamepadSensitivity : uint8;

class UObject;

/** Defines a set of gamepad sensitivity to a float value. */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "DC Aim Sensitivity Data", ShortTooltip = "Data asset used to define a map of Gamepad Sensitivty to a float value."))
class UDCAimSensitivityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API UDCAimSensitivityData(const FObjectInitializer& ObjectInitializer);
	
	UE_API const float SensitivtyEnumToFloat(const EDCGamepadSensitivity InSensitivity) const;
	
protected:
	/** Map of SensitivityMap settings to their corresponding float */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EDCGamepadSensitivity, float> SensitivityMap;
};

#undef UE_API
