// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraEquipmentDefinition.h"
#include "DCLyraEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraEquipmentDefinition)

UDCLyraEquipmentDefinition::UDCLyraEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UDCLyraEquipmentInstance::StaticClass();
}
