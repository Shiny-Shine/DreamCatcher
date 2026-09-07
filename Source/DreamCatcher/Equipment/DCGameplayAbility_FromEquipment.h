#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "DCGameplayAbility_FromEquipment.generated.h"

class UDCEquipmentInstance;

// 장비를 찾는 Ability 기반 클래스.
// 장비를 통해 부여되는 Ability의 공통 기반.
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_FromEquipment : public UDCGameplayAbility
{
	GENERATED_BODY()

public:
	// 현재 Ability를 부여한 장비 Instance를 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	UDCEquipmentInstance* GetAssociatedEquipment() const;
};
