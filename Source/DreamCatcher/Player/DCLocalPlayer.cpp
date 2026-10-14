// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/DCLocalPlayer.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Settings/DCSettingsShared.h"
#include "CommonUserSubsystem.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Settings/DCLyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLocalPlayer)

class UObject;

UDCLocalPlayer::UDCLocalPlayer()
{
}

void UDCLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();
	// Bind real local players, never the class default during native class loading.
	if (!HasAnyFlags(RF_ClassDefaultObject) && GEngine)
	{
		if (UDCLyraSettingsLocal* Settings = GetLocalSettings())
		{
			Settings->OnAudioOutputDeviceChanged.AddUObject(this, &ThisClass::OnAudioOutputDeviceChanged);
		}
	}
}

UDCLyraSettingsLocal* UDCLocalPlayer::GetLocalSettings() const
{
	return UDCLyraSettingsLocal::Get();
}

void UDCLocalPlayer::OnAudioOutputDeviceChanged(const FString& DeviceId)
{
	FOnCompletedDeviceSwap Callback;
	Callback.BindUFunction(this, FName("OnCompletedAudioDeviceSwap"));
	UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(GetWorld(), DeviceId, Callback);
}

void UDCLocalPlayer::OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult)
{
	if (SwapResult.Result == ESwapAudioOutputDeviceResultState::Failure)
	{
	}
}

void UDCLocalPlayer::SwitchController(class APlayerController* PC)
{
	Super::SwitchController(PC);

	OnPlayerControllerChanged(PlayerController);
}

bool UDCLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	const bool bResult = Super::SpawnPlayActor(URL, OutError, InWorld);

	OnPlayerControllerChanged(PlayerController);

	return bResult;
}

void UDCLocalPlayer::InitOnlineSession()
{
	OnPlayerControllerChanged(PlayerController);

	Super::InitOnlineSession();
}

void UDCLocalPlayer::OnPlayerControllerChanged(APlayerController* NewController)
{
	// Stop listening for changes from the old controller
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (IDCTeamAgentInterface* ControllerAsTeamProvider = Cast<IDCTeamAgentInterface>(LastBoundPC.Get()))
	{
		OldTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	// Grab the current team ID and listen for future changes
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (IDCTeamAgentInterface* ControllerAsTeamProvider = Cast<IDCTeamAgentInterface>(NewController))
	{
		NewTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
		LastBoundPC = NewController;
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
}

void UDCLocalPlayer::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// Do nothing, we merely observe the team of our associated player controller
}

FGenericTeamId UDCLocalPlayer::GetGenericTeamId() const
{
	if (IDCTeamAgentInterface* ControllerAsTeamProvider = Cast<IDCTeamAgentInterface>(PlayerController))
	{
		return ControllerAsTeamProvider->GetGenericTeamId();
	}
	else
	{
		return FGenericTeamId::NoTeam;
	}
}

FOnDCTeamIndexChangedDelegate* UDCLocalPlayer::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

UDCSettingsShared* UDCLocalPlayer::GetSharedSettings() const
{
	if (!SharedSettings)
	{
		// On PC it's okay to use the sync load because it only checks the disk
		// This could use a platform tag to check for proper save support instead
		bool bCanLoadBeforeLogin = PLATFORM_DESKTOP;
		
		if (bCanLoadBeforeLogin)
		{
			SharedSettings = UDCSettingsShared::LoadOrCreateSettings(this);
		}
		else
		{
			// We need to wait for user login to get the real settings so return temp ones
			SharedSettings = UDCSettingsShared::CreateTemporarySettings(this);
		}
	}

	return SharedSettings;
}

void UDCLocalPlayer::LoadSharedSettingsFromDisk(bool bForceLoad)
{
	FUniqueNetIdRepl CurrentNetId = GetCachedUniqueNetId();
	if (!bForceLoad && SharedSettings && CurrentNetId == NetIdForSharedSettings)
	{
		// Already loaded once, don't reload
		return;
	}

	ensure(UDCSettingsShared::AsyncLoadOrCreateSettings(this, UDCSettingsShared::FOnSettingsLoadedEvent::CreateUObject(this, &UDCLocalPlayer::OnSharedSettingsLoaded)));
}

void UDCLocalPlayer::OnSharedSettingsLoaded(UDCSettingsShared* LoadedOrCreatedSettings)
{
	// The settings are applied before it gets here
	if (ensure(LoadedOrCreatedSettings))
	{
		// This will replace the temporary or previously loaded object which will GC out normally
		SharedSettings = LoadedOrCreatedSettings;

		NetIdForSharedSettings = GetCachedUniqueNetId();
	}
}

void UDCLocalPlayer::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

