#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCCombatComponent.generated.h"

// 발사, 회피, 궁극기 요청은 "신호"만 보냄.
// 실제 투사체 생성, 몽타주 재생, VFX 실행은 Character/BP 쪽에서 담당.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCCombatActionSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCUltimateChargeChangedSignature, float, NormalizedCharge);

/*
 * 이 컴포넌트는 전투 규칙을 담당.
 * 예:
 * - 연사 간격
 * - 회피 쿨다운
 * - 궁극기 게이지 축적/소모
 *
 * 반대로 아래는 여기서 하지 않음.
 * - 총알 Spawn
 * - 애니메이션 Montage 재생
 * - 사운드/VFX
 */
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

protected:
	virtual void BeginPlay() override;

	// 타이머를 쓰는 컴포넌트는 EndPlay에서 정리를 해두는 편이 안전.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 자동 연사 간격.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Fire", meta=(ClampMin="0.05", Units="s"))
	float FireInterval = 0.15f;

	// 회피 쿨다운.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Dodge", meta=(ClampMin="0.1", Units="s"))
	float DodgeCooldown = 0.8f;

	// 궁극기 최대 게이지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Ultimate", meta=(ClampMin="1.0"))
	float UltimateChargeMax = 100.0f;

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
};