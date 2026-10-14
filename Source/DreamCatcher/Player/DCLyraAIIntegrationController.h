// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Player/DCLoadoutController.h"
#include "Teams/DCTeamAgentInterface.h"
#include "DCLyraAIIntegrationController.generated.h"

class APlayerState;

/** Production player controller with Lyra's PlayerState-backed team interface. */
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
