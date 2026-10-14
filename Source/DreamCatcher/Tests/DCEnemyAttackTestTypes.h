// Native transient fixtures only; never select these in gameplay assets.
#pragma once

#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "Tests/DCEnemyGASTestTypes.h"
#include "DCEnemyAttackTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10AttackDamageFixture : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UDCR10AttackDamageFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10AttackAbilityFixture : public UDCGameplayAbility_EnemyRangedAttack
{
	GENERATED_BODY()
public:
	UDCR10AttackAbilityFixture();
	virtual void BP_OnAttackIntent_Implementation(FVector TraceStart, FVector AimPoint, float DelaySeconds) override;
	virtual void BP_OnAttackShot_Implementation(FVector TraceStart, FVector TraceEnd, AActor* HitActor) override;
	virtual void BP_OnAttackEnded_Implementation(bool bWasCancelled) override;
	void RemoveEffectForTest() { DamageEffectClass = nullptr; }
	int32 Intents = 0;
	int32 Shots = 0;
	int32 Ends = 0;
	int32 Cancellations = 0;
	bool bCancelOnIntent = false;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10AttackSetFixture : public UDCAbilitySet
{
	GENERATED_BODY()
public:
	UDCR10AttackSetFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10AttackEnemyFixture : public ADCR10EnemyFixture
{
	GENERATED_BODY()
public:
	ADCR10AttackEnemyFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10AutoAttackEnemyFixture : public ADCR10AttackEnemyFixture
{
	GENERATED_BODY()
public:
	ADCR10AutoAttackEnemyFixture();
};
