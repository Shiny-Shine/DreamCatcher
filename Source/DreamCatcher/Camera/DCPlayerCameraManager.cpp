// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "DCCameraComponent.h"
#include "DCUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

ADCPlayerCameraManager::ADCPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = DC_CAMERA_DEFAULT_FOV;
	ViewPitchMin = DC_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = DC_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UDCUICameraManagerComponent>(UICameraComponentName);
}

UDCUICameraManagerComponent* ADCPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void ADCPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void ADCPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("DCPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UDCCameraComponent* CameraComponent = UDCCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}
