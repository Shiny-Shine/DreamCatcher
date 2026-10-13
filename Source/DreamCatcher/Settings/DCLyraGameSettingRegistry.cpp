// Copyright Epic Games, Inc. All Rights Reserved.
#include "Settings/DCLyraGameSettingRegistry.h"

#include "GameSettingCollection.h"
#include "Settings/DCLyraSettingsLocal.h"
#include "Settings/DCSettingsShared.h"
#include "Player/DCLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraGameSettingRegistry)

DEFINE_LOG_CATEGORY(LogDCLyraGameSettingRegistry);

#define LOCTEXT_NAMESPACE "Lyra"

//--------------------------------------
// ULyraGameSettingRegistry
//--------------------------------------

UDCLyraGameSettingRegistry::UDCLyraGameSettingRegistry()
{
}

UDCLyraGameSettingRegistry* UDCLyraGameSettingRegistry::Get(UDCLocalPlayer* InLocalPlayer)
{
	UDCLyraGameSettingRegistry* Registry = FindObject<UDCLyraGameSettingRegistry>(InLocalPlayer, TEXT("LyraGameSettingRegistry"), EFindObjectFlags::ExactClass);
	if (Registry == nullptr)
	{
		Registry = NewObject<UDCLyraGameSettingRegistry>(InLocalPlayer, TEXT("LyraGameSettingRegistry"));
		Registry->Initialize(InLocalPlayer);
	}

	return Registry;
}

bool UDCLyraGameSettingRegistry::IsFinishedInitializing() const
{
	if (Super::IsFinishedInitializing())
	{
		if (UDCLocalPlayer* LocalPlayer = Cast<UDCLocalPlayer>(OwningLocalPlayer))
		{
			if (LocalPlayer->GetSharedSettings() == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void UDCLyraGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	UDCLocalPlayer* DCLocalPlayer = Cast<UDCLocalPlayer>(InLocalPlayer);

	VideoSettings = InitializeVideoSettings(DCLocalPlayer);
	InitializeVideoSettings_FrameRates(VideoSettings, DCLocalPlayer);
	RegisterSetting(VideoSettings);

	AudioSettings = InitializeAudioSettings(DCLocalPlayer);
	RegisterSetting(AudioSettings);

	GameplaySettings = InitializeGameplaySettings(DCLocalPlayer);
	RegisterSetting(GameplaySettings);

	MouseAndKeyboardSettings = InitializeMouseAndKeyboardSettings(DCLocalPlayer);
	RegisterSetting(MouseAndKeyboardSettings);

	GamepadSettings = InitializeGamepadSettings(DCLocalPlayer);
	RegisterSetting(GamepadSettings);
}

void UDCLyraGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	
	if (UDCLocalPlayer* LocalPlayer = Cast<UDCLocalPlayer>(OwningLocalPlayer))
	{
		// Game user settings need to be applied to handle things like resolution, this saves indirectly
		LocalPlayer->GetLocalSettings()->ApplySettings(false);
		
		LocalPlayer->GetSharedSettings()->ApplySettings();
		LocalPlayer->GetSharedSettings()->SaveSettings();
	}
}

#undef LOCTEXT_NAMESPACE

