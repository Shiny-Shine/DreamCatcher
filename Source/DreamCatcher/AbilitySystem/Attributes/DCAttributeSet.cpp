#include "AbilitySystem/Attributes/DCAttributeSet.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Engine/World.h"

// AttributeSet들이 공통 기능을 재사용
UDCAttributeSet::UDCAttributeSet()
{
}

UWorld* UDCAttributeSet::GetWorld() const
{
	const UObject* OuterObject = GetOuter();

	return OuterObject ? OuterObject->GetWorld() : nullptr;
}

UDCAbilitySystemComponent* UDCAttributeSet::GetDCAbilitySystemComponent() const
{
	return Cast<UDCAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
