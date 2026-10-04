// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/DCGameInstance.h"

#include "AbilitySystem/DCGameplayTags.h"
#include "CommonUserSubsystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "DCLogChannels.h"
#include "GameUIManagerSubsystem.h"
#include "Player/DCLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCGameInstance)

UDCGameInstance::UDCGameInstance(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCGameInstance::Init()
{
	Super::Init();

	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (!ensure(ComponentManager))
	{
		return;
	}

	// Ported from ULyraGameInstance::Init.
	ComponentManager->RegisterInitState(DCGameplayTags::InitState_Spawned, false, FGameplayTag());

	ComponentManager->RegisterInitState(DCGameplayTags::InitState_DataAvailable, false,
	                                    DCGameplayTags::InitState_Spawned);

	ComponentManager->RegisterInitState(DCGameplayTags::InitState_DataInitialized, false,
	                                    DCGameplayTags::InitState_DataAvailable);

	ComponentManager->RegisterInitState(DCGameplayTags::InitState_GameplayReady, false,
	                                    DCGameplayTags::InitState_DataInitialized);

	// Verification for this migration step.
	const bool bOrderValid =
		ComponentManager->IsInitStateAfterOrEqual(DCGameplayTags::InitState_DataAvailable,
		                                          DCGameplayTags::InitState_Spawned)
		&& ComponentManager->IsInitStateAfterOrEqual(DCGameplayTags::InitState_DataInitialized,
		                                             DCGameplayTags::InitState_DataAvailable)
		&& ComponentManager->IsInitStateAfterOrEqual(DCGameplayTags::InitState_GameplayReady,
		                                             DCGameplayTags::InitState_DataInitialized);

	ensureMsgf(bOrderValid, TEXT("DreamCatcher InitState order is invalid."));

	const bool bDedicatedServer = IsDedicatedServerInstance();

	UGameUIManagerSubsystem* UIManager = GetSubsystem<UGameUIManagerSubsystem>(this);

	if (!bDedicatedServer)
	{
		ensureMsgf(UIManager != nullptr, TEXT("DreamCatcher requires a concrete UI manager subsystem."));
	}

	UE_LOG(LogDC, Log, TEXT("[R2-2] InitStateOrder=%s, UIManager=%s"),
	       bOrderValid ? TEXT("OK") : TEXT("FAILED"),
	       bDedicatedServer? TEXT("NotRequired"): (UIManager ? TEXT("OK") : TEXT("MISSING")));
}

void UDCGameInstance::HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	Super::HandlerUserInitialized(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);

	// If login succeeded, tell the local player to load their settings
	if (bSuccess && ensure(UserInfo))
	{
		UDCLocalPlayer* LocalPlayer = Cast<UDCLocalPlayer>(GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex));

		// There will not be a local player attached to the dedicated server user
		if (LocalPlayer)
		{
			LocalPlayer->LoadSharedSettingsFromDisk();
		}
	}
}
