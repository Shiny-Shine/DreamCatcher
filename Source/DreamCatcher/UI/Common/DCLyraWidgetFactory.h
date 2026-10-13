// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "DCLyraWidgetFactory.generated.h"

#define UE_API DREAMCATCHER_API

template <class TClass> class TSubclassOf;

class UUserWidget;
struct FFrame;

UCLASS(MinimalAPI, Abstract, Blueprintable, BlueprintType, EditInlineNew)
class UDCLyraWidgetFactory : public UObject
{
	GENERATED_BODY()

public:
	UDCLyraWidgetFactory() { }

	UFUNCTION(BlueprintNativeEvent)
	UE_API TSubclassOf<UUserWidget> FindWidgetClassForData(const UObject* Data) const;
};

#undef UE_API
