// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Development/DCRifleIntegrationActors.h"
#include "Teams/DCTeamAgentInterface.h"
#include "DCLyraAIIntegrationController.generated.h"

class APlayerState;

/** Opt-in AI test player controller.
 * Preserves the existing rifle/input/camera/UI path and adds only Lyra's
 * PlayerState-backed team interface. No existing player Blueprint is reparented here.
 */
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCLyraAIIntegrationController : public ADCRifleIntegrationController, public IDCTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnDCTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;

protected:
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPlayerStateChanged();

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	void BroadcastOnPlayerStateChanged();

	UPROPERTY()
	FOnDCTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
