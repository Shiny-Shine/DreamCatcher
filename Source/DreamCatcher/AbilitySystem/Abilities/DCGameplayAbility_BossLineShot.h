#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "DCGameplayAbility_BossLineShot.generated.h"

class ADCBossTelegraph;

/** Adds only presentation hooks to the accepted R10 single-shot implementation. */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_BossLineShot : public UDCGameplayAbility_EnemyRangedAttack
{
	GENERATED_BODY()
public:
	UDCGameplayAbility_BossLineShot(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void BP_OnAttackIntent_Implementation(FVector TraceStart, FVector AimPoint, float DelaySeconds) override;
	virtual void BP_OnAttackShot_Implementation(FVector TraceStart, FVector TraceEnd, AActor* HitActor) override;
	virtual void BP_OnAttackEnded_Implementation(bool bWasCancelled) override;
private:
	TWeakObjectPtr<ADCBossTelegraph> Warning;
};
