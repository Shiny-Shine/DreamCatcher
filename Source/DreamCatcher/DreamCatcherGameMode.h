// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DreamCatcherGameMode.generated.h"

class ADreamCatcherCharacter;
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

	// 플레이어가 죽었을 때 현재 레벨을 재시작하기 전까지의 지연.
	UPROPERTY(EditAnywhere, Category="Flow", meta=(ClampMin="0.0", Units="s"))
	float RestartLevelDelay = 2.0f;

private:
	bool InitializePlayerPawnData(AController* InController);
	void BindPlayerDeath();
	void RestartCurrentLevel();

	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor);

	FTimerHandle BindPlayerTimerHandle;
	FTimerHandle RestartLevelTimerHandle;
	bool bRestartQueued = false;
	bool bPlayerDeathBound = false;
};
