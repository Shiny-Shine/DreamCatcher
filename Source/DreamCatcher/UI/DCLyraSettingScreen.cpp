// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/DCLyraSettingScreen.h"

#include "Input/CommonUIInputTypes.h"
#include "Player/DCLocalPlayer.h"
#include "Settings/DCLyraGameSettingRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraSettingScreen)

class UGameSettingRegistry;

void UDCLyraSettingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackHandle = RegisterUIActionBinding(FBindUIActionArgs(BackInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction)));
	ApplyHandle = RegisterUIActionBinding(FBindUIActionArgs(ApplyInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleApplyAction)));
	CancelChangesHandle = RegisterUIActionBinding(FBindUIActionArgs(CancelChangesInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleCancelChangesAction)));
}

UGameSettingRegistry* UDCLyraSettingScreen::CreateRegistry()
{
	UDCLyraGameSettingRegistry* NewRegistry = NewObject<UDCLyraGameSettingRegistry>();

	if (UDCLocalPlayer* LocalPlayer = CastChecked<UDCLocalPlayer>(GetOwningLocalPlayer()))
	{
		NewRegistry->Initialize(LocalPlayer);
	}

	return NewRegistry;
}

void UDCLyraSettingScreen::HandleBackAction()
{
	if (AttemptToPopNavigation())
	{
		return;
	}

	ApplyChanges();

	DeactivateWidget();
}

void UDCLyraSettingScreen::HandleApplyAction()
{
	ApplyChanges();
}

void UDCLyraSettingScreen::HandleCancelChangesAction()
{
	CancelChanges();
}

void UDCLyraSettingScreen::OnSettingsDirtyStateChanged_Implementation(bool bSettingsDirty)
{
	if (bSettingsDirty)
	{
		if (!GetActionBindings().Contains(ApplyHandle))
		{
			AddActionBinding(ApplyHandle);
		}
		if (!GetActionBindings().Contains(CancelChangesHandle))
		{
			AddActionBinding(CancelChangesHandle);
		}
	}
	else
	{
		RemoveActionBinding(ApplyHandle);
		RemoveActionBinding(CancelChangesHandle);
	}
}
