#pragma once

#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossLineShot.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "Tests/DCEnemyAttackTestTypes.h"
#include "DCBossTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR12DataFixture : public UDCBossData
{
	GENERATED_BODY()
public:
	UDCR12DataFixture();
};
UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR12LineFixture : public UDCGameplayAbility_BossLineShot
{
	GENERATED_BODY()
public:
	UDCR12LineFixture();
};
UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR12GroundFixture : public UDCGameplayAbility_BossGroundStrike
{
	GENERATED_BODY()
public:
	UDCR12GroundFixture();
};
UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR12SetFixture : public UDCAbilitySet
{
	GENERATED_BODY()
public:
	UDCR12SetFixture();
};
UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR12BossFixture : public ADCBossCharacter
{
	GENERATED_BODY()
public:
	ADCR12BossFixture();
};
UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR12SpawnerFixture : public ADCR10SpawnerFixture
{
	GENERATED_BODY()
public:
	void ConfigureOwned() { ConfigureGAS(ADCR12BossFixture::StaticClass()); bCleanupOwnedGASNPCs = true; }
};
