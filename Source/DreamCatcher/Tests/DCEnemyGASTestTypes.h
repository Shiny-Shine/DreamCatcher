// Native, unsaved R10 fixtures. Never use these classes in production assets.
#pragma once

#include "AI/DCEnemyCharacter.h"
#include "AI/DCEnemySpawner.h"
#include "AI/DCGASEnemyCharacter.h"
#include "AbilitySystem/Abilities/DCLyraGameplayAbility_Death.h"
#include "GameplayEffect.h"
#include "Stage/DCEncounterController.h"
#include "DCEnemyGASTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10DeathFixture : public UDCLyraGameplayAbility_Death
{
	GENERATED_BODY()
protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void FinishFixtureDeath();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10MarkerAbility : public UDCGameplayAbility
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10MarkerEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UDCR10MarkerEffect();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10ExtraSet : public UDCAbilitySet
{
	GENERATED_BODY()
public:
	UDCR10ExtraSet();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10EnemyFixture : public ADCGASEnemyCharacter
{
	GENERATED_BODY()
public:
	ADCR10EnemyFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10UnconfiguredEnemyFixture : public ADCR10EnemyFixture
{
	GENERATED_BODY()
public:
	ADCR10UnconfiguredEnemyFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10ImmediateEnemyFixture : public ADCR10EnemyFixture
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10LegacyEnemyFixture : public ADCEnemyCharacter
{
	GENERATED_BODY()
public:
	ADCR10LegacyEnemyFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10SpawnerFixture : public ADCEnemySpawner
{
	GENERATED_BODY()
public:
	void ConfigureGAS(TSubclassOf<ADCGASEnemyCharacter> Class, int32 Count = 1, int32 Concurrent = 1);
	void ConfigureLegacy();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10EncounterFixture : public ADCEncounterController
{
	GENERATED_BODY()
public:
	void Configure(ADCEnemySpawner* First, ADCEnemySpawner* Second = nullptr);
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10Observer : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void EnemyDied(ADCGASEnemyCharacter* Enemy);
	UFUNCTION()
	void SpawnerCleared(ADCEnemySpawner* Spawner);
	UFUNCTION()
	void EncounterCompleted(ADCEncounterController* Encounter);
	int32 Deaths = 0;
	int32 Clears = 0;
	int32 Encounters = 0;
};
