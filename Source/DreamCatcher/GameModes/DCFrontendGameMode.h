#pragma once

#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Engine/EngineBaseTypes.h"
#include "DCFrontendGameMode.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FDCFrontendTravelFailed, const FText&);

/** UI-only, standalone entry point. Does not use the stage's PawnData/spawn flow. */
UCLASS()
class DREAMCATCHER_API ADCFrontendGameMode : public ADCLyraGameMode
{
	GENERATED_BODY()
public:
	ADCFrontendGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void StartPlay() override;
	virtual bool ControllerCanRestart(AController* Controller) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* Controller) override;

	UFUNCTION(BlueprintCallable, Category="Frontend")
	bool RequestStartStory();
	UFUNCTION(BlueprintPure, Category="Frontend")
	bool IsStoryTravelPending() const { return bTravelPending; }
	FDCFrontendTravelFailed OnStoryTravelFailed;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PerformStoryTravel(FName Destination);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	UPROPERTY(EditDefaultsOnly, Category="Frontend")
	TSoftObjectPtr<UWorld> StoryMap;

private:
	void ExecuteStoryTravel();
	FTimerHandle TravelTimer;
	FDelegateHandle TravelFailureHandle;
	FName PendingDestination;
	bool bTravelPending = false;
	bool bEndingPlay = false;
};
