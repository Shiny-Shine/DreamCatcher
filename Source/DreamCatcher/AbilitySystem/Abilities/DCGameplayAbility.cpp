#include "AbilitySystem/Abilities/DCGameplayAbility.h"

UDCGameplayAbility::UDCGameplayAbility(
	const FObjectInitializer& ObjectInitializer
)
	: Super(ObjectInitializer)
{
	// Ability를 사용하는 Actor마다 별도의 Ability 객체를 만듬. 
	// 이로 인해 Ability가 실행되는 동안 Timer, AbilityTask, Montage 상태 같은 런타임 데이터를 안전하게 가질 수 있음. 
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}