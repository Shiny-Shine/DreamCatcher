#include "Animation/DCAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UDCAnimInstance::UDCAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDCAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	GameplayTagPropertyMap.Initialize(this, AbilitySystemComponent);
}

void UDCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	AActor* OwningActor = GetOwningActor();

	if (!OwningActor)
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystemComponent =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
	{
		InitializeWithAbilitySystem(AbilitySystemComponent);
	}
}
