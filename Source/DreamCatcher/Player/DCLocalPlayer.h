// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"
#include "Teams/DCTeamAgentInterface.h"

#include "DCLocalPlayer.generated.h"

#define UE_API DREAMCATCHER_API

struct FGenericTeamId;

class APlayerController;
class UInputMappingContext;
class UDCSettingsShared;
class UObject;
class UWorld;
struct FFrame;

/**
 * UDCLocalPlayer
 *
 * R4-2: Shared settings and team observation are ported from Lyra.
 * LocalSettings/audio-output-device hooks are deferred until their settings
 * and audio dependencies are ported; the UCommonLocalPlayer base is preserved.
 */
UCLASS(MinimalAPI)
class UDCLocalPlayer : public UCommonLocalPlayer, public IDCTeamAgentInterface
{
	GENERATED_BODY()

public:

	UE_API UDCLocalPlayer();

	//~UPlayer interface
	UE_API virtual void SwitchController(class APlayerController* PC) override;
	//~End of UPlayer interface

	//~ULocalPlayer interface
	UE_API virtual bool SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld) override;
	UE_API virtual void InitOnlineSession() override;
	//~End of ULocalPlayer interface

	//~IDCTeamAgentInterface interface
	UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	UE_API virtual FGenericTeamId GetGenericTeamId() const override;
	UE_API virtual FOnDCTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IDCTeamAgentInterface interface

	/** Gets the shared setting for this player, this is read using the save game system so may not be correct until after user login */
	UFUNCTION()
	UE_API UDCSettingsShared* GetSharedSettings() const;

	/** Starts an async request to load the shared settings, this will call OnSharedSettingsLoaded after loading or creating new ones */
	UE_API void LoadSharedSettingsFromDisk(bool bForceLoad = false);

protected:
	UE_API void OnSharedSettingsLoaded(UDCSettingsShared* LoadedOrCreatedSettings);

	UE_API void OnPlayerControllerChanged(APlayerController* NewController);

	UFUNCTION()
	UE_API void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

private:
	UPROPERTY(Transient)
	mutable TObjectPtr<UDCSettingsShared> SharedSettings;

	FUniqueNetIdRepl NetIdForSharedSettings;

	UPROPERTY(Transient)
	mutable TObjectPtr<const UInputMappingContext> InputMappingContext;

	UPROPERTY()
	FOnDCTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TWeakObjectPtr<APlayerController> LastBoundPC;
};

#undef UE_API
