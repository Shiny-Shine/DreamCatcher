#include "AbilitySystem/Abilities/DCGameplayAbility_Aim.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "GameplayEffect.h"

UDCGameplayAbility_Aim::UDCGameplayAbility_Aim(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ActivationPolicy = EDCAbilityActivationPolicy::OnInputTriggered;
}

void UDCGameplayAbility_Aim::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent();

	if (!AbilitySystemComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	bShoulderAimActive = false;
	ActiveShoulderEffectHandle = FActiveGameplayEffectHandle();

	/*
	 * Scope 상태에서 우클릭을 누른 경우:
	 *
	 * 누르는 순간 즉시 Hip으로 돌아가고 Ability를 종료.
	 * Ability가 이미 종료됐으므로 이후 Release 입력은 아무 상태 변화도 X.
	 */
	if (AbilitySystemComponent->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope))
	{
		AbilitySystemComponent->ClearAimState();

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);

		return;
	}

	// 비정상적으로 남아 있을 수 있는 기존 조준 Effect 정리.
	AbilitySystemComponent->ClearAimState();

	/*
	 * Input Release와 Hold Threshold를 동시에 대기.
	 *
	 * Release가 먼저: Scope, Delay가 먼저: Shoulder
	 */
	WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);

	WaitInputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::HandleInputReleased);

	WaitInputReleaseTask->ReadyForActivation();
	
	// 이미 입력이 해제된 상태라면 Task 활성화 중에 HandleInputReleased가 즉시 실행되어 Ability가 끝날 수 있음.
	if (!IsActive())
	{
		return;
	}

	if (ShoulderHoldThreshold <= 0.0f)
	{
		HandleShoulderThresholdReached();
		return;
	}

	ShoulderDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ShoulderHoldThreshold);

	ShoulderDelayTask->OnFinish.AddDynamic(this, &ThisClass::HandleShoulderThresholdReached);

	ShoulderDelayTask->ReadyForActivation();
}

void UDCGameplayAbility_Aim::HandleInputReleased(float TimeHeld)
{
	(void)TimeHeld;

	// WaitInputRelease는 Release 발생 후 자체 종료됨.
	WaitInputReleaseTask = nullptr;

	if (ShoulderDelayTask)
	{
		ShoulderDelayTask->EndTask();
		ShoulderDelayTask = nullptr;
	}

	UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent();

	if (!AbilitySystemComponent)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

		return;
	}

	if (bShoulderAimActive)
	{
		// Hold 후 해제: Shoulder → Hip
		RemoveShoulderEffect();
	}
	else
	{
		// Threshold 전에 해제: Hip → Scope
		AbilitySystemComponent->ClearAimState();

		ApplyAimEffect(ScopeEffectClass);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UDCGameplayAbility_Aim::HandleShoulderThresholdReached()
{
	// WaitDelay는 OnFinish 발생 후 자체 종료됨.
	ShoulderDelayTask = nullptr;

	UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent();

	if (!AbilitySystemComponent)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

		return;
	}

	AbilitySystemComponent->ClearAimState();

	ActiveShoulderEffectHandle = ApplyAimEffect(ShoulderEffectClass);

	if (!ActiveShoulderEffectHandle.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

		return;
	}

	bShoulderAimActive = true;
}

FActiveGameplayEffectHandle
UDCGameplayAbility_Aim::ApplyAimEffect(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!EffectClass || !CurrentActorInfo)
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());

	if (!EffectSpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, EffectSpecHandle);
}

void UDCGameplayAbility_Aim::RemoveShoulderEffect()
{
	if (!ActiveShoulderEffectHandle.IsValid())
	{
		bShoulderAimActive = false;
		return;
	}

	if (UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveShoulderEffectHandle);
	}

	ActiveShoulderEffectHandle = FActiveGameplayEffectHandle();

	bShoulderAimActive = false;
}

UDCAbilitySystemComponent* UDCGameplayAbility_Aim::GetDCAbilitySystemComponent() const
{
	return Cast<UDCAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

void UDCGameplayAbility_Aim::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (WaitInputReleaseTask)
	{
		WaitInputReleaseTask->EndTask();
		WaitInputReleaseTask = nullptr;
	}

	if (ShoulderDelayTask)
	{
		ShoulderDelayTask->EndTask();
		ShoulderDelayTask = nullptr;
	}

	/*
	 * Scope Effect는 짧은 클릭 후 Ability가 끝나도 유지해야 하므로 여기서 제거 X
	 * Shoulder Effect만 Ability 수명과 함께 정리.
	 */
	RemoveShoulderEffect();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
