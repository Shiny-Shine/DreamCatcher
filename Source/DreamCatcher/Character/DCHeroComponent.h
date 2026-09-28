// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "GameplayAbilitySpecHandle.h"
#include "DCHeroComponent.generated.h"

#define UE_API DREAMCATCHER_API

namespace EEndPlayReason { enum Type : int; }
struct FLoadedMappableConfigPair;
struct FMappableConfigPair;

class UGameFrameworkComponentManager;
class UInputComponent;
class UDCCameraMode;
class UDCInputConfig;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;
struct FInputActionValue;

/**
 * Component that sets up input and camera handling for player controlled pawns (or bots that simulate players).
 * This depends on a PawnExtensionComponent to coordinate initialization.
 */
UCLASS(MinimalAPI, Blueprintable, Meta=(BlueprintSpawnableComponent))
class UDCHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	UE_API UDCHeroComponent(const FObjectInitializer& ObjectInitializer);

	/** Returns the hero component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "DC|Hero")
	static UDCHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UDCHeroComponent>() : nullptr); }

	/** Overrides the camera from an active gameplay ability */
	UE_API void SetAbilityCameraMode(TSubclassOf<UDCCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** Clears the camera override if it is set */
	UE_API void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** Adds mode-specific input config */
	UE_API void AddAdditionalInputConfig(const UDCInputConfig* InputConfig);

	/** Removes a mode-specific input config if it has been added */
	UE_API void RemoveAdditionalInputConfig(const UDCInputConfig* InputConfig);

	/** True if this is controlled by a real player and has progressed far enough in initialization where additional input bindings can be added */
	UE_API bool IsReadyToBindInputs() const;

	// R2-3/R4 migration boundary: the existing Character currently owns these paths.
	bool UsesLegacyPlayerInput() const { return bUseLegacyPlayerInput; }
	bool UsesLegacyCameraMode() const { return bUseLegacyCameraMode; }
	
	/** The name of the extension event sent via UGameFrameworkComponentManager when ability inputs are ready to bind */
	static UE_API const FName NAME_BindInputsNow;

	/** The name of this component-implemented feature */
	static UE_API const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	UE_API virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	UE_API virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	UE_API virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	UE_API virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

protected:

	UE_API virtual void OnRegister() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UE_API virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	UE_API void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	UE_API void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	UE_API void Input_Move(const FInputActionValue& InputActionValue);
	UE_API void Input_LookMouse(const FInputActionValue& InputActionValue);
	UE_API void Input_LookStick(const FInputActionValue& InputActionValue);
	UE_API void Input_Crouch(const FInputActionValue& InputActionValue);
	UE_API void Input_AutoRun(const FInputActionValue& InputActionValue);
	
	// DreamCatcher의 짧은 클릭 / Hold 조준 규칙을 위한 입력 연결.
    UE_API void Input_AimPressed(const FInputActionValue& InputActionValue);
    UE_API void Input_AimReleased(const FInputActionValue& InputActionValue);
    UE_API void Input_AimCanceled(const FInputActionValue& InputActionValue);

	UE_API TSubclassOf<UDCCameraMode> DetermineCameraMode() const;

protected:
	
	// Keep Started/Completed/Canceled aim input until the R2-3 binding migration is verified.
	UPROPERTY(EditDefaultsOnly, Category = "DC|Migration")
	bool bUseLegacyPlayerInput = true;

	// Keep the existing Hip/Shoulder/Scope camera selection until R4.
	UPROPERTY(EditDefaultsOnly, Category = "DC|Migration")
	bool bUseLegacyCameraMode = true;

	UPROPERTY(EditAnywhere)
	TArray<FInputMappingContextAndPriority> DefaultInputMappings;
	
	/** Camera mode set by an ability. */
	UPROPERTY()
	TSubclassOf<UDCCameraMode> AbilityCameraMode;

	/** Spec handle for the last ability to set a camera mode. */
	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	/** True when player input bindings have been applied, will never be true for non - players */
	bool bReadyToBindInputs;
};

#undef UE_API
