#include "Weapon/DCWeaponActor.h"

#include "Components/SkeletalMeshComponent.h"

ADCWeaponActor::ADCWeaponActor()
{
	// 무기 외형 자체에는 Actor Tick이 필요 X.
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));

	SetRootComponent(WeaponMesh);

	// 손에 부착하는 무기는 물리 시뮬레이션으로 움직이지 않음.
	WeaponMesh->SetSimulatePhysics(false);

	// 무기가 플레이어 이동, 카메라 충돌, 자신의 사격을 방해하지 않게 함.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);
}

bool ADCWeaponActor::TryGetMuzzleTransform(FTransform& OutTransform) const
{
	return TryGetWeaponSocketTransform(MuzzleSocketName, OutTransform);
}

bool ADCWeaponActor::TryGetShellEjectTransform(FTransform& OutTransform) const
{
	return TryGetWeaponSocketTransform(ShellEjectSocketName, OutTransform);
}

bool ADCWeaponActor::TryGetWeaponSocketTransform(
	FName SocketName, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (!WeaponMesh || !WeaponMesh->GetSkeletalMeshAsset() || SocketName.IsNone()
		|| !WeaponMesh->DoesSocketExist(SocketName))
	{
		// 잘못된 소켓 이름을 메시 원점으로 조용히 대체하지 않고 호출한 쪽에서 실패를 확인하고 처리할 수 있게 함.
		return false;
	}

	OutTransform = WeaponMesh->GetSocketTransform(SocketName, RTS_World);

	return true;
}
