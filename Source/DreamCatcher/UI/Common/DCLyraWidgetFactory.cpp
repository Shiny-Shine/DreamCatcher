// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/Common/DCLyraWidgetFactory.h"
#include "Templates/SubclassOf.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraWidgetFactory)

class UUserWidget;

TSubclassOf<UUserWidget> UDCLyraWidgetFactory::FindWidgetClassForData_Implementation(const UObject* Data) const
{
	return TSubclassOf<UUserWidget>();
}
