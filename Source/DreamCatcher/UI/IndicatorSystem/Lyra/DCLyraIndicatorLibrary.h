// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "DCLyraIndicatorLibrary.generated.h"

#define UE_API DREAMCATCHER_API

class AController;
class UDCLyraIndicatorManagerComponent;
class UObject;
struct FFrame;

UCLASS(MinimalAPI)
class UDCLyraIndicatorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UE_API UDCLyraIndicatorLibrary();

	/**  */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	static UE_API UDCLyraIndicatorManagerComponent* GetIndicatorManagerComponent(AController* Controller);
};

#undef UE_API
