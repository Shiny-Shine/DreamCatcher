#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/DCInputConfig.h"
#include "DCInputComponent.generated.h"

/**
 * DreamCatcher 전용 Enhanced Input Component.
 *
 * DCInputConfig의 InputAction → InputTag 데이터를 사용해
 * Native 입력과 GAS Ability 입력을 바인딩.
 */
UCLASS(Config = Input)
class DREAMCATCHER_API UDCInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UDCInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * NativeInputActions에서 InputTag에 해당하는 InputAction을 찾아
	 * 일반 C++ 입력 함수에 연결.
	 *
	 * 예:
	 * InputTag.Move → ADreamCatcherCharacter::Move
	 */
	template <class UserClass, typename FuncType>
	void BindNativeAction(const UDCInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent,
	                      UserClass* Object, FuncType Func, bool bLogIfNotFound = true);

	/**
	 * AbilityInputActions 배열 전체를 입력 태그 전달 함수에 연결.
	 *
	 * Started:
	 *   Input_AbilityInputTagPressed(InputTag)
	 *
	 * Completed 또는 Canceled:
	 *   Input_AbilityInputTagReleased(InputTag)
	 */
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc,
	                        ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	// BindHandles에 기록된 입력 바인딩을 모두 제거.
	void RemoveBinds(TArray<uint32>& BindHandles);
};

/**
 * Template 함수는 사용하는 위치에서 구현을 볼 수 있어야 하므로
 * CPP가 아니라 헤더에 구현.
 */
template <class UserClass, typename FuncType>
void UDCInputComponent::BindNativeAction(const UDCInputConfig* InputConfig, const FGameplayTag& InputTag,
                                         ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func,
                                         bool bLogIfNotFound)
{
	check(InputConfig);
	check(Object);

	const UInputAction* InputAction = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound);

	if (InputAction)
	{
		BindAction(InputAction, TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UDCInputComponent::BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object,
                                           PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
                                           TArray<uint32>& BindHandles)
{
	check(InputConfig);
	check(Object);

	for (const FDCInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}

		if (PressedFunc)
		{
			const FEnhancedInputActionEventBinding& PressedBinding = BindAction(
				Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag);

			BindHandles.Add(PressedBinding.GetHandle());
		}

		if (ReleasedFunc)
		{
			const FEnhancedInputActionEventBinding& CompletedBinding = BindAction(
				Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);

			BindHandles.Add(CompletedBinding.GetHandle());

			const FEnhancedInputActionEventBinding& CanceledBinding = BindAction(
				Action.InputAction, ETriggerEvent::Canceled, Object, ReleasedFunc, Action.InputTag);

			BindHandles.Add(CanceledBinding.GetHandle());
		}
	}
}
