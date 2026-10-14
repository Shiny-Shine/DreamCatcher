#pragma once

#include "Abilities/GameplayAbility.h"
#include "DCGameplayAbility.generated.h"

// 어빌리티를 언제 자동으로 활성화할지 정의, 실제 입력 처리는 이후 DCAbilitySystemComponent에서 구현.
UENUM(BlueprintType)
enum class EDCAbilityActivationPolicy : uint8
{
	// 입력을 누른 순간 한 번 활성화.
	OnInputTriggered UMETA(DisplayName = "On Input Triggered"),

	// 입력을 누르고 있는 동안 어빌리티가 끝날 때마다 계속 활성화를 시도.
	WhileInputActive UMETA(DisplayName = "While Input Active"),

	// 유효한 아바타가 연결되면 자동으로 활성화.
	OnSpawn UMETA(DisplayName = "On Spawn")
};

// DreamCatcher의 모든 Gameplay Ability가 상속할 기본 클래스.
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UDCGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ASC가 입력 처리 중 활성화 정책을 확인할 때 사용.
	EDCAbilityActivationPolicy GetActivationPolicy() const
	{
		return ActivationPolicy;
	}

protected:
	// 이 Ability가 입력이나 Avatar 초기화에 어떻게 반응할지 결정.
	// 자식 Gameplay Ability Blueprint의 Class Defaults에서 설정. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Ability Activation")
	EDCAbilityActivationPolicy ActivationPolicy = EDCAbilityActivationPolicy::OnInputTriggered;
};