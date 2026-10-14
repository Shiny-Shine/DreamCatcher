#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWeaponActor.generated.h"

class USkeletalMeshComponent;

/**
 * 장착 시 화면에 표시할 무기 Actor.
 *
 * 메시와 소켓 정보를 제공합니다.
 * 데미지, 연사, 탄 퍼짐은 이후 Ability와 WeaponInstance가 담당.
 */
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	ADCWeaponActor();

	// Blueprint에서 메시와 무기 애니메이션에 접근할 때 사용.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const
	{
		return WeaponMesh;
	}

	// 총구의 월드 위치와 회전을 반환. 메시 또는 소켓이 없으면 false.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	bool TryGetMuzzleTransform(FTransform& OutTransform) const;

	// 탄피 배출 위치와 회전을 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	bool TryGetShellEjectTransform(FTransform& OutTransform) const;

protected:
	// 실제 표시할 메시를 B_DC_Rifle Blueprint에서 지정.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// 라이플 Skeleton의 총구 소켓 이름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Sockets")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 다른 무기는 Blueprint에서 소켓 이름만 변경할 수 있음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Sockets")
	FName ShellEjectSocketName = TEXT("ShellEject");

private:
	// 소켓이 실제로 존재하는지 확인한 뒤 월드 Transform을 구함.
	bool TryGetWeaponSocketTransform(FName SocketName, FTransform& OutTransform) const;
};
