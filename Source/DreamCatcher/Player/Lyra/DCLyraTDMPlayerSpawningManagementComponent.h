// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Player/Lyra/DCLyraPlayerSpawningManagerComponent.h"

#include "DCLyraTDMPlayerSpawningManagementComponent.generated.h"

class AActor;
class AController;
class ADCLyraPlayerStart;
class UObject;

/**
 * 
 */
UCLASS()
class DREAMCATCHER_API UDCLyraTDMPlayerSpawningManagementComponent : public UDCLyraPlayerSpawningManagerComponent
{
	GENERATED_BODY()

public:

	UDCLyraTDMPlayerSpawningManagementComponent(const FObjectInitializer& ObjectInitializer);

	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<ADCLyraPlayerStart*>& PlayerStarts) override;
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) override;

protected:

};
