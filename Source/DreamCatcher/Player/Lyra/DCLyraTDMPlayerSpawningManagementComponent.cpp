// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraTDMPlayerSpawningManagementComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "Player/Lyra/DCLyraPlayerStart.h"
#include "Teams/DCTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraTDMPlayerSpawningManagementComponent)

class AActor;

UDCLyraTDMPlayerSpawningManagementComponent::UDCLyraTDMPlayerSpawningManagementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AActor* UDCLyraTDMPlayerSpawningManagementComponent::OnChoosePlayerStart(AController* Player, TArray<ADCLyraPlayerStart*>& PlayerStarts)
{
	UDCTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UDCTeamSubsystem>();
	if (!ensure(TeamSubsystem))
	{
		return nullptr;
	}

	const int32 PlayerTeamId = TeamSubsystem->FindTeamFromObject(Player);

	// We should have a TeamId by now, but early login stuff before post login can try to do stuff, ignore it.
	if (!ensure(PlayerTeamId != INDEX_NONE))
	{
		return nullptr;
	}

	ADCLyraGameState* GameState = GetGameStateChecked<ADCLyraGameState>();

	ADCLyraPlayerStart* BestPlayerStart = nullptr;
	double MaxDistance = 0;
	ADCLyraPlayerStart* FallbackPlayerStart = nullptr;
	double FallbackMaxDistance = 0;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		const int32 TeamId = TeamSubsystem->FindTeamFromObject(PS);
		
		// We should have a TeamId by now...
		if (PS->IsOnlyASpectator() || !ensure(TeamId != INDEX_NONE))
		{
			continue;
		}

		// If the other player isn't on the same team, lets find the furthest spawn from them.
		if (TeamId != PlayerTeamId)
		{
			for (ADCLyraPlayerStart* PlayerStart : PlayerStarts)
			{
				if (APawn* Pawn = PS->GetPawn())
				{
					const double Distance = PlayerStart->GetDistanceTo(Pawn);

					if (PlayerStart->IsClaimed())
					{
						if (FallbackPlayerStart == nullptr || Distance > FallbackMaxDistance)
						{
							FallbackPlayerStart = PlayerStart;
							FallbackMaxDistance = Distance;
						}
					}
					else if (PlayerStart->GetLocationOccupancy(Player) < EDCLyraPlayerStartLocationOccupancy::Full)
					{
						if (BestPlayerStart == nullptr || Distance > MaxDistance)
						{
							BestPlayerStart = PlayerStart;
							MaxDistance = Distance;
						}
					}
				}
			}
		}
	}

	if (BestPlayerStart)
	{
		return BestPlayerStart;
	}

	return FallbackPlayerStart;
}

void UDCLyraTDMPlayerSpawningManagementComponent::OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation)
{
	
}
