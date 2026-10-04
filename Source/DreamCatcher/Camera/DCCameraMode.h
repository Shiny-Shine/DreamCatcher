// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"
#include "GameplayTagContainer.h"

#include "DCCameraMode.generated.h"

#define UE_API DREAMCATCHER_API

class AActor;
class UCanvas;
class UDCCameraComponent;

/**
 * EDCCameraModeBlendFunction
 *
 *	Blend function used for transitioning between camera modes.
 */
UENUM(BlueprintType)
enum class EDCCameraModeBlendFunction : uint8
{
	// Does a simple linear interpolation.
	Linear,

	// Immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by the exponent.
	EaseIn,

	// Smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by the exponent.
	EaseOut,

	// Smoothly accelerates and decelerates.  Ease amount controlled by the exponent.
	EaseInOut,

	COUNT	UMETA(Hidden)
};


/**
 * FDCCameraModeView
 *
 *	View data produced by the camera mode that is used to blend camera modes.
 */
struct FDCCameraModeView
{
public:

	FDCCameraModeView();

	void Blend(const FDCCameraModeView& Other, float OtherWeight);

public:

	FVector Location;
	FRotator Rotation;
	FRotator ControlRotation;
	float FieldOfView;

	// DreamCatcher extension: blend the existing per-mode input sensitivity.
	float LookSensitivityMultiplier = 1.0f;
};


/**
 * UDCCameraMode
 *
 *	Base class for all camera modes.
 */
UCLASS(MinimalAPI, Abstract, NotBlueprintable)
class UDCCameraMode : public UObject
{
	GENERATED_BODY()

public:

	UE_API UDCCameraMode();

	UE_API UDCCameraComponent* GetDCCameraComponent() const;

	UE_API virtual UWorld* GetWorld() const override;

	UE_API AActor* GetTargetActor() const;

	const FDCCameraModeView& GetCameraModeView() const { return View; }

	// Called when this camera mode is activated on the camera mode stack.
	virtual void OnActivation() {};

	// Called when this camera mode is deactivated on the camera mode stack.
	virtual void OnDeactivation() {};

	UE_API void UpdateCameraMode(float DeltaTime);

	float GetBlendTime() const { return BlendTime; }
	float GetBlendWeight() const { return BlendWeight; }
	UE_API void SetBlendWeight(float Weight);

	FGameplayTag GetCameraTypeTag() const
	{
		return CameraTypeTag;
	}

	UE_API virtual void DrawDebug(UCanvas* Canvas) const;

protected:

	UE_API virtual FVector GetPivotLocation() const;
	UE_API virtual FRotator GetPivotRotation() const;

	UE_API virtual void UpdateView(float DeltaTime);
	UE_API virtual void UpdateBlending(float DeltaTime);

protected:
	// A tag that can be queried by gameplay code that cares when a kind of camera mode is active
	// without having to ask about a specific mode (e.g., when aiming downsights to get more accuracy)
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	FGameplayTag CameraTypeTag;

	// Preserve the property used by existing Hip/Shoulder/Scope camera assets.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Input", meta = (ClampMin = "0.01"))
	float LookSensitivityMultiplier = 1.0f;

	// View output produced by the camera mode.
	FDCCameraModeView View;

	// The horizontal field of view (in degrees).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView;

	// Minimum view pitch (in degrees).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin;

	// Maximum view pitch (in degrees).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax;

	// How long it takes to blend in this mode.
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendTime;

	// Function used for blending.
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	EDCCameraModeBlendFunction BlendFunction;

	// Exponent used by blend functions to control the shape of the curve.
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendExponent;

	// Linear blend alpha used to determine the blend weight.
	float BlendAlpha;

	// Blend weight calculated using the blend alpha and function.
	float BlendWeight;

protected:
	/** If true, skips all interpolation and puts camera in ideal location.  Automatically set to false next frame. */
	UPROPERTY(transient)
	uint32 bResetInterpolation:1;
};


/**
 * UDCCameraModeStack
 *
 *	Stack used for blending camera modes.
 */
UCLASS()
class UDCCameraModeStack : public UObject
{
	GENERATED_BODY()

public:

	UDCCameraModeStack();

	void ActivateStack();
	void DeactivateStack();

	bool IsStackActivate() const { return bIsActive; }

	void PushCameraMode(TSubclassOf<UDCCameraMode> CameraModeClass);

	bool EvaluateStack(float DeltaTime, FDCCameraModeView& OutCameraModeView);

	void DrawDebug(UCanvas* Canvas) const;

	// Gets the tag associated with the top layer and the blend weight of it
	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;

protected:

	UDCCameraMode* GetCameraModeInstance(TSubclassOf<UDCCameraMode> CameraModeClass);

	void UpdateStack(float DeltaTime);
	void BlendStack(FDCCameraModeView& OutCameraModeView) const;

protected:

	bool bIsActive;

	UPROPERTY()
	TArray<TObjectPtr<UDCCameraMode>> CameraModeInstances;

	UPROPERTY()
	TArray<TObjectPtr<UDCCameraMode>> CameraModeStack;
};

#undef UE_API
