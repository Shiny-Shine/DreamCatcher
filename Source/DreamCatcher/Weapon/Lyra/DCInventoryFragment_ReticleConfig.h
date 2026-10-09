// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Inventory/DCInventoryItemDefinition.h"

#include "DCInventoryFragment_ReticleConfig.generated.h"

class UDCLyraReticleWidgetBase;
class UObject;

UCLASS()
class UDCInventoryFragment_ReticleConfig : public UDCInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Reticle)
	TArray<TSubclassOf<UDCLyraReticleWidgetBase>> ReticleWidgets;
};
