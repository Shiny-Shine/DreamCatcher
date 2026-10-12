// Copyright Epic Games, Inc. All Rights Reserved.
#include "DCLyraExperiencePlayerState.h"

#include "Character/DCPawnData.h"
#include "DCLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraExperiencePlayerState)

ADCLyraExperiencePlayerState::ADCLyraExperiencePlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ADCLyraExperiencePlayerState::PostInitializeComponents()
{
	// Base already constructs/registers ASC and attributes. Never create a second ASC.
	Super::PostInitializeComponents();

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		AGameStateBase* GameState = World->GetGameState();
		check(GameState);
		UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnDCLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void ADCLyraExperiencePlayerState::OnExperienceLoaded(const UDCLyraExperienceDefinition* /*CurrentExperience*/)
{
	if (ADCLyraGameMode* GameMode = GetWorld()->GetAuthGameMode<ADCLyraGameMode>())
	{
		if (const UDCPawnData* NewPawnData = GameMode->GetPawnDataForController(GetOwningController()))
		{
			// The original SetPawnData is immutable. Preserve an already assigned per-controller loadout.
			if (!GetPawnData<UDCPawnData>())
			{
				SetPawnData(NewPawnData);
			}
		}
		else
		{
			UE_LOG(LogDC, Error, TEXT("Experience ready, but no PawnData for player state [%s]."), *GetNameSafe(this));
		}
	}
}
