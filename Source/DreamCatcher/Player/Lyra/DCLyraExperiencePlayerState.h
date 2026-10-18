// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Player/DCPlayerState.h"
#include "DCLyraExperiencePlayerState.generated.h"

class UDCLyraExperienceDefinition;

/** Restores Lyra's Experience-ready PawnData grant only for the new opt-in GameMode.
 * The existing ADCPlayerState/legacy GameMode initialization is not changed.
 */
UCLASS()
class DREAMCATCHER_API ADCLyraExperiencePlayerState : public ADCPlayerState
{
	GENERATED_BODY()
public:
	ADCLyraExperiencePlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
protected:
	virtual void PostInitializeComponents() override;
private:
	void OnExperienceLoaded(const UDCLyraExperienceDefinition* CurrentExperience);
	bool InitializeRoleBaseDamage(const UDCLyraExperienceDefinition* CurrentExperience);

	// The PlayerState and its ASC survive Pawn replacement. Do not reset combat progression on respawn.
	bool bRoleBaseDamageInitialized = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBaseDamageTestAccess;
#endif
};
