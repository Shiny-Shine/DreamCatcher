// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraWeaponUserInterface.h"

#include "Equipment/Lyra/DCLyraEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "Weapon/Lyra/DCLyraWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraWeaponUserInterface)

struct FGeometry;

UDCLyraWeaponUserInterface::UDCLyraWeaponUserInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCLyraWeaponUserInterface::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDCLyraWeaponUserInterface::NativeDestruct()
{
	Super::NativeDestruct();
}

void UDCLyraWeaponUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UDCLyraEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UDCLyraEquipmentManagerComponent>())
		{
			if (UDCLyraWeaponInstance* NewInstance = EquipmentManager->GetFirstInstanceOfType<UDCLyraWeaponInstance>())
			{
				if (NewInstance != CurrentInstance && NewInstance->GetInstigator() != nullptr)
				{
					UDCLyraWeaponInstance* OldWeapon = CurrentInstance;
					CurrentInstance = NewInstance;
					RebuildWidgetFromWeapon();
					OnWeaponChanged(OldWeapon, CurrentInstance);
				}
			}
		}
	}
}

void UDCLyraWeaponUserInterface::RebuildWidgetFromWeapon()
{

}
