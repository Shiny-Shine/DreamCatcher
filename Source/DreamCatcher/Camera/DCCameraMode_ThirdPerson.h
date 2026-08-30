#pragma once

#include "CoreMinimal.h"
#include "Camera/DCCameraMode.h"
#include "DCCameraMode_ThirdPerson.generated.h"

class UCurveVector;

// DreamCatcher의 3인칭 CameraMode.
// Pivot을 기준으로 로컬 Offset을 적용하고, Camera Channel Sphere Sweep으로 벽 관통을 방지.
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCCameraMode_ThirdPerson : public UDCCameraMode
{
	GENERATED_BODY()

public:
	UDCCameraMode_ThirdPerson();

protected:
	virtual void UpdateView(float DeltaTime) override;

	// Pivot에서 원하는 카메라 위치까지 벽 충돌을 검사.
	void UpdatePreventPenetration(float DeltaTime, const FVector& PivotLocation);

	/**
	 * 카메라 Pitch에 따라 로컬 Offset을 반환하는 Curve.
	 *
	 * X: 앞/뒤. 음수면 캐릭터 뒤.
	 * Y: 좌/우. 양수면 오른쪽 Shoulder.
	 * Z: 위/아래.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Third Person")
	TObjectPtr<const UCurveVector> TargetOffsetCurve;

	// TargetOffsetCurve가 없을 때 사용하는 고정 Offset.
	// Pivot이 이미 Pawn의 Eye Height이므로 기존 SocketOffset Z=65와 맞추려면 Z는 약 0을 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Third Person")
	FVector DefaultTargetOffset = FVector(-325.0f, 55.0f, 0.0f);

	// Camera Channel 충돌을 사용할지 여부.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Collision")
	bool bPreventPenetration = true;

	// Sphere Sweep의 반지름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Collision",
		meta = (ClampMin = "0.0", Units = "cm"))
	float CollisionRadius = 14.0f;

	// 벽 표면에서 카메라를 추가로 밀어낼 거리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Collision",
		meta = (ClampMin = "0.0", Units = "cm"))
	float CollisionPushOutDistance = 2.0f;

	// 장애물이 사라진 뒤 원래 거리로 돌아가는 시간.
	// 벽에 막혔을 때는 관통 방지를 위해 즉시 당기고, 벽에서 벗어날 때만 부드럽게 복귀.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Camera|Collision",
		meta = (ClampMin = "0.0", Units = "s"))
	float PenetrationBlendOutTime = 0.15f;

private:
	// 0: Pivot 위치까지 완전히 당겨짐. 1: 원래 카메라 위치.
	float CurrentDistanceBlockedPercent = 1.0f;
};
