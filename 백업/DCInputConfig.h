#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DCInputConfig.generated.h"

class UInputAction;

/**
 * InputAction과 GameplayTag 한 쌍.
 *
 * 예:
 * IA_Move  → InputTag.Move
 * IA_Fire  → InputTag.Weapon.Fire
 */
USTRUCT(BlueprintType)
struct FDCInputAction
{
	GENERATED_BODY()

	// Enhanced Input의 InputAction 에셋.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	// InputAction을 식별할 Gameplay Tag.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 플레이어 입력 설정 DataAsset.
 *
 * NativeInputActions:
 *   Move, Look처럼 C++ 함수에 직접 연결할 입력.
 *
 * AbilityInputActions:
 *   Jump, Aim, Dodge, Fire처럼 ASC에 전달할 입력.
 */
UCLASS(BlueprintType, Const,
	Meta = (DisplayName = "DreamCatcher Input Config", ShortTooltip = "Maps Input Actions to Gameplay Input Tags."))
class DREAMCATCHER_API UDCInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UDCInputConfig(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// NativeInputActions에서 해당 태그의 InputAction을 찾음.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Input")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	// AbilityInputActions에서 해당 태그의 InputAction을 찾음.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Input")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * C++ 함수에 직접 연결할 입력.
	 *
	 * 현재 대상:
	 * - Move
	 * - Look
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Input",
		meta = (TitleProperty = "InputAction"))
	TArray<FDCInputAction> NativeInputActions;

	/**
	 * ASC에 InputTag를 전달할 입력.
	 *
	 * 현재 대상:
	 * - Jump
	 * - Aim
	 * - Dodge
	 * - Ultimate
	 * - Fire
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Input",
		meta = (TitleProperty = "InputAction"))
	TArray<FDCInputAction> AbilityInputActions;
};
