// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraIndicatorManagerComponent.h"

#include "GameFramework/Controller.h"

#include "DCLyraIndicatorDescriptor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraIndicatorManagerComponent)

UDCLyraIndicatorManagerComponent::UDCLyraIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

/*static*/ UDCLyraIndicatorManagerComponent* UDCLyraIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<UDCLyraIndicatorManagerComponent>();
	}

	return nullptr;
}

void UDCLyraIndicatorManagerComponent::AddIndicator(UDCLyraIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded.Broadcast(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void UDCLyraIndicatorManagerComponent::RemoveIndicator(UDCLyraIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);

		OnIndicatorRemoved.Broadcast(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}
