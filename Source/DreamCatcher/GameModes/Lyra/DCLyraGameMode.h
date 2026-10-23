// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "UObject/PrimaryAssetId.h"
#include "Templates/Function.h"
#include "DCLyraGameMode.generated.h"

class UDCLyraExperienceDefinition;
class UDCPawnData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDCLyraGameModePlayerInitialized, AGameModeBase*, AController*);

/** Opt-in Lyra gameplay/AI GameMode. Existing DreamCatcher game modes are unchanged.
 * Original Experience/spawn/restart functions are ported, not a second AI implementation.
 * Online session hosting, dedicated-server login and replay defaults are deferred.
 */
UCLASS(Config=Game)
class DREAMCATCHER_API ADCLyraGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
public:
	ADCLyraGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Pawn")
	const UDCPawnData* GetPawnDataForController(const AController* InController) const;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual void InitGameState() override;
	virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	virtual void FailedToRestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintCallable)
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);
	virtual bool ControllerCanRestart(AController* Controller);
	FOnDCLyraGameModePlayerInitialized OnGameModePlayerInitialized;

protected:
	void OnExperienceLoaded(const UDCLyraExperienceDefinition* CurrentExperience);
	bool IsExperienceLoaded() const;
	void OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource);
	void HandleMatchAssignmentIfNotExpectingOne();

	/** Project-only fallback instead of Lyra's hard-coded B_LyraDefaultExperience.
	 * No default is selected globally. A new AI map must assign an actual scanned Experience.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Lyra|Experience", meta=(AllowedTypes="DCLyraExperienceDefinition"))
	FPrimaryAssetId DefaultExperience;

private:
	/** Shared selection policy; providers stay lazy so explicit PlayerState data
	 * never needs Experience/GameState access or loads the global fallback. */
	static const UDCPawnData* SelectPawnDataForController(const AController* InController,
		TFunctionRef<const UDCLyraExperienceDefinition*()> GetLoadedExperience,
		TFunctionRef<const UDCPawnData*()> GetFallbackPawnData);
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCInspectorBotPawnDataTestAccess;
#endif
};
