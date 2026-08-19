#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/DCWeaponTypes.h"
#include "DCCombatComponent.generated.h"

/*
 * 이 컴포넌트는 전투 규칙을 담당.
 * 예:
 * - 연사 간격
 * - 회피 쿨다운
 * - 궁극기 게이지 축적/소모
 *
 * 아래는 여기서 하지 않음.
 * - 총알 Spawn
 * - 애니메이션 Montage 재생
 * - 사운드/VFX
 */

/*
 * 조준 상태를 정의, 리정희? 어딜만져!
 * Scope가 꺼진 상태
 * 긴 입력 → Shoulder
 * 해제 → Hip
 *	
 * Scope가 켜진 상태
 * 긴 입력 → Shoulder
 * 해제 → Scope
 */
UENUM(BlueprintType)
enum class EDCAimMode : uint8
{
	Hip,
	Shoulder,
	Scope
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCAimModeChangedSignature, EDCAimMode, NewAimMode);

// 발사, 회피, 궁극기 요청은 "신호"만 보냄.
// 실제 투사체 생성, 몽타주 재생, VFX 실행은 Character/BP 쪽에서 담당.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCCombatActionSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCUltimateChargeChangedSignature, float, NormalizedCharge);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCCrosshairSpreadChangedSignature, float, NormalizedSpread, float, SpreadDegrees);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCRecoilRequestedSignature, float, PitchKickDegrees, float, YawKickDegrees);

UCLASS(ClassGroup=(DreamCatcher), meta=(BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDCCombatComponent();

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FDCCombatActionSignature OnPrimaryFireRequested;

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FDCCombatActionSignature OnDodgeRequested;

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FDCCombatActionSignature OnUltimateRequested;

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FDCUltimateChargeChangedSignature OnUltimateChargeChanged;

	UFUNCTION(BlueprintCallable, Category="Combat")
	void StartPrimaryFire();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void StopPrimaryFire();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool TryDodge();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool TryUltimate();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void AddUltimateCharge(float Amount);

	UFUNCTION(BlueprintPure, Category="Combat")
	float GetUltimateChargeNormalized() const;
	
	// 조준 상태 정의
	UPROPERTY(BlueprintAssignable, Category="Combat|Aim")
	FDCAimModeChangedSignature OnAimModeChanged;

	UFUNCTION(BlueprintCallable, Category="Combat|Aim")
	void ToggleScopeAim();

	UFUNCTION(BlueprintCallable, Category="Combat|Aim")
	void BeginShoulderAim();

	UFUNCTION(BlueprintCallable, Category="Combat|Aim")
	void EndShoulderAim();

	UFUNCTION(BlueprintCallable, Category="Combat|Aim")
	void ClearAimState();

	UFUNCTION(BlueprintCallable, Category="Combat|Aim")
	void SetAimAllowed(bool bAllowed);

	UFUNCTION(BlueprintPure, Category="Combat|Aim")
	EDCAimMode GetAimMode() const { return CurrentAimMode; }

	UFUNCTION(BlueprintPure, Category="Combat|Aim")
	bool IsAimAllowed() const { return bAimAllowed; }
	
	UFUNCTION(BlueprintPure, Category="Combat|Fire")
	float GetPrimaryFireDamage() const { return PrimaryFireDamage; }
	
	UPROPERTY(BlueprintAssignable, Category="Combat|Crosshair")
	FDCCrosshairSpreadChangedSignature OnCrosshairSpreadChanged;

	UPROPERTY(BlueprintAssignable, Category="Combat|Recoil")
	FDCRecoilRequestedSignature OnRecoilRequested;

	UFUNCTION(BlueprintPure, Category="Combat|Accuracy")
	float GetCurrentSpreadDegrees() const
	{
		return CurrentSpreadDegrees;
	}

	UFUNCTION(BlueprintPure, Category="Combat|Accuracy")
	float GetCrosshairSpreadNormalized() const;

protected:
	virtual void BeginPlay() override;

	// 타이머를 쓰는 컴포넌트는 EndPlay에서 정리를 해두는 편이 안전.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 자동 연사 간격.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Fire", meta=(ClampMin="0.05", Units="s"))
	float FireInterval = 0.15f;
	
	// 플레이어 기본 사격 1발의 데미지 프로토타입 값.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Fire", meta=(ClampMin="0.0"))
	float PrimaryFireDamage = 10.0f;

	// 회피 쿨다운.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Dodge", meta=(ClampMin="0.1", Units="s"))
	float DodgeCooldown = 0.8f;

	// 궁극기 최대 게이지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Ultimate", meta=(ClampMin="1.0"))
	float UltimateChargeMax = 100.0f;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Weapon")
	FDCWeaponHandlingProfile WeaponHandlingProfile;

private:
	// 내부용 발사 요청 함수.
	// StartPrimaryFire()에서 즉시 1회 발사하고, 이후 타이머가 이 함수를 반복 호출.
	void EmitPrimaryFire();

	void ResetDodge();
	void BroadcastUltimateCharge();

	bool bIsHoldingPrimaryFire = false;
	bool bDodgeReady = true;
	float CurrentUltimateCharge = 0.0f;

	FTimerHandle PrimaryFireTimerHandle;
	FTimerHandle DodgeCooldownTimerHandle;
	
	//조준상태정의
	void RefreshAimMode();

	bool bScopeToggled = false;
	bool bShoulderHeld = false;
	bool bAimAllowed = true;

	EDCAimMode CurrentAimMode = EDCAimMode::Hip;
	
	void UpdateWeaponSpread(float DeltaTime);
	void BroadcastCrosshairSpread(bool bForce = false);

	float GetAimSpreadMultiplier() const;
	float GetAimRecoilMultiplier() const;

	float CurrentShotSpreadDegrees = 0.0f;
	float CurrentSpreadDegrees = 0.0f;
	float LastBroadcastSpreadNormalized = -1.0f;
};