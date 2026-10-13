// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/DCLyraSimulatedInputWidget.h"
#include "EnhancedInputSubsystems.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "DCLogChannels.h"
#include "InputKeyEventArgs.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraSimulatedInputWidget)

#define LOCTEXT_NAMESPACE "LyraSimulatedInputWidget"

UDCLyraSimulatedInputWidget::UDCLyraSimulatedInputWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetConsumePointerInput(true);
}

#if WITH_EDITOR
const FText UDCLyraSimulatedInputWidget::GetPaletteCategory()
{
	return LOCTEXT("PalleteCategory", "Input");
}
#endif // WITH_EDITOR

void UDCLyraSimulatedInputWidget::NativeConstruct()
{
	// Find initial key, then listen for any changes to control mappings
	QueryKeyToSimulate();

	if (UEnhancedInputLocalPlayerSubsystem* System = GetEnhancedInputSubsystem())
	{
		System->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UDCLyraSimulatedInputWidget::OnControlMappingsRebuilt);
	}
	
	Super::NativeConstruct();
}

void UDCLyraSimulatedInputWidget::NativeDestruct()
{
	if (UEnhancedInputLocalPlayerSubsystem* System = GetEnhancedInputSubsystem())
	{
		System->ControlMappingsRebuiltDelegate.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UDCLyraSimulatedInputWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	FlushSimulatedInput();
	
	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

UEnhancedInputLocalPlayerSubsystem* UDCLyraSimulatedInputWidget::GetEnhancedInputSubsystem() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = GetOwningLocalPlayer())
		{
			return LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}
	return nullptr;
}

UEnhancedPlayerInput* UDCLyraSimulatedInputWidget::GetPlayerInput() const
{
	if (UEnhancedInputLocalPlayerSubsystem* System = GetEnhancedInputSubsystem())
	{
		return System->GetPlayerInput();
	}
	return nullptr;
}

void UDCLyraSimulatedInputWidget::InputKeyValue(const FVector& Value)
{
	const APlayerController* PC = GetOwningPlayer();
	const FPlatformUserId UserId = PC ? PC->GetPlatformUserId() : PLATFORMUSERID_NONE;
	// If we have an associated input action then we can use it
	if (AssociatedAction)
	{
		if (UEnhancedInputLocalPlayerSubsystem* System = GetEnhancedInputSubsystem())
		{
			// We don't want to apply any modifiers or triggers to this action, but they are required for the function signature
			TArray<UInputModifier*> Modifiers;
			TArray<UInputTrigger*> Triggers;
			System->InjectInputVectorForAction(AssociatedAction, Value, Modifiers, Triggers);
		}
	}
	// In case there is no associated input action, we can attempt to simulate input on the fallback key
	else if (UEnhancedPlayerInput* Input = GetPlayerInput())
	{
		const FInputDeviceId DeviceToSimulate = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
		if(KeyToSimulate.IsValid())
		{
			const float DeltaTime = GetWorld()->GetDeltaSeconds();
			auto SimulateKeyPress = [Input, DeltaTime, DeviceToSimulate](const FKey& KeyToSim, const float Value, const EInputEvent Event)
			{
				FInputKeyEventArgs Args = FInputKeyEventArgs::CreateSimulated(
					KeyToSim,
					Event,
					Value,
					KeyToSim.IsAnalog() ? 1 : 0,
					DeviceToSimulate);

				Args.DeltaTime = DeltaTime;
			
				Input->InputKey(Args);
			};
			
			// For keys which are the "root" of the key pair (such as Mouse2D
			// being made up of the MouseX and MouseY keys) we should call InputKey for each key in the pair,
			// not the paired key itself. This is so that the events accumulate correctly in
			// the key state map of UPlayerInput. All input events
			// from the message handler and viewport client work this way, so when we simulate key inputs, we should
			// do so as well.
			if (const EKeys::FPairedKeyDetails* PairDetails = EKeys::GetPairedKeyDetails(KeyToSimulate))
			{
				SimulateKeyPress(PairDetails->XKeyDetails->GetKey(), Value.X, IE_Axis);
				SimulateKeyPress(PairDetails->YKeyDetails->GetKey(), Value.Y, IE_Axis);
			}
			else
			{
				SimulateKeyPress(KeyToSimulate, Value.X, IE_Pressed);
			}
		}
	}
	else
	{
		UE_LOG(LogDC, Error, TEXT("'%s' is attempting to simulate input but has no player input!"), *GetNameSafe(this));
	}
}

void UDCLyraSimulatedInputWidget::InputKeyValue2D(const FVector2D& Value)
{
	InputKeyValue(FVector(Value.X, Value.Y, 0.0));
}

void UDCLyraSimulatedInputWidget::FlushSimulatedInput()
{
	if (UEnhancedPlayerInput* Input = GetPlayerInput())
	{
		Input->FlushPressedKeys();
	}
}

void UDCLyraSimulatedInputWidget::QueryKeyToSimulate()
{
	if (UEnhancedInputLocalPlayerSubsystem* System = GetEnhancedInputSubsystem())
	{
		TArray<FKey> Keys = System->QueryKeysMappedToAction(AssociatedAction);
		if(!Keys.IsEmpty() && Keys[0].IsValid())
		{
			KeyToSimulate = Keys[0];
		}
		else
		{
			KeyToSimulate = FallbackBindingKey;
		}
	}
}

void UDCLyraSimulatedInputWidget::OnControlMappingsRebuilt()
{
	QueryKeyToSimulate();
}

#undef LOCTEXT_NAMESPACE
