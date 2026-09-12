// Copyright Epic Games, Inc. All Rights Reserved.

// 이 구조체는 아래 정보를 전달함.
// - 누구의 Ability가 실패했는지
// - 어떤 이유 태그로 실패했는지
// - 사용자에게 보여줄 설명이 무엇인지

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

#include "DCAbilitySimpleFailureMessage.generated.h"

class APlayerController;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ABILITY_SIMPLE_FAILURE_MESSAGE);

USTRUCT(BlueprintType)
struct FDCAbilitySimpleFailureMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer FailureTags;

	UPROPERTY(BlueprintReadWrite)
	FText UserFacingReason;
};