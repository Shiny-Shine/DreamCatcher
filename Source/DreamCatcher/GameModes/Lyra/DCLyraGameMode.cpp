// Copyright Epic Games, Inc. All Rights Reserved.
// Lyra's AI/gameplay GameMode functions. Online hosting/replay are explicitly deferred.
#include "DCLyraGameMode.h"

#include "AssetRegistry/AssetData.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "DCLogChannels.h"
#include "Development/DCLyraDeveloperSettings.h"
#include "DreamCatcherCharacter.h"
#include "DreamCatcherPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "GameModes/Lyra/DCLyraWorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Player/Lyra/DCLyraExperiencePlayerState.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Player/Lyra/DCLyraPlayerSpawningManagerComponent.h"
#include "System/DCAssetManager.h"
#include "TimerManager.h"
#include "UI/Lyra/DCLyraHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraGameMode)

ADCLyraGameMode::ADCLyraGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ADCLyraGameState::StaticClass();
	PlayerControllerClass = ADreamCatcherPlayerController::StaticClass();
	PlayerStateClass = ADCLyraExperiencePlayerState::StaticClass();
	DefaultPawnClass = ADreamCatcherCharacter::StaticClass();
	HUDClass = ADCLyraHUD::StaticClass();
	// No global GameMode, replay spectator, login or online GameSession replacement.
}

const UDCPawnData* ADCLyraGameMode::GetPawnDataForController(const AController* InController) const
{
	return SelectPawnDataForController(InController,
		[this]() -> const UDCLyraExperienceDefinition*
		{
			check(GameState);
			UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
			check(ExperienceComponent);
			return ExperienceComponent->IsExperienceLoaded() ? ExperienceComponent->GetCurrentExperienceChecked() : nullptr;
		},
		[]() -> const UDCPawnData* { return UDCAssetManager::Get().GetDefaultPawnData(); });
}

const UDCPawnData* ADCLyraGameMode::SelectPawnDataForController(const AController* InController,
	TFunctionRef<const UDCLyraExperienceDefinition*()> GetLoadedExperience,
	TFunctionRef<const UDCPawnData*()> GetFallbackPawnData)
{
	// See if pawn data is already set on the player state
	if (InController != nullptr)
	{
		if (const ADCPlayerState* LyraPS = InController->GetPlayerState<ADCPlayerState>())
		{
			if (const UDCPawnData* PawnData = LyraPS->GetPawnData<UDCPawnData>())
			{
				return PawnData;
			}
		}
	}

	if (const UDCLyraExperienceDefinition* Experience = GetLoadedExperience())
	{
		if (InController && InController->IsA<ADCLyraPlayerBotController>() && Experience->BotPawnData)
		{
			return Experience->BotPawnData;
		}
		if (Experience->DefaultPawnData != nullptr)
		{
			return Experience->DefaultPawnData;
		}

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		return GetFallbackPawnData();
	}

	// Experience not loaded yet, so there is no pawn data to be had
	return nullptr;
}

void ADCLyraGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Wait for the next frame to give time to initialize startup settings
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void ADCLyraGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;

	// Precedence order (highest wins)
	//  - Matchmaking assignment is deferred (no automatic online login in this mode)
	//  - URL Options override
	//  - Developer Settings (PIE only)
	//  - Command Line override
	//  - World Settings
	//  - Explicit GameMode default experience (dedicated-server hosting is deferred)

	UWorld* World = GetWorld();

	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UDCLyraExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UDCLyraDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}

	// see if the command line wants to set the experience
	if (!ExperienceId.IsValid())
	{
		FString ExperienceFromCommandLine;
		if (FParse::Value(FCommandLine::Get(), TEXT("Experience="), ExperienceFromCommandLine))
		{
			ExperienceId = FPrimaryAssetId::ParseTypeAndName(ExperienceFromCommandLine);
			if (!ExperienceId.PrimaryAssetType.IsValid())
			{
				ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UDCLyraExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromCommandLine));
			}
			ExperienceIdSource = TEXT("CommandLine");
		}
	}

	// see if the world settings has a default experience
	if (!ExperienceId.IsValid())
	{
		if (ADCLyraWorldSettings* TypedWorldSettings = Cast<ADCLyraWorldSettings>(GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	UDCAssetManager& AssetManager = UDCAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogDCExperience, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"), *ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}

	// Final fallback to the default experience
	if (!ExperienceId.IsValid())
	{
		// R10 scope: online hosting is deferred. Do not silently select a foreign project asset.
		// Configure a real scanned Experience on the opt-in GameMode or its WorldSettings.
		ExperienceId = DefaultExperience;
		ExperienceIdSource = TEXT("GameModeDefault");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

void ADCLyraGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogDCExperience, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogDCExperience, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void ADCLyraGameMode::OnExperienceLoaded(const UDCLyraExperienceDefinition* CurrentExperience)
{
	// Spawn any players that are already attached
	//@TODO: Here we're handling only *player* controllers, but in GetDefaultPawnClassForController_Implementation we skipped all controllers
	// GetDefaultPawnClassForController_Implementation might only be getting called for players anyways
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

bool ADCLyraGameMode::IsExperienceLoaded() const
{
	check(GameState);
	UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

UClass* ADCLyraGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UDCPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ADCLyraGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// Never save the default player pawns into a map.
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const UDCPawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
				else
				{
					UE_LOG(LogDC, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."), *GetNameSafe(SpawnedPawn));
				}
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
		else
		{
			UE_LOG(LogDC, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		}
	}
	else
	{
		UE_LOG(LogDC, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
	}

	return nullptr;
}

bool ADCLyraGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn management component.
	return false;
}

void ADCLyraGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Delay starting new players until the experience has been loaded
	// (players who log in prior to that will be started by OnExperienceLoaded)
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

AActor* ADCLyraGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UDCLyraPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UDCLyraPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ADCLyraGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UDCLyraPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UDCLyraPlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool ADCLyraGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

bool ADCLyraGameMode::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			return false;
		}
	}

	if (UDCLyraPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UDCLyraPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ControllerCanRestart(Controller);
	}

	return true;
}

void ADCLyraGameMode::InitGameState()
{
	Super::InitGameState();

	// Listen for the experience load to complete
	UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnDCLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void ADCLyraGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void ADCLyraGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	else if (ADCLyraPlayerBotController* BotController = Cast<ADCLyraPlayerBotController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &ADCLyraPlayerBotController::ServerRestartController);
	}
}

bool ADCLyraGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	// Do nothing, we'll wait until PostLogin when we try to spawn the player for real.
	// Doing anything right now is no good, systems like team assignment haven't even occurred yet.
	return true;
}

void ADCLyraGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// If we tried to spawn a pawn and it failed, lets try again *note* check if there's actually a pawn class
	// before we try this forever.
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			// If it's a player don't loop forever, maybe something changed and they can no longer restart if so stop trying.
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);
			}
			else
			{
				UE_LOG(LogDC, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogDC, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
	}
}
