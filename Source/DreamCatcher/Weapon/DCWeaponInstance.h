#pragma once

#include "CoreMinimal.h"
#include "Equipment/DCEquipmentInstance.h"
#include "DCWeaponInstance.generated.h"

class ADCWeaponActor;
class UAnimInstance;
class UCameraShakeBase;

// 모든 무기가 공유하는 장착 런타임 객체.
// 실제 무기 Actor를 찾고, 발사 시간과 연출 설정을 제공.
UCLASS(Abstract, BlueprintType, Blueprintable)
class DREAMCATCHER_API UDCWeaponInstance : public UDCEquipmentInstance
{
	GENERATED_BODY()

public:
	UDCWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnEquipped() override;

	// 이 장비가 생성한 Actor 중 실제 무기 Actor를 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	ADCWeaponActor* GetWeaponActor() const;

	// 마지막 발사 이후 경과 시간. 장착 후 아직 발사하지 않았다면 -1을 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	float GetTimeSinceLastFired() const;

	// 이후 발사 GameplayCue가 사용할 Camera Shake.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	TSubclassOf<UCameraShakeBase> GetFireCameraShakeClass() const
	{
		return FireCameraShakeClass;
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	float GetFireCameraShakeScale() const
	{
		return FireCameraShakeScale;
	}

	// 이후 장착 시 연결할 무기별 Linked Anim Layer.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	TSubclassOf<UAnimInstance> GetEquippedAnimLayerClass() const
	{
		return EquippedAnimLayerClass;
	}

protected:
	// 발사가 확정됐을 때 현재 월드 시간을 기록.
	void UpdateFiringTime();

	// -1은 이번 장착 이후 아직 발사하지 않았다는 의미.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Runtime")
	double LastFireTime = -1.0;

	// 아직 재생하지 않고 참조만 보관. 실제 재생은 6단계 GameplayCue에서 연결.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Feedback")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Feedback", meta = (ClampMin = "0.0"))
	float FireCameraShakeScale = 1.0f;

	// 현재는 None으로 두고 애니메이션 연결 단계에서 할당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Animation")
	TSubclassOf<UAnimInstance> EquippedAnimLayerClass;
};
