#pragma once

#include "AbilitySystemComponent.h"
#include "DCAbilitySystemComponent.generated.h"

/**
 * Ability System Component 기본 클래스.
 *
 * 플레이어의 경우 PlayerState가 이 컴포넌트를 소유하고,Character가 Avatar Actor가 됨.
 *
 * 일반 적과 보스는 Character가 직접 소유할 수 있음.
 */
UCLASS()
class DREAMCATCHER_API UDCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UDCAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};