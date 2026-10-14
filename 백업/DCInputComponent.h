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

	// 정상 해제와 입력 취소를 서로 다른 함수에 연결합니다.
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename CanceledFuncType>
	void BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc,
	                        ReleasedFuncType ReleasedFunc, CanceledFuncType CanceledFunc, TArray<uint32>& BindHandles);

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

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename CanceledFuncType>
void UDCInputComponent::BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object,
                                           PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
                                           CanceledFuncType CanceledFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);
	check(Object);

	for (const FDCInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}

		// 버튼을 누른 순간.
		if (PressedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction,
			                           ETriggerEvent::Started, Object, PressedFunc, Action.InputTag).GetHandle());
		}

		// 정상적으로 입력을 끝낸 경우.
		if (ReleasedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc,
			                           Action.InputTag).GetHandle());
		}

		// 입력 처리가 취소된 경우. Aim에서는 이 이벤트로 Scope를 켜면 X.
		if (CanceledFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Canceled, Object, CanceledFunc,
			                           Action.InputTag).GetHandle());
		}
	}
}
