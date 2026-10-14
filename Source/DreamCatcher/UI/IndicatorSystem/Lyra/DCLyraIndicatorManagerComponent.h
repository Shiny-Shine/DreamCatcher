// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/ControllerComponent.h"

#include "DCLyraIndicatorManagerComponent.generated.h"

#define UE_API DREAMCATCHER_API

class AController;
class UDCLyraIndicatorDescriptor;
class UObject;
struct FFrame;

/**
 * @class UDCLyraIndicatorManagerComponent
 */
UCLASS(MinimalAPI, BlueprintType, Blueprintable)
class UDCLyraIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UE_API UDCLyraIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer);

	static UE_API UDCLyraIndicatorManagerComponent* GetComponent(AController* Controller);

	UFUNCTION(BlueprintCallable, Category = Indicator)
	UE_API void AddIndicator(UDCLyraIndicatorDescriptor* IndicatorDescriptor);

	UFUNCTION(BlueprintCallable, Category = Indicator)
	UE_API void RemoveIndicator(UDCLyraIndicatorDescriptor* IndicatorDescriptor);

	DECLARE_EVENT_OneParam(UDCLyraIndicatorManagerComponent, FIndicatorEvent, UDCLyraIndicatorDescriptor* Descriptor)
	FIndicatorEvent OnIndicatorAdded;
	FIndicatorEvent OnIndicatorRemoved;

	const TArray<UDCLyraIndicatorDescriptor*>& GetIndicators() const { return Indicators; }

private:
	UPROPERTY()
	TArray<TObjectPtr<UDCLyraIndicatorDescriptor>> Indicators;
};

#undef UE_API
