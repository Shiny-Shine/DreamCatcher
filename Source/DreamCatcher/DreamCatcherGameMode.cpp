// Copyright Epic Games, Inc. All Rights Reserved.

#include "DreamCatcherGameMode.h"
#include "DreamCatcherCharacter.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Components/DCHealthComponent.h"
#include "DCLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DCPlayerState.h"
#include "System/DCAssetManager.h"
#include "TimerManager.h"

ADreamCatcherGameMode::ADreamCatcherGameMode()
{
}

const UDCPawnData* ADreamCatcherGameMode::GetPawnDataForController(const AController* InController) const
{
	const ADCPlayerState* DCPlayerState = InController ? InController->GetPlayerState<ADCPlayerState>() : nullptr;
	if (!DCPlayerState)
	{
		// Do not opt the old, non-GAS game modes into the new pawn configuration.
		return nullptr;
	}

	if (const UDCPawnData* PawnData = DCPlayerState->GetPawnData<UDCPawnData>())
	{
		return PawnData;
	}

	// Migration-only difference: Lyra reaches this fallback after its Experience is loaded.
	// Experience selection is not present yet; reuse its existing AssetManager fallback directly.
	return UDCAssetManager::Get().GetDefaultPawnData();
}

bool ADreamCatcherGameMode::InitializePlayerPawnData(AController* InController)
{
	ADCPlayerState* DCPlayerState = InController ? InController->GetPlayerState<ADCPlayerState>() : nullptr;
	if (!DCPlayerState || DCPlayerState->GetPawnData<UDCPawnData>())
	{
		return true;
	}

	const UDCPawnData* PawnData = GetPawnDataForController(InController);
	if (!PawnData)
	{
		UE_LOG(LogDC, Error, TEXT("[R2-2] No PawnData for [%s]. Configure DCAssetManager.DefaultPawnData."), *GetNameSafe(InController));
		return false;
	}

	// Replaces only LyraPlayerState::OnExperienceLoaded's trigger for now, not SetPawnData's grant logic.
	DCPlayerState->SetPawnData(PawnData);
	return DCPlayerState->GetPawnData<UDCPawnData>() == PawnData;
}

void ADreamCatcherGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (InitializePlayerPawnData(NewPlayer))
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

UClass* ADreamCatcherGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// Ported from ALyraGameMode::GetDefaultPawnClassForController_Implementation.
	if (const UDCPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ADreamCatcherGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	if (!NewPlayer || !NewPlayer->GetPlayerState<ADCPlayerState>())
	{
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
	}

	// Also covers a direct RestartPlayer path without granting again after respawn.
	if (!InitializePlayerPawnData(NewPlayer))
	{
		return nullptr;
	}

	const UDCPawnData* PawnData = GetPawnDataForController(NewPlayer);
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!PawnData || !PawnClass)
	{
		UE_LOG(LogDC, Error, TEXT("[R2-2] Cannot spawn pawn for [%s]: missing PawnData or PawnClass."), *GetNameSafe(NewPlayer));
		return nullptr;
	}

	// Ported from ALyraGameMode: provide PawnData before construction/BeginPlay finishes.
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true;

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	if (!SpawnedPawn)
	{
		UE_LOG(LogDC, Error, TEXT("[R2-2] Failed to spawn pawn class [%s]."), *GetNameSafe(PawnClass));
		return nullptr;
	}

	UDCPawnExtensionComponent* PawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn);
	if (!PawnExtension)
	{
		UE_LOG(LogDC, Error, TEXT("[R2-2] GAS pawn [%s] requires DCPawnExtensionComponent."), *GetNameSafe(SpawnedPawn));
		SpawnedPawn->Destroy();
		return nullptr;
	}

	// Existing test BPs may still serialize PawnData in Class Defaults. Accept the same data,
	// but never silently overwrite a different configuration or invoke the one-shot setter twice.
	const UDCPawnData* ExistingPawnData = PawnExtension->GetPawnData<UDCPawnData>();
	if (ExistingPawnData && ExistingPawnData != PawnData)
	{
		UE_LOG(LogDC, Error, TEXT("[R2-2] Pawn [%s] defaults to [%s], but PlayerState uses [%s]. Clear or align the BP PawnData default."),
			*GetNameSafe(SpawnedPawn), *GetNameSafe(ExistingPawnData), *GetNameSafe(PawnData));
		SpawnedPawn->Destroy();
		return nullptr;
	}
	if (!ExistingPawnData)
	{
		PawnExtension->SetPawnData(PawnData);
	}

	SpawnedPawn->FinishSpawning(SpawnTransform);
	return SpawnedPawn;
}

void ADreamCatcherGameMode::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay 시점에 플레이어가 아직 준비되지 않았을 수 있으므로 별도 바인딩 함수를 둠.
	BindPlayerDeath();
}

void ADreamCatcherGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BindPlayerTimerHandle);
	GetWorldTimerManager().ClearTimer(RestartLevelTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ADreamCatcherGameMode::BindPlayerDeath()
{
	if (bPlayerDeathBound)
	{
		return;
	}

	ADreamCatcherCharacter* PlayerCharacter = Cast<ADreamCatcherCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter || !PlayerCharacter->GetHealthComponent())
	{
		GetWorldTimerManager().SetTimer(BindPlayerTimerHandle, this, &ADreamCatcherGameMode::BindPlayerDeath, 0.1f, false);
		return;
	}

	PlayerCharacter->GetHealthComponent()->OnDeath.RemoveDynamic(this, &ADreamCatcherGameMode::HandlePlayerDeath);
	PlayerCharacter->GetHealthComponent()->OnDeath.AddDynamic(this, &ADreamCatcherGameMode::HandlePlayerDeath);
	bPlayerDeathBound = true;
}

void ADreamCatcherGameMode::HandlePlayerDeath(AActor* DeadActor)
{
	if (bRestartQueued)
	{
		return;
	}

	bRestartQueued = true;
	GetWorldTimerManager().SetTimer(RestartLevelTimerHandle, this, &ADreamCatcherGameMode::RestartCurrentLevel, RestartLevelDelay, false);
}

void ADreamCatcherGameMode::RestartCurrentLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}
