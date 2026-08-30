#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameplayTagContainer.h"
#include "DCCameraComponent.generated.h"

class UDCCameraMode;
class UDCCameraModeStack;

DECLARE_DELEGATE_RetVal(TSubclassOf<UDCCameraMode>, FDCCameraModeDelegate);

/**
 * DreamCatcher의 CameraMode Stack을 실행하는 CameraComponent.
 *
 * 아직 Character에 연결 X.
 */
UCLASS(ClassGroup = (DreamCatcher), meta = (BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UDCCameraComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Camera")
	static UDCCameraComponent* FindCameraComponent(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Camera")
	float GetCurrentLookSensitivityMultiplier() const
	{
		return CurrentLookSensitivityMultiplier;
	}

	// 이 카메라가 추적할 Actor. 기본값은 Owner.
	virtual AActor* GetTargetActor() const
	{
		return GetOwner();
	}

	/**
	 * 매 프레임 현재 사용할 CameraMode 클래스를 요청.
	 *
	 * 4-3에서 Character 또는 Pawn 구성 요소가 연결.
	 */
	FDCCameraModeDelegate DetermineCameraModeDelegate;

	// 한 프레임 동안 적용할 추가 FOV.
	void AddFieldOfViewOffset(float FOVOffset)
	{
		FieldOfViewOffset += FOVOffset;
	}

	void GetBlendInfo(float& OutTopModeWeight, FGameplayTag& OutTopModeTag) const;

protected:
	virtual void OnRegister() override;

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	void UpdateCameraModes();

private:
	UPROPERTY()
	TObjectPtr<UDCCameraModeStack> CameraModeStack;

	float FieldOfViewOffset = 0.0f;
	
	float CurrentLookSensitivityMultiplier = 1.0f;
};
