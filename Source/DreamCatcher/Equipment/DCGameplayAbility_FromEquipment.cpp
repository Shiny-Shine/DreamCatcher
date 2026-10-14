#include "Equipment/DCGameplayAbility_FromEquipment.h"

#include "GameplayAbilitySpec.h"
#include "Equipment/DCEquipmentInstance.h"

UDCEquipmentInstance* UDCGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	// 클래스 기본 객체에는 실행 중인 Ability Spec이 없음.
	if (!IsInstantiated())
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();

	if (!Spec)
	{
		return nullptr;
	}

	return Cast<UDCEquipmentInstance>(Spec->SourceObject.Get());
}
