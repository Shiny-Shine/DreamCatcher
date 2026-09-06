#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Weapon/DCWeaponInstance.h"
#include "DCRangedWeaponInstance.generated.h"

// 원거리 무기의 설정과 탄 퍼짐 런타임 상태.
// 실제 발사 Ability와 Reticle은 GetCurrentSpreadAngle()이 반환하는 동일한 값을 사용.
UCLASS(BlueprintType, Blueprintable)
class DREAMCATCHER_API UDCRangedWeaponInstance : public UDCWeaponInstance
{
	GENERATED_BODY()

public:
	UDCRangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnEquipped() override;
	virtual void TickEquipment(float DeltaSeconds) override;

	// 실제 한 발이 확정된 뒤 호출. 현재 단계에서는 테스트 키로 호출하여 Heat 계산을 확인.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Weapon|Spread")
	void AddSpread();

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Fire")
	float GetFireInterval() const
	{
		return FMath::Max(FireInterval, 0.01f);
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Fire")
	float GetMaxDamageRange() const
	{
		return FMath::Max(MaxDamageRange, 1.0f);
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Fire")
	int32 GetBulletsPerCartridge() const
	{
		return FMath::Max(BulletsPerCartridge, 1);
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Fire")
	float GetBulletTraceSweepRadius() const
	{
		return FMath::Max(BulletTraceSweepRadius, 0.0f);
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Spread")
	float GetSpreadExponent() const
	{
		return FMath::Max(SpreadExponent, 0.1f);
	}

	// 상태 배율까지 적용된 최종 퍼짐각. 단위는 도, 원뿔의 반각이 아니라 전체 각도.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Spread")
	float GetCurrentSpreadAngle() const
	{
		return CurrentSpreadAngle;
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Spread")
	float GetCurrentHeat() const
	{
		return CurrentHeat;
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Spread")
	float GetCurrentSpreadMultiplier() const
	{
		return CurrentSpreadMultiplier;
	}

	// 거리 단위는 Unreal의 cm. Curve가 비어 있으면 거리 감쇠 없이 1배를 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon|Damage")
	float GetDistanceDamageMultiplier(float DistanceCm) const;

protected:
	// 초 단위 발사 간격입니다. 실제 연사는 6단계 Ability가 처리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Fire",
		meta = (ClampMin = "0.01", Units = "s"))
	float FireInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Fire",
		meta = (ClampMin = "1.0", Units = "cm"))
	float MaxDamageRange = 10000.0f;

	// 라이플은 1, 산탄총은 한 번에 여러 탄환을 사용할 수 있음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Fire", meta = (ClampMin = "1"))
	int32 BulletsPerCartridge = 1;

	// 0이면 Line Trace입니다. 실제 판정 연결은 6단계에서.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Fire",
		meta = (ClampMin = "0.0", Units = "cm"))
	float BulletTraceSweepRadius = 0.0f;

	// 이후 발사 방향 분포를 계산할 때 사용할 중심 집중도.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Spread", meta = (ClampMin = "0.1"))
	float SpreadExponent = 1.0f;

	// Heat는 연사에 따른 퍼짐 누적량. 이 값만으로 발사가 막히는 과열 시스템은 아님.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Spread", meta = (ClampMin = "0.01"))
	float MaxHeat = 10.0f;

	// X: 현재 Heat / Y: 상태 배율 적용 전 퍼짐 전체 각도.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Weapon|Spread")
	FRuntimeFloatCurve HeatToSpreadCurve;

	// X: 현재 Heat / Y: 한 발에 추가할 Heat.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Weapon|Spread")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	// X: 현재 Heat / Y: 초당 회복할 Heat.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Weapon|Spread")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	// 마지막 발사 후 이 시간이 지나야 Heat 회복 시작.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Spread",
		meta = (ClampMin = "0.0", Units = "s"))
	float SpreadRecoveryCooldownDelay = 0.2f;

	// 최대 이동 속도일 때의 배율. 정지 시에는 1배.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Multipliers",
		meta = (ClampMin = "0.0"))
	float MovingSpreadMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Multipliers",
		meta = (ClampMin = "0.0"))
	float AirborneSpreadMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Multipliers",
		meta = (ClampMin = "0.0"))
	float ShoulderSpreadMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Multipliers",
		meta = (ClampMin = "0.0"))
	float ScopeSpreadMultiplier = 0.15f;

	// 상태 배율 전환 속도입니다. 0이면 즉시 적용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Multipliers",
		meta = (ClampMin = "0.0"))
	float MultiplierInterpSpeed = 12.0f;

	// X: 거리(cm) / Y: 데미지 배율.
	UPROPERTY(EditDefaultsOnly, Category = "DreamCatcher|Weapon|Damage")
	FRuntimeFloatCurve DistanceDamageFalloff;

private:
	// 현재 이동·공중·조준 상태로 목표 배율을 계산.
	float CalculateTargetSpreadMultiplier() const;

	// Heat와 현재 배율에서 최종 퍼짐각을 다시 계산.
	void RefreshSpreadAngle();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Runtime",
		meta = (AllowPrivateAccess = "true"))
	float CurrentHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Runtime",
		meta = (AllowPrivateAccess = "true"))
	float CurrentSpreadMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Runtime",
		meta = (AllowPrivateAccess = "true"))
	float CurrentSpreadAngle = 0.0f;
};
