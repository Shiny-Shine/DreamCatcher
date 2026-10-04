// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"

#include "DCShooterCoreRuntimeSettings.generated.h"

class UObject;

/** Runtime settings specific to the ShooterCoreRuntime plugin */
UCLASS(config = Game, defaultconfig)
class UDCShooterCoreRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDCShooterCoreRuntimeSettings(const FObjectInitializer& Initializer);

	ECollisionChannel GetAimAssistCollisionChannel() const { return AimAssistCollisionChannel; }

private:

	/**
	 * What trace channel should be used to find available targets for Aim Assist.
	 * @see UDCAimAssistTargetManagerComponent::GetVisibleTargets
	 */
	UPROPERTY(config, EditAnywhere, Category = "Aim Assist")
	TEnumAsByte<ECollisionChannel> AimAssistCollisionChannel = ECollisionChannel::ECC_EngineTraceChannel5;
};
