// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/CapsuleComponent.h"
#include "GameplayTagContainer.h"
#include "Input/DCAimAssistTargetInterface.h"

#include "DCAimAssistTargetComponent.generated.h"

#define UE_API DREAMCATCHER_API

class UObject;

/**
 * This component can be added to any actor to have it register with the Aim Assist Target Manager.
 */
UCLASS(MinimalAPI, BlueprintType, meta=(BlueprintSpawnableComponent))
class UDCAimAssistTargetComponent : public UCapsuleComponent, public IDCAimAssistTaget
{
	GENERATED_BODY()

public:
	
	//~ Begin IDCAimAssistTaget interface
	UE_API virtual void GatherTargetOptions(OUT FDCAimAssistTargetOptions& TargetData) override;
	//~ End IDCAimAssistTaget interface
	
protected:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDCAimAssistTargetOptions TargetData {};
};

#undef UE_API
