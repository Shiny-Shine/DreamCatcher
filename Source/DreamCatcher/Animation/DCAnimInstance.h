#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "DCAnimInstance.generated.h"

class UAbilitySystemComponent;

// GAS GameplayTag를 AnimBP 변수에 자동 연결하는 DreamCatcher AnimInstance 기본 클래스.
UCLASS(Config = Game)
class DREAMCATCHER_API UDCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UDCAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent);

protected:
	virtual void NativeInitializeAnimation() override;

	/**
	 * Gameplay Tag와 AnimInstance 프로퍼티를 연결.
	 *
	 * 예:State.Aim.Shoulder, bIsShoulderAiming
	 */
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Gameplay Tags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;
};
