// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"

#include "IDCLyraActorIndicatorWidget.generated.h"

class AActor;
class UDCLyraIndicatorDescriptor;

UINTERFACE(MinimalAPI, BlueprintType)
class UDCLyraIndicatorWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

class IDCLyraIndicatorWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	void BindIndicator(UDCLyraIndicatorDescriptor* Indicator);

	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	void UnbindIndicator(const UDCLyraIndicatorDescriptor* Indicator);
};
