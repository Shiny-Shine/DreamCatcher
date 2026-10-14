#include "Weapon/DCWeaponInstance.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/World.h"
#include "Weapon/DCWeaponActor.h"

UDCWeaponInstance::UDCWeaponInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDCWeaponInstance::OnEquipped()
{
	if (IsEquipped())
	{
		return;
	}

	// Blueprint의 On Equipped 이벤트가 실행되기 전에 초기화.
	LastFireTime = -1.0;
	Super::OnEquipped();
}

ADCWeaponActor* UDCWeaponInstance::GetWeaponActor() const
{
	for (AActor* Actor : GetSpawnedActors())
	{
		if (ADCWeaponActor* WeaponActor = Cast<ADCWeaponActor>(Actor))
		{
			return WeaponActor;
		}
	}
	return nullptr;
}

void UDCWeaponInstance::UpdateFiringTime()
{
	if (UWorld* World = GetWorld())
	{
		LastFireTime = World->GetTimeSeconds();
	}
}

float UDCWeaponInstance::GetTimeSinceLastFired() const
{
	const UWorld* World = GetWorld();

	if (!World || LastFireTime < 0.0)
	{
		return -1.0f;
	}

	return static_cast<float>(World->GetTimeSeconds() - LastFireTime);
}
