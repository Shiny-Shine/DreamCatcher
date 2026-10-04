// Copyright Epic Games, Inc. All Rights Reserved.

#include "DreamCatcherPlayerController.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Camera/DCPlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DreamCatcherCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Actor.h"
#include "InputMappingContext.h"
#include "Player/DCPlayerState.h"
#include "UI/DCPlayerHUDWidget.h"
#include "AbilitySystem/DCGameplayTags.h"

ADreamCatcherPlayerController::ADreamCatcherPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ADCPlayerCameraManager::StaticClass();
}

// Camera assist callbacks ported from LyraPlayerController.
void ADreamCatcherPlayerController::OnCameraPenetratingTarget()
{
	bHideViewTargetPawnNextFrame = true;
}

void ADreamCatcherPlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, OutHiddenComponents);

	if (bHideViewTargetPawnNextFrame)
	{
		AActor* const ViewTargetPawn = PlayerCameraManager ? Cast<AActor>(PlayerCameraManager->GetViewTarget()) : nullptr;
		if (ViewTargetPawn)
		{
			// internal helper func to hide all the components
			auto AddToHiddenComponents = [&OutHiddenComponents](const TInlineComponentArray<UPrimitiveComponent*>& InComponents)
			{
				// add every component and all attached children
				for (UPrimitiveComponent* Comp : InComponents)
				{
					if (Comp->IsRegistered())
					{
						OutHiddenComponents.Add(Comp->GetPrimitiveSceneId());

						for (USceneComponent* AttachedChild : Comp->GetAttachChildren())
						{
							static FName NAME_NoParentAutoHide(TEXT("NoParentAutoHide"));
							UPrimitiveComponent* AttachChildPC = Cast<UPrimitiveComponent>(AttachedChild);
							if (AttachChildPC && AttachChildPC->IsRegistered() && !AttachChildPC->ComponentTags.Contains(NAME_NoParentAutoHide))
							{
								OutHiddenComponents.Add(AttachChildPC->GetPrimitiveSceneId());
							}
						}
					}
				}
			};

			//TODO Solve with an interface.  Gather hidden components or something.
			//TODO Hiding isn't awesome, sometimes you want the effect of a fade out over a proximity, needs to bubble up to designers.

			// hide pawn's components
			TInlineComponentArray<UPrimitiveComponent*> PawnComponents;
			ViewTargetPawn->GetComponents(PawnComponents);
			AddToHiddenComponents(PawnComponents);

			//// hide weapon too
			//if (ViewTargetPawn->CurrentWeapon)
			//{
			//	TInlineComponentArray<UPrimitiveComponent*> WeaponComponents;
			//	ViewTargetPawn->CurrentWeapon->GetComponents(WeaponComponents);
			//	AddToHiddenComponents(WeaponComponents);
			//}
		}

		// we consumed it, reset for next frame
		bHideViewTargetPawnNextFrame = false;
	}
}

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

void ADreamCatcherPlayerController::PostProcessInput(float DeltaTime,bool bGamePaused)
{
	if (UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent())
	{
		// 과도기: 기존 프로젝트의 입력 차단 시 조준 해제 정책.
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
		{
			ASC->CancelAimInputAndState();
		}

		// 원본 ASC 입력 처리.
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
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

UDCAbilitySystemComponent* ADreamCatcherPlayerController::GetDCAbilitySystemComponent() const
{
	const ADCPlayerState* DCPlayerState = GetPlayerState<ADCPlayerState>();
	return DCPlayerState
		       ? DCPlayerState->GetDCAbilitySystemComponent()
		       : nullptr;
}

void ADreamCatcherPlayerController::SetIsAutoRunning(bool bEnabled)
{
	const bool bIsAutoRunning = GetIsAutoRunning();

	if (bEnabled != bIsAutoRunning)
	{
		if (!bEnabled)
		{
			OnEndAutoRun();
		}
		else
		{
			OnStartAutoRun();
		}
	}
}

bool ADreamCatcherPlayerController::GetIsAutoRunning() const
{
	bool bIsAutoRunning = false;

	if (const UDCAbilitySystemComponent* DCASC = GetDCAbilitySystemComponent())
	{
		bIsAutoRunning = DCASC->GetTagCount(DCGameplayTags::Status_AutoRunning) > 0;
	}

	return bIsAutoRunning;
}

void ADreamCatcherPlayerController::OnStartAutoRun()
{
	if (UDCAbilitySystemComponent* DCASC = GetDCAbilitySystemComponent())
	{
		DCASC->SetLooseGameplayTagCount(DCGameplayTags::Status_AutoRunning, 1);
		K2_OnStartAutoRun();
	}
}

void ADreamCatcherPlayerController::OnEndAutoRun()
{
	if (UDCAbilitySystemComponent* DCASC = GetDCAbilitySystemComponent())
	{
		DCASC->SetLooseGameplayTagCount(DCGameplayTags::Status_AutoRunning, 0);
		K2_OnEndAutoRun();
	}
}
