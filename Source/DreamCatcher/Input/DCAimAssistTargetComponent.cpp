// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/DCAimAssistTargetComponent.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Actor.h"

#include "Input/DCAimAssistTargetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAimAssistTargetComponent)

void UDCAimAssistTargetComponent::GatherTargetOptions(FDCAimAssistTargetOptions& OutTargetData)
{
	if (!TargetData.TargetShapeComponent.IsValid())
	{
		if (AActor* Owner = GetOwner())
		{
			TargetData.TargetShapeComponent = Owner->FindComponentByClass<UShapeComponent>();	
		}
	}
	OutTargetData = TargetData;
}

