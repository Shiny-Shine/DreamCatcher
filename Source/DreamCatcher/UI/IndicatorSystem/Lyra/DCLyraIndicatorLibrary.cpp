// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraIndicatorLibrary.h"

#include "DCLyraIndicatorManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraIndicatorLibrary)

class AController;

UDCLyraIndicatorLibrary::UDCLyraIndicatorLibrary()
{
}

UDCLyraIndicatorManagerComponent* UDCLyraIndicatorLibrary::GetIndicatorManagerComponent(AController* Controller)
{
	return UDCLyraIndicatorManagerComponent::GetComponent(Controller);
}
