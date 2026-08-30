#include "AbilitySystem/Abilities/DCGameplayAbility_Death.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Components/DCHealthComponent.h"
#include "DreamCatcher.h"

UDCGameplayAbility_Death::UDCGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	bAutoStartDeath = true;

	// Blueprint에서 Trigger를 매번 설정하지 않도록 CDO에 GameplayEvent.Death Trigger를 등록.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = DCGameplayTags::GameplayEvent_Death;

		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

		AbilityTriggers.Add(TriggerData);
	}
}

void UDCGameplayAbility_Death::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo
	ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	check(ActorInfo);

	UDCAbilitySystemComponent* DCAbilitySystem = CastChecked<UDCAbilitySystemComponent>(
		ActorInfo->AbilitySystemComponent.Get());

	// 현재 실행 중인 다른 Ability를 모두 취소. 추후 Ability.Behavior.SurvivesDeath 태그를 추가하면 일부 수동 Ability 제외 가능.
	DCAbilitySystem->CancelAbilities(nullptr, nullptr, this);

	// 남아 있는 입력 상태 제거.
	DCAbilitySystem->ClearAbilityInput();
	// Scope와 Shoulder GameplayEffect 제거.
	DCAbilitySystem->ClearAimState();

	SetCanBeCanceled(false);
	
	UE_LOG(LogDreamCatcher, Log, TEXT("Death Ability [%s] activated for [%s]."), *GetNameSafe(this),
		   *GetNameSafe(GetAvatarActorFromActorInfo()));

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UDCGameplayAbility_Death::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo
	ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UDCGameplayAbility_Death::StartDeath()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!AvatarActor)
	{
		return;
	}

	if (UDCHealthComponent* HealthComponent = AvatarActor->FindComponentByClass<UDCHealthComponent>())
	{
		if (HealthComponent->GetDeathState() == EDCDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void UDCGameplayAbility_Death::FinishDeath()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!AvatarActor)
	{
		return;
	}

	if (
		UDCHealthComponent* HealthComponent = AvatarActor->FindComponentByClass<UDCHealthComponent>())
	{
		if (HealthComponent->GetDeathState() == EDCDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}
