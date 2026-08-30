#pragma once

#include "AbilitySystemComponent.h"
#include "DCAbilitySystemComponent.generated.h"

/**
 * DreamCatcher의 공통 Ability System Component.
 *
 * PlayerState가 소유하거나 적/보스 Character가 직접 소유할 수 있음.
 * 입력 태그를 Ability Spec Handle로 변환하고 매 프레임 처리.
 */
UCLASS()
class DREAMCATCHER_API UDCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UDCAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// InputTag와 연결된 Ability를 Pressed/Held 상태로 기록.
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	// InputTag와 연결된 Ability를 Released 상태로 기록.
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// 기록된 입력 상태를 이용해 Ability를 활성화하거나 입력 이벤트를 전달.
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	// Possession 해제나 입력 차단 시 모든 입력 상태를 초기화.
	void ClearAbilityInput();
	
	// Scope와 Shoulder 조준 GameplayEffect를 모두 제거하여 Hip으로 돌아감.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Aim")
	void ClearAimState();

protected:
	// 활성화된 Ability에 WaitInputPress 이벤트를 전달.
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;

	// 활성화된 Ability에 WaitInputRelease 이벤트를 전달.
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

private:
	// 이번 프레임에 누른 Ability.
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// 이번 프레임에 해제한 Ability.
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// 현재 계속 누르고 있는 Ability.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
