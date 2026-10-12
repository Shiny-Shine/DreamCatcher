// Unsaved native test fixtures; never use as production Blueprint parents.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Weapon/Lyra/DCLyraWeaponSpawner.h"
#include "DCLyraAIFoundationTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10BotControllerFixture : public ADCLyraPlayerBotController
{
	GENERATED_BODY()
public:
	ADCR10BotControllerFixture();
	void AssignFixturePlayerState(APlayerState* NewState);
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10AIObserver : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void TeamChanged(bool bTeamSet, int32 TeamId);
	UFUNCTION()
	void ExperienceReady();
	int32 TeamCalls = 0;
	int32 LastTeam = INDEX_NONE;
	int32 ReadyCalls = 0;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10ExperienceWaitGameModeFixture : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADCR10ExperienceWaitGameModeFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10WeaponSpawnerFixture : public ADCLyraWeaponSpawner
{
	GENERATED_BODY()
public:
	void StartFixtureCooldown();
	bool IsAvailable() const { return bIsWeaponAvailable; }
};
