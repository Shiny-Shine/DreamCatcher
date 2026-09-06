#include "Equipment/DCEquipmentDefinition.h"

#include "Equipment/DCEquipmentInstance.h"

UDCEquipmentDefinition::UDCEquipmentDefinition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// 아직 무기 전용 Instance가 없어도 기본 장비 객체를 생성할 수 있음.
	InstanceType = UDCEquipmentInstance::StaticClass();
}
