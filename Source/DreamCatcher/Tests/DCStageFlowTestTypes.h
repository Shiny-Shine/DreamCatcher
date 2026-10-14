// Native unsaved tests. They do not replace the Level 1 copy's actual PIE checks.
#pragma once

#include "GameModes/DCStageRunGameMode.h"
#include "Stage/DCEncounterController.h"
#include "Stage/DCStageDirector.h"
#include "DCStageFlowTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR11DirectorFixture : public ADCStageDirector
{
	GENERATED_BODY()
public:
	ADCR11DirectorFixture();
	void Configure(const TArray<FDCStagePhaseDefinition>& InPhases);
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR11EncounterFixture : public ADCEncounterController
{
	GENERATED_BODY()
public:
	void Configure(float Delay, ADCEnemySpawner* Spawner = nullptr);
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR11ModeFixture : public ADCStageRunGameMode
{
	GENERATED_BODY()
public:
	ADCR11ModeFixture();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	int32 TravelCalls = 0;
	FName LastTravel;
protected:
	virtual void PerformLevelTravel(FName LevelName) override;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR11Observer : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void PhaseStarted(ADCStageDirector* Director, int32 Index, const FText& Title, EDCStagePhaseType Type);
	UFUNCTION()
	void StageEnded(ADCStageDirector* Director, bool bSucceeded);
	TArray<int32> Indices;
	int32 EndCalls = 0;
	bool bWon = false;
};
