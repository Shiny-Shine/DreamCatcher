#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DCCameraMode.generated.h"

class AActor;
class UDCCameraComponent;

// CameraMode 전환에 사용할 Blend 함수.
UENUM(BlueprintType)
enum class EDCCameraModeBlendFunction : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut
};

// CameraMode 하나가 계산한 최종 카메라 정보.
struct FDCCameraModeView
{
	FDCCameraModeView();

	/**
	 * 현재 View에 Other View를 지정한 가중치만큼 혼합.
	 *
	 * OtherWeight:
	 *   0.0 → 현재 View 유지
	 *   1.0 → Other View로 완전히 교체
	 */
	void Blend(const FDCCameraModeView& Other, float OtherWeight);

	FVector Location;

	FRotator Rotation;

	FRotator ControlRotation;

	float FieldOfView;

	float LookSensitivityMultiplier;
};

/**
 * 모든 DreamCatcher CameraMode의 기본 클래스.
 *
 * CameraMode는 Actor가 아니라 CameraComponent가 소유하는 UObject.
 * 같은 CameraMode 클래스는 Stack 안에서 인스턴스 하나를 재사용.
 */
UCLASS(Abstract, NotBlueprintable)
class DREAMCATCHER_API UDCCameraMode : public UObject
{
	GENERATED_BODY()

public:
	UDCCameraMode();

	virtual UWorld* GetWorld() const override;

	UDCCameraComponent* GetDCCameraComponent() const;

	AActor* GetTargetActor() const;

	const FDCCameraModeView& GetCameraModeView() const
	{
		return View;
	}

	float GetBlendTime() const
	{
		return BlendTime;
	}

	float GetBlendWeight() const
	{
		return BlendWeight;
	}

	FGameplayTag GetCameraTypeTag() const
	{
		return CameraTypeTag;
	}

	void SetBlendWeight(float Weight);

	void UpdateCameraMode(float DeltaTime);

	// CameraMode가 Stack에 처음 추가될 때 호출.
	virtual void OnActivation()
	{
	}

	// CameraMode가 Stack에서 제거될 때 호출.
	virtual void OnDeactivation()
	{
	}

protected:
	// 카메라가 바라보는 기준 위치.
	virtual FVector GetPivotLocation() const;

	// 카메라의 기준 회전.
	virtual FRotator GetPivotRotation() const;

	// 매 프레임 CameraMode의 View를 계산.
	virtual void UpdateView(float DeltaTime);

	// BlendAlpha와 BlendWeight를 갱신.
	void UpdateBlending(float DeltaTime);

	// 이 CameraMode에서 사용할 Look 입력 감도 배율.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Input", meta = (ClampMin = "0.01"))
	float LookSensitivityMultiplier = 1.0f;

	/**
	 * 현재 CameraMode의 종류를 나타내는 태그.
	 *
	 * 이후 예:
	 * Camera.Type.Hip
	 * Camera.Type.Shoulder
	 * Camera.Type.Scope
	 */
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Blending")
	FGameplayTag CameraTypeTag;

	// 가로 FOV.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|View",
		meta = (ClampMin = "5.0", ClampMax = "170.0", UIMin = "5.0", UIMax = "170.0"))
	float FieldOfView = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|View", meta = (ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin = -89.9f;

	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|View", meta = (ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax = 89.9f;

	// 이 CameraMode가 완전히 적용되는 데 걸리는 시간.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Blending", meta = (ClampMin = "0.0", Units = "s"))
	float BlendTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Blending")
	EDCCameraModeBlendFunction BlendFunction =
		EDCCameraModeBlendFunction::EaseOut;

	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Camera|Blending", meta = (ClampMin = "1.0"))
	float BlendExponent = 4.0f;

	FDCCameraModeView View;

	float BlendAlpha = 1.0f;

	float BlendWeight = 1.0f;
};

/**
 * 여러 CameraMode를 보관하고 최종 View를 계산하는 Stack.
 *
 * 배열의 0번:
 *   가장 위에 있는 최신 CameraMode
 *
 * 배열의 마지막:
 *   Blend의 기반이 되는 가장 오래된 CameraMode
 */
UCLASS()
class DREAMCATCHER_API UDCCameraModeStack : public UObject
{
	GENERATED_BODY()

public:
	UDCCameraModeStack();

	void ActivateStack();

	void DeactivateStack();

	bool IsStackActive() const
	{
		return bIsActive;
	}

	// 지정한 CameraMode를 Stack 최상단으로 이동 또는 추가.
	void PushCameraMode(TSubclassOf<UDCCameraMode> CameraModeClass);

	// Stack을 갱신하고 최종 카메라 View를 반환.
	bool EvaluateStack(float DeltaTime, FDCCameraModeView& OutCameraModeView);

	// 현재 최상단 CameraMode의 Blend 정보.
	void GetBlendInfo(float& OutTopModeWeight, FGameplayTag& OutTopModeTag) const;

private:
	UDCCameraMode* GetCameraModeInstance(TSubclassOf<UDCCameraMode> CameraModeClass);

	void UpdateStack(float DeltaTime);

	void BlendStack(FDCCameraModeView& OutCameraModeView) const;

	bool bIsActive = true;

	// 클래스별로 생성된 CameraMode 인스턴스.
	UPROPERTY()
	TArray<TObjectPtr<UDCCameraMode>> CameraModeInstances;

	// 실제 Blend 순서를 나타내는 Stack.
	UPROPERTY()
	TArray<TObjectPtr<UDCCameraMode>> CameraModeStack;
};
