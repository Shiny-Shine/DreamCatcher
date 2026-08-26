// Copyright Epic Games, Inc. All Rights Reserved.

#include "DreamCatcherPlayerController.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "DreamCatcherCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Player/DCPlayerState.h"
#include "UI/DCPlayerHUDWidget.h"

void ADreamCatcherPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 첫 플레이어블은 게임 조작만 있으면 되므로 GameOnly 입력 모드로 둠.
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	ApplyInputMappingContexts();
	CreateHUD();
	BindHUDToCurrentPawn();
}

void ADreamCatcherPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 플레이어 Pawn이 바뀌면 HUD가 새 캐릭터를 다시 바라보게 함.
	BindHUDToCurrentPawn();
}

// 현재 컨트롤러와 연결된 ADCPlayerState와 전용 ASC를 가져온 후 ASC에 입력되있는 입력을 처리
void ADreamCatcherPlayerController::PostProcessInput(float DeltaTime, bool bGamePaused)
{
	if (ADCPlayerState* DCPlayerState = GetPlayerState<ADCPlayerState>())
	{
		if (UDCAbilitySystemComponent* AbilitySystemComponent = DCPlayerState->GetDCAbilitySystemComponent())
		{
			AbilitySystemComponent->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void ADreamCatcherPlayerController::ApplyInputMappingContexts()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		for (UInputMappingContext* MappingContext : DefaultMappingContexts)
		{
			if (MappingContext)
			{
				Subsystem->AddMappingContext(MappingContext, 0);
			}
		}
	}
}

void ADreamCatcherPlayerController::CreateHUD()
{
	if (!IsLocalController() || HUDWidget || !HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UDCPlayerHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ADreamCatcherPlayerController::BindHUDToCurrentPawn()
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->BindToCharacter(Cast<ADreamCatcherCharacter>(GetPawn()));
}
