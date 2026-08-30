#include "Camera/DCCameraMode_ThirdPerson.h"

#include "Curves/CurveVector.h"
#include "Engine/World.h"

UDCCameraMode_ThirdPerson::UDCCameraMode_ThirdPerson()
{
	DefaultTargetOffset = FVector(-325.0f, 55.0f, 0.0f);

	bPreventPenetration = true;
	CollisionRadius = 14.0f;
	CollisionPushOutDistance = 2.0f;
	PenetrationBlendOutTime = 0.15f;

	CurrentDistanceBlockedPercent = 1.0f;
}

void UDCCameraMode_ThirdPerson::UpdateView(float DeltaTime)
{
	const FVector PivotLocation = GetPivotLocation();

	FRotator PivotRotation = GetPivotRotation();

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	FVector TargetOffset = DefaultTargetOffset;

	/*
	 * View Pitch를 Curve의 X 입력값으로 사용.
	 *
	 * 예:
	 * Pitch -60 → 아래를 보는 카메라 Offset
	 * Pitch   0 → 수평 카메라 Offset
	 * Pitch +60 → 위를 보는 카메라 Offset
	 */
	if (TargetOffsetCurve)
	{
		TargetOffset = TargetOffsetCurve->GetVectorValue(PivotRotation.Pitch);
	}

	View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);

	View.Rotation = PivotRotation;
	View.ControlRotation = PivotRotation;
	View.FieldOfView = FieldOfView;

	View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);

	View.Rotation = PivotRotation;
	View.ControlRotation = PivotRotation;
	View.FieldOfView = FieldOfView;
	View.LookSensitivityMultiplier = LookSensitivityMultiplier;

	UpdatePreventPenetration(DeltaTime, PivotLocation);
}

void UDCCameraMode_ThirdPerson::UpdatePreventPenetration(float DeltaTime, const FVector& PivotLocation)
{
	if (!bPreventPenetration)
	{
		CurrentDistanceBlockedPercent = 1.0f;
		return;
	}

	UWorld* World = GetWorld();
	AActor* TargetActor = GetTargetActor();

	if (!World || !TargetActor)
	{
		CurrentDistanceBlockedPercent = 1.0f;
		return;
	}

	const FVector DesiredCameraLocation = View.Location;

	const FVector PivotToCamera = DesiredCameraLocation - PivotLocation;

	const float DesiredDistance = PivotToCamera.Size();

	if (DesiredDistance <= KINDA_SMALL_NUMBER)
	{
		CurrentDistanceBlockedPercent = 1.0f;
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DCCameraPenetration), false, TargetActor);

	// 자신의 Capsule, Mesh, Weapon 등을 무시.
	QueryParams.AddIgnoredActor(TargetActor);

	FHitResult HitResult;

	const bool bBlocked = World->SweepSingleByChannel(
		HitResult,
		PivotLocation,
		DesiredCameraLocation,
		FQuat::Identity,
		ECC_Camera,
		FCollisionShape::MakeSphere(CollisionRadius),
		QueryParams
	);

	float TargetBlockedPercent = 1.0f;

	if (bBlocked)
	{
		// Hit.Time은 Trace 전체 구간의 0~1 비율.
		// PushOutDistance만큼 조금 더 Pivot 쪽으로 당겨 카메라가 벽 표면과 겹치지 않도록 함.
		const float PushOutPercent = CollisionPushOutDistance / DesiredDistance;

		TargetBlockedPercent = FMath::Clamp(HitResult.Time - PushOutPercent, 0.0f, 1.0f);
	}

	if (TargetBlockedPercent < CurrentDistanceBlockedPercent)
	{
		// 새 장애물이 생겼다면 즉시 카메라를 당김.
		// 여기서 보간하면 보간 중 몇 프레임 동안 벽 안에 남을 수 있음.
		CurrentDistanceBlockedPercent = TargetBlockedPercent;
	}
	else if (PenetrationBlendOutTime > 0.0f)
	{
		// 장애물에서 벗어날 때는 원래 거리로 부드럽게 복귀.
		const float InterpSpeed = 1.0f / PenetrationBlendOutTime;

		CurrentDistanceBlockedPercent = FMath::FInterpTo(
			CurrentDistanceBlockedPercent,
			TargetBlockedPercent, DeltaTime, InterpSpeed);
	}
	else
	{
		CurrentDistanceBlockedPercent = TargetBlockedPercent;
	}

	CurrentDistanceBlockedPercent =
		FMath::Clamp(CurrentDistanceBlockedPercent, 0.0f, 1.0f);

	View.Location = PivotLocation + PivotToCamera * CurrentDistanceBlockedPercent;
}
