// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/GameStateComponent.h"

#include "DCLyraPlayerSpawningManagerComponent.generated.h"

#define UE_API DREAMCATCHER_API

class AController;
class APlayerController;
class APlayerState;
class APlayerStart;
class ADCLyraPlayerStart;
class AActor;

/**
 * @class UDCLyraPlayerSpawningManagerComponent
 */
UCLASS(MinimalAPI)
class UDCLyraPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UE_API UDCLyraPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer);

	/** UActorComponent */
	UE_API virtual void InitializeComponent() override;
	UE_API virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~UActorComponent */

protected:
	// Utility
	UE_API APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<ADCLyraPlayerStart*>& FoundStartPoints) const;

	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<ADCLyraPlayerStart*>& PlayerStarts) { return nullptr; }
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) { }

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnFinishRestartPlayer))
	UE_API void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);

private:

	/** We proxy these calls from ADCLyraGameMode, to this component so that each experience can more easily customize the respawn system they want. */
	UE_API AActor* ChoosePlayerStart(AController* Player);
	UE_API bool ControllerCanRestart(AController* Player);
	UE_API void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class ADCLyraGameMode;
	/** ~ADCLyraGameMode */

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ADCLyraPlayerStart>> CachedPlayerStarts;

private:
	UE_API void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);
	UE_API void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	UE_API APlayerStart* FindPlayFromHereStart(AController* Player);
#endif
};

#undef UE_API
