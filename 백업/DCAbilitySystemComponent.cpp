#include "AbilitySystem/DCAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "AbilitySystem/DCGameplayTags.h"

UDCAbilitySystemComponent::UDCAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UDCAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		// AbilitySet에서 Ability Spec에 넣은 InputTag와 비교합니다.
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);

			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UDCAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);

			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UDCAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// 현재 단계에서는 사용하지 않지만 이후 Ability 입력 처리 확장에 사용.
	(void)DeltaTime;
	(void)bGamePaused;

	if (HasMatchingGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked))
	{
		// 입력이 차단되면 조준 대기 작업과 Scope 유지 상태도 종료.
		CancelAimInputAndState();
		ClearAbilityInput();
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	// WhileInputActive Ability 처리.
	// 입력을 누르고 있으며 Ability가 비활성 상태라면 매 프레임 활성화를 다시 시도.
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability || AbilitySpec->IsActive())
		{
			continue;
		}

		const UDCGameplayAbility* AbilityCDO = Cast<UDCGameplayAbility>(AbilitySpec->Ability);

		if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EDCAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	// 이번 프레임에 새로 누른 입력을 처리.
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = true;

		if (AbilitySpec->IsActive())
		{
			// 이미 실행 중이라면 AbilityTask 등에 Press 이벤트를 전달.
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}

		const UDCGameplayAbility* AbilityCDO = Cast<UDCGameplayAbility>(AbilitySpec->Ability);

		if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EDCAbilityActivationPolicy::OnInputTriggered)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	// Held와 Pressed 양쪽에서 같은 Ability가 발견될 수 있으므로 AddUnique로 모은 뒤 한 번씩만 활성화를 시도.
	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	// 이번 프레임에 해제된 입력을 처리.
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = false;

		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	// Pressed와 Released는 한 프레임짜리 상태.
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();

	// Held는 버튼을 뗄 때까지 유지.
}

void UDCAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UDCAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (!Spec.IsActive())
	{
		return;
	}

	/*
	 * WaitInputPress 같은 AbilityTask가 입력 이벤트를 받을 수 있도록
	 * GAS Generic Replicated Event를 발생시킴.
	 */
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	const UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();

	const FPredictionKey PredictionKey = AbilityInstance
		                                     ? AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey()
		                                     : Spec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, PredictionKey);
}

void UDCAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (!Spec.IsActive())
	{
		return;
	}

	// WaitInputRelease가 사용하는 Release 이벤트를 발생시킴. 이후 GA_DC_Aim의 짧은 클릭/Hold 구분에 필요.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	const UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();

	const FPredictionKey PredictionKey = AbilityInstance
		                                     ? AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey()
		                                     : Spec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, PredictionKey);
}

void UDCAbilitySystemComponent::ClearAimState()
{
	FGameplayTagContainer AimStateTags;
	AimStateTags.AddTag(DCGameplayTags::State_Aim_Shoulder);
	AimStateTags.AddTag(DCGameplayTags::State_Aim_Scope);

	// 지정한 태그 중 하나라도 부여하는 활성 GameplayEffect를 제거.
	RemoveActiveEffectsWithGrantedTags(AimStateTags);
}

void UDCAbilitySystemComponent::CancelAimInputAndState()
{
	TArray<FGameplayAbilitySpecHandle> AimHandles;

	// Ability를 취소하면 실행 상태가 바뀔 수 있으므로 먼저 대상 Handle만 별도 배열에 모음.
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(DCGameplayTags::InputTag_Aim))
		{
			AimHandles.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : AimHandles)
	{
		// 취소 직후 남은 입력으로 Aim이 다시 시작되지 않게 함.
		InputPressedSpecHandles.Remove(Handle);
		InputReleasedSpecHandles.Remove(Handle);
		InputHeldSpecHandles.Remove(Handle);

		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle))
		{
			Spec->InputPressed = false;
		}

		// WaitDelay와 WaitInputRelease도 Ability 종료와 함께 정리됨.
		CancelAbilityHandle(Handle);
	}

	// Scope는 Ability 종료 후에도 Effect가 유지되므로 별도 제거.
	ClearAimState();
}

void UDCAbilitySystemComponent::ClearAbilityInputForHandle(const FGameplayAbilitySpecHandle& Handle)
{
	InputPressedSpecHandles.Remove(Handle);
	InputReleasedSpecHandles.Remove(Handle);
	InputHeldSpecHandles.Remove(Handle);

	if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle))
	{
		// 입력 해제 이벤트를 발생시키지 않고 기록만 초기화. 실행 중인 Ability의 취소는 호출한 쪽에서 처리.
		Spec->InputPressed = false;
	}
}
