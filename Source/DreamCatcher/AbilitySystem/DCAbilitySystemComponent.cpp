#include "AbilitySystem/DCAbilitySystemComponent.h"

UDCAbilitySystemComponent::UDCAbilitySystemComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// 플레이어, 적, 보스 모두 같은 ASC 클래스를 사용할 수 있도록 복제 가능한 컴포넌트로 설정.
	SetIsReplicatedByDefault(true);
}