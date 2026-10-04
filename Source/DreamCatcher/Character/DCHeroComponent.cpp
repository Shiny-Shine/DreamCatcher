// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCHeroComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Logging/MessageLog.h"
#include "DCLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "DreamCatcherPlayerController.h"
#include "Player/DCPlayerState.h"
#include "Engine/LocalPlayer.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Character/DCPawnData.h"
#include "DreamCatcherCharacter.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Input/DCInputConfig.h"
#include "Input/DCInputComponent.h"
#include "Camera/DCCameraComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "PlayerMappableInputConfig.h"
#include "Camera/DCCameraMode.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "CoreGlobals.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace DCHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UDCHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UDCHeroComponent::NAME_ActorFeatureName("Hero");

UDCHeroComponent::UDCHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;
}

void UDCHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogDC, Error,
		       TEXT(
			       "[UDCHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."
		       ));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("DCHeroComponent", "NotOnPawnError",
			                                       "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("DCHeroComponent");

			FMessageLog(HeroMessageLogName).Error()
			                               ->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
			                               ->AddToken(FTextToken::Create(Message));

			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		TArray<UActorComponent*> HeroComponents;
		GetPawn<APawn>()->GetComponents(UDCHeroComponent::StaticClass(), HeroComponents);
		if (!ensureAlwaysMsgf(HeroComponents.Num() == 1, TEXT("Only one DCHeroComponent should exist on [%s]."),
		                      *GetNameSafe(GetOwner())))
		{
			return;
		}

		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool UDCHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                          FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == DCGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == DCGameplayTags::InitState_Spawned && DesiredState ==
		DCGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<ADCPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) &&
				(Controller->PlayerState != nullptr) &&
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			ADreamCatcherPlayerController* DCPC = GetController<ADreamCatcherPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !DCPC || !DCPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == DCGameplayTags::InitState_DataAvailable && DesiredState ==
		DCGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		ADCPlayerState* DCPS = GetPlayerState<ADCPlayerState>();

		return DCPS && Manager->HasFeatureReachedInitState(Pawn, UDCPawnExtensionComponent::NAME_ActorFeatureName,
		                                                   DCGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == DCGameplayTags::InitState_DataInitialized && DesiredState ==
		DCGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UDCHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                             FGameplayTag DesiredState)
{
	UE_LOG(LogDC, Log, TEXT("[R2-2] Hero [%s] InitState=%s"), *GetNameSafe(GetOwner()), *DesiredState.ToString());

	if (CurrentState == DCGameplayTags::InitState_DataAvailable && DesiredState ==
		DCGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ADCPlayerState* DCPS = GetPlayerState<ADCPlayerState>();
		if (!ensure(Pawn && DCPS))
		{
			return;
		}

		const UDCPawnData* PawnData = nullptr;

		if (UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(DCPS->GetDCAbilitySystemComponent(), DCPS);
		}

		// Keep the original implementation available; do not bind it beside the Character path.
		if (!bUseLegacyPlayerInput && GetController<ADreamCatcherPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		// Hook up the delegate for all pawns, in case we spectate later
		if (PawnData && !bUseLegacyCameraMode)
		{
			if (UDCCameraComponent* CameraComponent = UDCCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
	}
}

void UDCHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UDCPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == DCGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UDCHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {
		DCGameplayTags::InitState_Spawned, DCGameplayTags::InitState_DataAvailable,
		DCGameplayTags::InitState_DataInitialized, DCGameplayTags::InitState_GameplayReady
	};

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UDCHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UsesOriginalADSInputRouting())
	{
		if (UDCPawnExtensionComponent* PawnExt = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
		{
			PawnExt->OnAbilitySystemUninitializing_Register(
				FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::UnbindOriginalADSInput));
			PawnExt->OnAbilitySystemInitialized_RegisterAndCall(
				FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::BindOriginalADSInput));
		}
	}

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UDCPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(DCGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UDCHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindOriginalADSInput();
	if (UDCPawnExtensionComponent* PawnExt = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExt->UnregisterAbilitySystemDelegates(this);
	}
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UDCHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	BindOriginalADSInput();

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UDCPawnData* PawnData = PawnExtComp->GetPawnData())
		{
			if (const UDCInputConfig* InputConfig = PawnData->InputConfig)
			{
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);
							}

							FModifyContextOptions Options = {};
							Options.bIgnoreAllPressedKeysUntilRelease = false;
							// Actually add the config to the local player							
							Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
						}
					}
				}

				// The DC Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the UDCInputComponent or modify this component accordingly.
				UDCInputComponent* DCIC = Cast<UDCInputComponent>(PlayerInputComponent);
				if (ensureMsgf(
					DCIC,
					TEXT(
						"Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UDCInputComponent or a subclass of it."
					)))
				{
					// Add the key mappings that may have been set by the player
					DCIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events. 
					TArray<uint32> BindHandles;
					DCIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
					                         &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					// 원본 Triggered / Completed 바인딩은 유지. 현재 프로젝트에서 처리하던 입력 취소도 해제로 전달.
					for (const FDCInputAction& Action : InputConfig->AbilityInputActions)
					{
						if (Action.InputAction && Action.InputTag.IsValid())
						{
							BindHandles.Add(DCIC->BindAction(Action.InputAction, ETriggerEvent::Canceled, this,
							                                 &ThisClass::Input_AbilityInputTagReleased,
							                                 Action.InputTag).GetHandle());
						}
					}

					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this,
					                       &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered,
					                       this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered,
					                       this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this,
					                       &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_AutoRun, ETriggerEvent::Triggered,
					                       this, &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Aim, ETriggerEvent::Started, this,
					                       &ThisClass::Input_AimPressed,/*bLogIfNotFound=*/ true);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Aim, ETriggerEvent::Completed, this,
					                       &ThisClass::Input_AimReleased,/*bLogIfNotFound=*/ true);
					DCIC->BindNativeAction(InputConfig, DCGameplayTags::InputTag_Aim, ETriggerEvent::Canceled, this,
					                       &ThisClass::Input_AimCanceled,/*bLogIfNotFound=*/ true);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UDCHeroComponent::AddAdditionalInputConfig(const UDCInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UDCInputComponent* DCIC = Pawn->FindComponentByClass<UDCInputComponent>();
		if (ensureMsgf(
			DCIC,
			TEXT(
				"Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UDCInputComponent or a subclass of it."
			)))
		{
			DCIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
			                         &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UDCHeroComponent::RemoveAdditionalInputConfig(const UDCInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UDCHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UDCHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (UsesOriginalADSInputRouting() &&
		(InputTag == DCGameplayTags::InputTag_Aim ||
		 InputTag == DCGameplayTags::InputTag_Weapon_ADS_Shoulder ||
		 InputTag == DCGameplayTags::InputTag_Weapon_ADS_Scope))
	{
		// Physical aim uses Started/Completed below. Logical ADS requests come only from the adapter.
		return;
	}
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UDCAbilitySystemComponent* DCASC = PawnExtComp->GetDCAbilitySystemComponent())
			{
				DCASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UDCHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (UsesOriginalADSInputRouting() &&
		(InputTag == DCGameplayTags::InputTag_Aim ||
		 InputTag == DCGameplayTags::InputTag_Weapon_ADS_Shoulder ||
		 InputTag == DCGameplayTags::InputTag_Weapon_ADS_Scope))
	{
		return;
	}
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UDCAbilitySystemComponent* DCASC = PawnExtComp->GetDCAbilitySystemComponent())
		{
			DCASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UDCHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	if (ADreamCatcherPlayerController* DCController = Cast<ADreamCatcherPlayerController>(Controller))
	{
		DCController->SetIsAutoRunning(false);
	}

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UDCHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const ADreamCatcherCharacter* Character = Cast<ADreamCatcherCharacter>(Pawn);

	const float LookSensitivity = Character ? Character->GetCurrentLookSensitivityMultiplier() : 1.0f;

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * LookSensitivity);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * LookSensitivity);
	}
}

void UDCHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * DCHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * DCHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UDCHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (ADreamCatcherCharacter* Character = GetPawn<ADreamCatcherCharacter>())
	{
		Character->ToggleCrouch();
	}
}

void UDCHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (ADreamCatcherPlayerController* Controller = Cast<ADreamCatcherPlayerController>(Pawn->GetController()))
		{
			// Toggle auto running
			Controller->SetIsAutoRunning(!Controller->GetIsAutoRunning());
		}
	}
}

TSubclassOf<UDCCameraMode>UDCHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (UDCPawnExtensionComponent* PawnExtComp = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UDCPawnData* PawnData = PawnExtComp->GetPawnData<UDCPawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}

	return nullptr;
}

void UDCHeroComponent::SetAbilityCameraMode(TSubclassOf<UDCCameraMode> CameraMode,
                                            const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void UDCHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}

void UDCHeroComponent::Input_AimPressed(const FInputActionValue& InputActionValue)
{
	if (!UsesOriginalADSInputRouting())
	{
		Input_AbilityInputTagPressed(DCGameplayTags::InputTag_Aim);
		return;
	}

	// A context can disappear while the button is down, so Completed is not guaranteed.
	// Recover on a genuinely new physical press, never on a held-key mapping rebuild.
	if (bIgnoreAimUntilRelease && GFrameCounter > AimSuppressionFrame && IsFreshAimKeyPress())
	{
		bIgnoreAimUntilRelease = false;
		bPhysicalAimPressed = false;
	}
	if (bPhysicalAimPressed)
	{
		return;
	}
	bPhysicalAimPressed = true;
	if (bIgnoreAimUntilRelease || !CanRouteOriginalADSInput())
	{
		ResetOriginalADSGesture();
		return;
	}

	FlushADSInputCleanup();
	UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	if (RoutedADSTag == DCGameplayTags::InputTag_Weapon_ADS_Scope ||
		ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope))
	{
		// Scope -> Hip consumes this entire press, including its eventual release.
		bIgnoreAimUntilRelease = true;
		AimSuppressionFrame = GFrameCounter;
		ReleaseOriginalADS();
		return;
	}
	if (RoutedADSHandle.IsValid() || ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Shoulder))
	{
		bIgnoreAimUntilRelease = true;
		AimSuppressionFrame = GFrameCounter;
		return;
	}

	bAwaitingAimHold = true;
	AimPressTime = GetWorld()->GetTimeSeconds();
	if (ShoulderHoldThreshold <= 0.0f)
	{
		HandleOriginalADSHold();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(AimHoldTimerHandle, this,
			&ThisClass::HandleOriginalADSHold, ShoulderHoldThreshold, false);
	}
}

void UDCHeroComponent::Input_AimReleased(const FInputActionValue& InputActionValue)
{
	if (!UsesOriginalADSInputRouting())
	{
		Input_AbilityInputTagReleased(DCGameplayTags::InputTag_Aim);
		return;
	}

	const bool bWasPressed = bPhysicalAimPressed;
	const bool bWasAwaitingHold = bAwaitingAimHold;
	bPhysicalAimPressed = false;
	bAwaitingAimHold = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimHoldTimerHandle);
	}
	if (bIgnoreAimUntilRelease)
	{
		bIgnoreAimUntilRelease = false;
		return;
	}
	if (!bWasPressed)
	{
		return;
	}
	if (RoutedADSTag == DCGameplayTags::InputTag_Weapon_ADS_Shoulder)
	{
		ReleaseOriginalADS();
	}
	else if (bWasAwaitingHold && CanRouteOriginalADSInput())
	{
		// Input can run before the timer in the threshold frame. A late release is NOT a tap.
		const double HeldTime = GetWorld()->GetTimeSeconds() - AimPressTime;
		if (HeldTime < ShoulderHoldThreshold)
		{
			RequestOriginalADS(DCGameplayTags::InputTag_Weapon_ADS_Scope);
		}
	}
}

void UDCHeroComponent::Input_AimCanceled(const FInputActionValue& InputActionValue)
{
	if (UsesOriginalADSInputRouting())
	{
		if (UDCAbilitySystemComponent* ASC = AimInputASC.Get())
		{
			ASC->CancelAimInputAndState();
		}
		else
		{
			ResetOriginalADSGesture();
		}
		return;
	}
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const UDCPawnExtensionComponent* PawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn);

	if (!PawnExtension)
	{
		return;
	}

	if (UDCAbilitySystemComponent* ASC = PawnExtension->GetDCAbilitySystemComponent())
	{
		// 취소는 정상적인 버튼 해제와 다름, 짧은 클릭으로 처리하여 Scope가 켜지지 않도록 함.
		ASC->CancelAimInputAndState();
	}
}

void UDCHeroComponent::BindOriginalADSInput()
{
	if (!UsesOriginalADSInputRouting())
	{
		return;
	}
	const APawn* Pawn = GetPawn<APawn>();
	const UDCPawnExtensionComponent* PawnExt = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	UDCAbilitySystemComponent* ASC = PawnExt ? PawnExt->GetDCAbilitySystemComponent() : nullptr;
	if (!Pawn || !Pawn->IsLocallyControlled() || !ASC || ASC->GetAvatarActor() != Pawn ||
		PawnExt->IsAbilitySystemUninitializing() || AimInputASC.Get() == ASC)
	{
		return;
	}

	UnbindOriginalADSInput();
	AimInputASC = ASC;
	AimCanceledDelegateHandle = ASC->OnAimInputCanceled.AddUObject(this, &ThisClass::ResetOriginalADSGesture);
	AimEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleOriginalADSEnded);
	AimFailedDelegateHandle = ASC->AbilityFailedCallbacks.AddUObject(this, &ThisClass::HandleOriginalADSFailed);

	if (const UDCPawnData* PawnData = PawnExt->GetPawnData())
	{
		if (PawnData->InputConfig)
		{
			RoutedAimAction = PawnData->InputConfig->FindNativeInputActionForTag(DCGameplayTags::InputTag_Aim, false);
		}
	}
	if (RoutedAimAction.IsValid())
	{
		// Fallback for initial possession before the subsystem has rebuilt its active mappings.
		for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
		{
			if (const UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
			{
				for (const FEnhancedActionKeyMapping& KeyMapping : IMC->GetMappings())
				{
					if (KeyMapping.Action == RoutedAimAction.Get())
					{
						DefaultAimKeys.AddUnique(KeyMapping.Key);
					}
				}
			}
		}
	}
	bIgnoreAimUntilRelease = IsAimKeyDown();
	AimSuppressionFrame = GFrameCounter;
	UE_LOG(LogDC, Log, TEXT("[R4-3] Original ADS input routing bound on [%s]."), *GetNameSafe(Pawn));
}

void UDCHeroComponent::UnbindOriginalADSInput()
{
	if (UDCAbilitySystemComponent* ASC = AimInputASC.Get())
	{
		if (ASC->GetAvatarActor() == GetOwner())
		{
			ASC->CancelAimInputAndState();
		}
		ASC->OnAimInputCanceled.Remove(AimCanceledDelegateHandle);
		ASC->OnAbilityEnded.Remove(AimEndedDelegateHandle);
		ASC->AbilityFailedCallbacks.Remove(AimFailedDelegateHandle);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimHoldTimerHandle);
		World->GetTimerManager().ClearTimer(ADSInputCleanupTimerHandle);
		World->GetTimerManager().ClearTimer(ADSActivationCheckTimerHandle);
	}
	AimCanceledDelegateHandle.Reset();
	AimEndedDelegateHandle.Reset();
	AimFailedDelegateHandle.Reset();
	AimInputASC.Reset();
	RoutedAimAction.Reset();
	DefaultAimKeys.Reset();
	ADSInputCleanupHandles.Reset();
	RoutedADSHandle = FGameplayAbilitySpecHandle();
	RoutedADSTag = FGameplayTag();
	bAwaitingAimHold = false;
	bPhysicalAimPressed = false;
	bIgnoreAimUntilRelease = false;
}

void UDCHeroComponent::ResetOriginalADSGesture()
{
	const bool bHadRequest = bAwaitingAimHold || RoutedADSHandle.IsValid();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimHoldTimerHandle);
		World->GetTimerManager().ClearTimer(ADSActivationCheckTimerHandle);
	}
	bAwaitingAimHold = false;
	bIgnoreAimUntilRelease |= bPhysicalAimPressed || IsAimKeyDown();
	AimSuppressionFrame = GFrameCounter;
	RoutedADSHandle = FGameplayAbilitySpecHandle();
	RoutedADSTag = FGameplayTag();
	if (bHadRequest)
	{
		UE_LOG(LogDC, Log, TEXT("[R4-3] ADS gesture reset. IgnoreUntilRelease=%d."), bIgnoreAimUntilRelease);
	}
}

bool UDCHeroComponent::CanRouteOriginalADSInput() const
{
	const APawn* Pawn = GetPawn<APawn>();
	const UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	const UDCPawnExtensionComponent* PawnExt = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	return UsesOriginalADSInputRouting() && Pawn && Pawn->IsLocallyControlled() && GetWorld() &&
		!GetWorld()->IsPaused() && ASC && ASC->GetAvatarActor() == Pawn && PawnExt &&
		!PawnExt->IsAbilitySystemUninitializing() &&
		!ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked) &&
		!ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death) &&
		!ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dead) &&
		!ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dodging);
}

void UDCHeroComponent::HandleOriginalADSHold()
{
	if (!bAwaitingAimHold || !bPhysicalAimPressed || bIgnoreAimUntilRelease)
	{
		return;
	}
	bAwaitingAimHold = false;
	if (!RequestOriginalADS(DCGameplayTags::InputTag_Weapon_ADS_Shoulder))
	{
		ResetOriginalADSGesture();
	}
}

bool UDCHeroComponent::RequestOriginalADS(const FGameplayTag& InputTag)
{
	if (!CanRouteOriginalADSInput() || RoutedADSHandle.IsValid())
	{
		return false;
	}
	FlushADSInputCleanup();
	UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	FGameplayAbilitySpecHandle MatchingHandle;
	if (ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Shoulder) ||
		ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope))
	{
		// The state may have changed externally while the hold timer was pending.
		return false;
	}
	int32 MatchCount = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && !Spec.PendingRemove && !Spec.RemoveAfterActivation &&
			Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			MatchingHandle = Spec.Handle;
			++MatchCount;
		}
	}
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(MatchingHandle);
	const UDCGameplayAbility* Ability = Spec ? Cast<UDCGameplayAbility>(Spec->Ability) : nullptr;
	if (MatchCount != 1 || !Ability || Spec->IsActive() ||
		Ability->GetActivationPolicy() != EDCAbilityActivationPolicy::OnInputTriggered ||
		Ability->GetNetExecutionPolicy() != EGameplayAbilityNetExecutionPolicy::LocalPredicted)
	{
		UE_LOG(LogDC, Warning, TEXT("[R4-3] ADS request rejected for [%s]: expected one inactive OnInputTriggered, LocalPredicted grant (found %d)."),
			*InputTag.ToString(), MatchCount);
		return false;
	}

	RoutedADSHandle = MatchingHandle;
	RoutedADSTag = InputTag;
	// Preserve original ProcessAbilityInput -> Spec.InputPressed -> WaitInputRelease semantics.
	// Scope keeps this logical press until the NEXT physical press, not the tap's release.
	ASC->AbilityInputTagPressed(InputTag);
	ADSRequestFrame = GFrameCounter;
	ADSActivationCheckTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckOriginalADSActivation);
	UE_LOG(LogDC, Log, TEXT("[R4-3] ADS press queued: [%s]."), *InputTag.ToString());
	return true;
}

void UDCHeroComponent::CheckOriginalADSActivation()
{
	UWorld* World = GetWorld();
	if (!World || !RoutedADSHandle.IsValid())
	{
		return;
	}
	// A request made by the hold timer can follow PostProcessInput in the same frame.
	// Allow a full subsequent input frame. LocalPredicted ADS does not wait for a server round trip.
	if (GFrameCounter <= ADSRequestFrame + 1)
	{
		ADSActivationCheckTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckOriginalADSActivation);
		return;
	}
	UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	if (!ASC || ASC->GetAvatarActor() != GetOwner())
	{
		ResetOriginalADSGesture();
		return;
	}
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(RoutedADSHandle);
	if (!Spec || !Spec->IsActive())
	{
		// TryActivateAbility has early exits without AbilityFailedCallbacks (e.g. a removed grant).
		UE_LOG(LogDC, Warning, TEXT("[R4-3] ADS request did not activate: [%s]. Clearing pending input."), *RoutedADSTag.ToString());
		ASC->ClearAbilityInputForHandle(RoutedADSHandle);
		ResetOriginalADSGesture();
	}
	else
	{
		UE_LOG(LogDC, Log, TEXT("[R4-3] ADS active confirmed: [%s]."), *RoutedADSTag.ToString());
	}
}

void UDCHeroComponent::ReleaseOriginalADS()
{
	UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	if (!ASC || ASC->GetAvatarActor() != GetOwner())
	{
		ResetOriginalADSGesture();
		return;
	}
	FlushADSInputCleanup();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(RoutedADSHandle);
	if (Spec && Spec->IsActive())
	{
		ASC->AbilityInputTagReleased(RoutedADSTag);
		UE_LOG(LogDC, Log, TEXT("[R4-3] ADS release queued: [%s]."), *RoutedADSTag.ToString());
	}
	else if (RoutedADSHandle.IsValid())
	{
		// A second press/release may arrive before the first request was processed. Do not activate then end it.
		ASC->ClearAbilityInputForHandle(RoutedADSHandle);
		ResetOriginalADSGesture();
	}
	else
	{
		// Also recover a legacy Scope GE if the migration option was changed with old state present.
		ASC->CancelAimInputAndState();
	}
}

void UDCHeroComponent::HandleOriginalADSEnded(const FAbilityEndedData& EndedData)
{
	if (RoutedADSHandle.IsValid() && EndedData.AbilitySpecHandle == RoutedADSHandle)
	{
		UE_LOG(LogDC, Log, TEXT("[R4-3] ADS ended: [%s], Cancelled=%d."), *RoutedADSTag.ToString(), EndedData.bWasCancelled);
		DeferADSInputCleanup(RoutedADSHandle);
		ResetOriginalADSGesture();
	}
}

void UDCHeroComponent::HandleOriginalADSFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureTags)
{
	UDCAbilitySystemComponent* ASC = AimInputASC.Get();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(RoutedADSHandle) : nullptr;
	// Failure callbacks can carry the CDO, not an instance with a current spec handle.
	if (Ability && Spec && Spec->Ability && !Spec->IsActive() && Spec->Ability->GetClass() == Ability->GetClass())
	{
		UE_LOG(LogDC, Warning, TEXT("[R4-3] ADS activation failed: [%s], Reasons=[%s]."),
			*RoutedADSTag.ToString(), *FailureTags.ToStringSimple());
		DeferADSInputCleanup(RoutedADSHandle);
		ResetOriginalADSGesture();
	}
}

void UDCHeroComponent::DeferADSInputCleanup(FGameplayAbilitySpecHandle Handle)
{
	ADSInputCleanupHandles.AddUnique(Handle);
	if (UWorld* World = GetWorld(); World && !ADSInputCleanupTimerHandle.IsValid())
	{
		// End/failure can be broadcast while the original ASC is iterating its input arrays.
		ADSInputCleanupTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushADSInputCleanup);
	}
}

void UDCHeroComponent::FlushADSInputCleanup()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ADSInputCleanupTimerHandle);
	}
	if (UDCAbilitySystemComponent* ASC = AimInputASC.Get(); ASC && ASC->GetAvatarActor() == GetOwner())
	{
		for (const FGameplayAbilitySpecHandle Handle : ADSInputCleanupHandles)
		{
			const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
			if (!Spec || !Spec->IsActive())
			{
				ASC->ClearAbilityInputForHandle(Handle);
			}
		}
	}
	ADSInputCleanupHandles.Reset();
}

void UDCHeroComponent::GetAimInputKeys(TArray<FKey>& OutKeys) const
{
	OutKeys.Reset();
	if (const APlayerController* PC = GetController<APlayerController>())
	{
		if (const ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (const UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (RoutedAimAction.IsValid())
				{
					OutKeys = Subsystem->QueryKeysMappedToAction(RoutedAimAction.Get());
				}
			}
		}
	}
	if (OutKeys.IsEmpty())
	{
		OutKeys = DefaultAimKeys;
	}
}

bool UDCHeroComponent::IsAimKeyDown() const
{
	if (const APlayerController* PC = GetController<APlayerController>())
	{
		TArray<FKey> Keys;
		GetAimInputKeys(Keys);
		for (const FKey& Key : Keys)
		{
			if (PC->IsInputKeyDown(Key))
			{
				return true;
			}
		}
	}
	return false;
}

bool UDCHeroComponent::IsFreshAimKeyPress() const
{
	if (const APlayerController* PC = GetController<APlayerController>())
	{
		TArray<FKey> Keys;
		GetAimInputKeys(Keys);
		for (const FKey& Key : Keys)
		{
			if (PC->WasInputKeyJustPressed(Key))
			{
				return true;
			}
		}
	}
	return false;
}
