#pragma once

#include "GameModes/DCFrontendGameMode.h"
#include "DCFrontendTestTypes.generated.h"

/** Unsaved world fixture. Never activates an Experience or opens a map. */
UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCFrontendModeFixture : public ADCFrontendGameMode
{
	GENERATED_BODY()
public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	void SetStoryDestination(const FString& Path);
	void SimulateTravelFailure();
	int32 TravelCalls = 0;
	FName LastDestination;
protected:
	virtual void PerformStoryTravel(FName Destination) override;
};
