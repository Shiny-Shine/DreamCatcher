// Copyright Epic Games, Inc. All Rights Reserved.
#include "DCLyraAIIntegrationController.h"

#include "DCLogChannels.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraAIIntegrationController)

void ADCLyraAIIntegrationController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void ADCLyraAIIntegrationController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void ADCLyraAIIntegrationController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IDCTeamAgentInterface* PlayerStateTeamInterface = Cast<IDCTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IDCTeamAgentInterface* PlayerStateTeamInterface = Cast<IDCTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void ADCLyraAIIntegrationController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ADCLyraAIIntegrationController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ADCLyraAIIntegrationController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
	// Only the original team hook is added here. The existing integration path
	// remains responsible for GAS/input readiness; this is not a full PC port.
}

void ADCLyraAIIntegrationController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogDCTeams, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId ADCLyraAIIntegrationController::GetGenericTeamId() const
{
	if (const IDCTeamAgentInterface* PSWithTeamInterface = Cast<IDCTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnDCTeamIndexChangedDelegate* ADCLyraAIIntegrationController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}
