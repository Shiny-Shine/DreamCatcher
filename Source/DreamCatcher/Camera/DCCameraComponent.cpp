#include "Camera/DCCameraComponent.h"

#include "Camera/DCCameraMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UDCCameraComponent::UDCCameraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CameraModeStack = nullptr;
	FieldOfViewOffset = 0.0f;
	CurrentLookSensitivityMultiplier = 1.0f;
}

UDCCameraComponent* UDCCameraComponent::FindCameraComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UDCCameraComponent>() : nullptr;
}

void UDCCameraComponent::OnRegister()
{
	Super::OnRegister();

	if (!CameraModeStack)
	{
		CameraModeStack = NewObject<UDCCameraModeStack>(this);

		check(CameraModeStack);
	}
}

void UDCCameraComponent::UpdateCameraModes()
{
	if (!CameraModeStack || !CameraModeStack->IsStackActive() || !DetermineCameraModeDelegate.IsBound())
	{
		return;
	}

	const TSubclassOf<UDCCameraMode> CameraModeClass = DetermineCameraModeDelegate.Execute();

	if (CameraModeClass)
	{
		CameraModeStack->PushCameraMode(CameraModeClass);
	}
}

void UDCCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	// Mode가 없더라도 일반 CameraComponent로 동작할 수 있도록 먼저 기본 View를 구함.
	Super::GetCameraView(DeltaTime, DesiredView);

	if (!CameraModeStack)
	{
		return;
	}

	UpdateCameraModes();

	FDCCameraModeView CameraModeView;

	if (!CameraModeStack->EvaluateStack(DeltaTime, CameraModeView))
	{
		// Mode가 아직 없다면 기본 Camera View에 FOV Offset만 적용.
		DesiredView.FOV += FieldOfViewOffset;
		FieldOfViewOffset = 0.0f;
		return;
	}

	CameraModeView.FieldOfView += FieldOfViewOffset;

	FieldOfViewOffset = 0.0f;

	// CameraMode가 계산한 ControlRotation을 PlayerController와 동기화.
	if (APawn* TargetPawn = Cast<APawn>(GetTargetActor()))
	{
		if (APlayerController* PlayerController = TargetPawn->GetController<APlayerController>())
		{
			PlayerController->SetControlRotation(CameraModeView.ControlRotation);
		}
	}

	SetWorldLocationAndRotation(CameraModeView.Location, CameraModeView.Rotation);

	FieldOfView = CameraModeView.FieldOfView;

	CurrentLookSensitivityMultiplier = CameraModeView.LookSensitivityMultiplier;

	// Super::GetCameraView에서 설정한 PostProcess, AspectRatio 등의 정보는 유지하고
	// Mode가 담당하는 Transform과 FOV만 교체.
	DesiredView.Location = CameraModeView.Location;

	DesiredView.Rotation = CameraModeView.Rotation;

	DesiredView.FOV = CameraModeView.FieldOfView;
}

void UDCCameraComponent::GetBlendInfo(float& OutTopModeWeight, FGameplayTag& OutTopModeTag) const
{
	if (!CameraModeStack)
	{
		OutTopModeWeight = 0.0f;
		OutTopModeTag = FGameplayTag();
		return;
	}

	CameraModeStack->GetBlendInfo(OutTopModeWeight, OutTopModeTag);
}
