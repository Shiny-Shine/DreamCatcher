// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UI/Common/DCLyraWidgetFactory.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "DCLyraWidgetFactory_Class.generated.h"

#define UE_API DREAMCATCHER_API

class UObject;
class UUserWidget;

UCLASS(MinimalAPI)
class UDCLyraWidgetFactory_Class : public UDCLyraWidgetFactory
{
	GENERATED_BODY()

public:
	UDCLyraWidgetFactory_Class() { }

	UE_API virtual TSubclassOf<UUserWidget> FindWidgetClassForData_Implementation(const UObject* Data) const override;
	
protected:
	UPROPERTY(EditAnywhere, Category = ListEntries, meta = (AllowAbstract))
	TMap<TSoftClassPtr<UObject>, TSubclassOf<UUserWidget>> EntryWidgetForClass;
};

#undef UE_API
