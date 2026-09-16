// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnhancedInputComponent.h"
#include "DCInputConfig.h"

#include "DCInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;


/**
 * UDCInputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config = Input)
class UDCInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:

	UDCInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const UDCInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UDCInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UDCInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
	
	// 과도기 연결: 기존 Started / Completed / Canceled 입력을 유지합니다.
	// Hero 및 조준 입력의 대체 연결이 검증된 뒤 제거합니다.
	template <class UserClass,typename PressedFuncType,typename ReleasedFuncType,typename CanceledFuncType>
	void BindLegacyAbilityActions(const UDCInputConfig* InputConfig,UserClass* Object,PressedFuncType PressedFunc,ReleasedFuncType ReleasedFunc,CanceledFuncType CanceledFunc,TArray<uint32>& BindHandles);
};


template<class UserClass, typename FuncType>
void UDCInputComponent::BindNativeAction(const UDCInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UDCInputComponent::BindAbilityActions(const UDCInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FDCInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}
}

template <class UserClass,typename PressedFuncType,typename ReleasedFuncType,typename CanceledFuncType>
void UDCInputComponent::BindLegacyAbilityActions(const UDCInputConfig* InputConfig,UserClass* Object,PressedFuncType PressedFunc,ReleasedFuncType ReleasedFunc,CanceledFuncType CanceledFunc,TArray<uint32>& BindHandles)
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
			BindHandles.Add(BindAction(Action.InputAction,ETriggerEvent::Started,Object,PressedFunc,Action.InputTag).GetHandle());
		}

		if (ReleasedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction,ETriggerEvent::Completed,Object,ReleasedFunc,Action.InputTag).GetHandle());
		}

		if (CanceledFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction,ETriggerEvent::Canceled,Object,CanceledFunc,Action.InputTag).GetHandle());
		}
	}
}