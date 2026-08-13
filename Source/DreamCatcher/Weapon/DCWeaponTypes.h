#pragma once

#include "CoreMinimal.h"
#include "DCWeaponTypes.generated.h"


// 캐릭터가 무기를 들 때 사용할 애니메이션 자세 유형. 
UENUM(BlueprintType)
enum class EDCWeaponAnimationType : uint8
{
	Unarmed UMETA(DisplayName = "Unarmed"),
	Rifle UMETA(DisplayName = "Rifle"),
	Pistol UMETA(DisplayName = "Pistol"),
	Heavy UMETA(DisplayName = "Heavy")
};