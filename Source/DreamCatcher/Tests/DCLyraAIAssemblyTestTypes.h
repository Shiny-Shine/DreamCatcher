// Unsaved native fixtures for the approved R10 team/spawn integration.
#pragma once

#include "CoreMinimal.h"
#include "Player/DCLyraAIIntegrationController.h"
#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Teams/DCTeamCreationComponent.h"
#include "DCLyraAIAssemblyTestTypes.generated.h"

class ADCPlayerState;

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10AssemblyGameModeFixture : public ADCLyraGameMode
{
	GENERATED_BODY()
public:
	ADCR10AssemblyGameModeFixture();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10TeamCreationFixture : public UDCTeamCreationComponent
{
	GENERATED_BODY()
public:
	void ConfigureTwoTeams();
#if WITH_SERVER_CODE
	void CreateTeamsForTest();
	void AssignExistingForTest();
	void ChooseTeamForTest(ADCPlayerState* State);
#endif
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10TeamPlayerControllerFixture : public ADCLyraAIIntegrationController
{
	GENERATED_BODY()
public:
	void AssignFixturePlayerState(APlayerState* NewState);
	void CleanupFixturePlayerState();
protected:
	// Team tests have no LocalPlayer, input, HUD or starting weapon assets.
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
};
