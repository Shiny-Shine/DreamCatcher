// Native fixtures only. Never granted by production AbilitySets or saved assets.
#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "GameplayEffect.h"
#include "DCDashDefenseTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCDashDefenseAutomationAbility : public UDCGameplayAbility
{
	GENERATED_BODY()
public:
	UDCDashDefenseAutomationAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void RecordOpened(const FGameplayEventData& Event);
	UFUNCTION()
	void RecordClosed(const FGameplayEventData& Event);
	UFUNCTION()
	void RecordAvoided(const FGameplayEventData& Event);
	bool bCommitted = false;
	int32 Opened = 0;
	int32 Closed = 0;
	int32 Avoided = 0;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCDashDefenseAutomationEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UDCDashDefenseAutomationEffect();
};
