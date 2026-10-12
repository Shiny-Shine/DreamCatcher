// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DCGameplayAbility.h"

#include "DCLyraGameplayAbility_Jump.generated.h"

class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;


/**
 * UDCLyraGameplayAbility_Jump
 *
 *	Gameplay ability used for character jumping.
 */
UCLASS(Abstract)
class UDCLyraGameplayAbility_Jump : public UDCGameplayAbility
{
	GENERATED_BODY()

public:

	UDCLyraGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	void CharacterJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	void CharacterJumpStop();
};
