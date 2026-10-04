// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DreamCatcherGameMode.generated.h"

class ADreamCatcherCharacter;
class APawn;
class APlayerController;
class UDCLyraHealthComponent;
class UDCPawnData;

UCLASS()
class DREAMCATCHER_API ADreamCatcherGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADreamCatcherGameMode();

	// Ported from LyraGameMode; Experience selection is supplied by the AssetManager default until R13.
	const UDCPawnData* GetPawnDataForController(const AController* InController) const;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Delay AFTER the death ability finishes, not after health first reaches zero.
	UPROPERTY(EditAnywhere, Category="Flow", meta=(ClampMin="0.0", Units="s"))
	float RestartLevelDelay = 2.0f;

private:
	bool InitializePlayerPawnData(AController* InController);
	void BindPlayerDeath();
	void UnbindPlayerDeath();
	void RestartCurrentLevel();

	UFUNCTION()
	void HandlePlayerPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor);

	TWeakObjectPtr<APlayerController> BoundPlayerController;
	TWeakObjectPtr<UDCLyraHealthComponent> BoundHealthComponent;

	FTimerHandle BindPlayerTimerHandle;
	FTimerHandle RestartLevelTimerHandle;
	bool bRestartQueued = false;
};
