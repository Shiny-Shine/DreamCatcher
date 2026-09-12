#include "Weapon/DCRangedWeaponInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

UDCRangedWeaponInstance::UDCRangedWeaponInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// 초기 테스트 곡선: Heat 0에서는 2도, Heat 10에서는 8도. 모두 원뿔의 전체 각도.
	const FKeyHandle StartKey = HeatToSpreadCurve.EditorCurveData.AddKey(0.0f, 2.0f);

	const FKeyHandle EndKey = HeatToSpreadCurve.EditorCurveData.AddKey(10.0f, 8.0f);

	HeatToSpreadCurve.EditorCurveData.SetKeyInterpMode(StartKey, RCIM_Linear);

	HeatToSpreadCurve.EditorCurveData.SetKeyInterpMode(EndKey, RCIM_Linear);

	// 키 하나인 곡선은 모든 Heat에서 같은 값을 반환.
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 4.0f);
}

void UDCRangedWeaponInstance::OnEquipped()
{
	if (IsEquipped())
	{
		return;
	}

	// 이번 장착의 초기 상태.
	CurrentHeat = 0.0f;
	CurrentSpreadMultiplier = CalculateTargetSpreadMultiplier();

	RefreshSpreadAngle();

	// 모든 초기값이 준비된 뒤 Blueprint 장착 이벤트를 실행.
	Super::OnEquipped();
	// Super 호출 후 실제 장착 상태가 되었으므로 정확도 조건도 갱신.
	RefreshSpreadAngle();
}

void UDCRangedWeaponInstance::TickEquipment(float DeltaSeconds)
{
	Super::TickEquipment(DeltaSeconds);

	if (!IsEquipped() || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	float RecoverySeconds = DeltaSeconds;

	if (LastFireTime >= 0.0)
	{
		const double RecoveryStartTime = LastFireTime + FMath::Max(SpreadRecoveryCooldownDelay, 0.0f);

		// 이번 프레임 중 회복 지연이 끝난 이후의 시간만 사용.
		RecoverySeconds = FMath::Clamp(static_cast<float>(World->GetTimeSeconds() - RecoveryStartTime), 0.0f,
		                               DeltaSeconds);
	}

	if (RecoverySeconds > 0.0f)
	{
		const float CooldownRate = FMath::Max(HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat, 4.0f),
		                                      0.0f);

		CurrentHeat = FMath::Clamp(CurrentHeat - CooldownRate * RecoverySeconds, 0.0f, FMath::Max(MaxHeat, 0.01f));
	}

	// 조준이나 이동 상태가 바뀔 때 퍼짐 배율을 부드럽게 변경.
	CurrentSpreadMultiplier = FMath::FInterpTo(CurrentSpreadMultiplier, CalculateTargetSpreadMultiplier(), DeltaSeconds,
	                                           MultiplierInterpSpeed);

	RefreshSpreadAngle();
}

void UDCRangedWeaponInstance::AddSpread()
{
	if (!IsEquipped() || !GetWorld())
	{
		return;
	}

	const float HeatPerShot = FMath::Max(HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat, 1.0f), 0.0f);

	CurrentHeat = FMath::Clamp(CurrentHeat + HeatPerShot, 0.0f, FMath::Max(MaxHeat, 0.01f));

	// 매 발마다 갱신해야 회복 지연이 마지막 발사 시점부터 계산됨.
	UpdateFiringTime();

	// 다음 Tick까지 기다리지 않고 발사 직후 퍼짐을 갱신.
	RefreshSpreadAngle();
}

float UDCRangedWeaponInstance::CalculateTargetSpreadMultiplier() const
{
	APawn* Pawn = GetPawn();

	if (!IsValid(Pawn))
	{
		return 1.0f;
	}

	float MovementMultiplier = 1.0f;
	float AirMultiplier = 1.0f;
	float AimMultiplier = 1.0f;

	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

		if (Movement)
		{
			// 점프의 수직 속도는 이동 배율에 중복 반영 X.
			const float SpeedRatio = FMath::Clamp(
				Pawn->GetVelocity().Size2D() / FMath::Max(Movement->GetMaxSpeed(), 1.0f), 0.0f, 1.0f);

			MovementMultiplier = FMath::Lerp(1.0f, MovingSpreadMultiplier, SpeedRatio);

			if (Movement->IsFalling())
			{
				AirMultiplier = AirborneSpreadMultiplier;
			}
		}
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);

	if (ASC)
	{
		if (ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope))
		{
			AimMultiplier = ScopeSpreadMultiplier;
		}
		else if (ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Shoulder))
		{
			AimMultiplier = ShoulderSpreadMultiplier;
		}
	}

	return FMath::Max(MovementMultiplier * AirMultiplier * AimMultiplier, 0.0f);
}

bool UDCRangedWeaponInstance::CanUseFirstShotAccuracy(float BaseSpreadAngle) const
{
	if (!bAllowFirstShotAccuracy || !IsEquipped())
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(GetPawn());
	const UWorld* World = GetWorld();

	if (!IsValid(Character) || !World)
	{
		return false;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	// 지상 이동 상태에서만 허용. 점프 입력을 새로 구현하는 것이 아니라 낙하 등의 상태를 검사.
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}

	const float MaxAllowedSpeed = FMath::Max(FirstShotAccuracyMaxSpeed, 0.0f);

	if (Character->GetVelocity().Size2D() > MaxAllowedSpeed)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);

	if (!IsValid(ASC))
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(
		DCGameplayTags::State_Dodging))
	{
		return false;
	}

	const bool bScope = ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope);

	const bool bShoulder = ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Shoulder);

	if (bFirstShotAccuracyRequiresAim && !bScope && !bShoulder)
	{
		return false;
	}

	// 발사 직후 바로 다시 정확도가 활성화되지 않도록, 기존 퍼짐 회복 지연이 끝났는지도 확인.
	if (LastFireTime >= 0.0)
	{
		const double TimeSinceFired = World->GetTimeSeconds() - LastFireTime;

		if (TimeSinceFired <= FMath::Max(SpreadRecoveryCooldownDelay, 0.0f))
		{
			return false;
		}
	}

	const FRichCurve* SpreadCurve = HeatToSpreadCurve.GetRichCurveConst();

	if (!SpreadCurve || !SpreadCurve->HasAnyData())
	{
		return false;
	}

	float MinCurveSpread = 0.0f;
	float MaxCurveSpread = 0.0f;
	SpreadCurve->GetValueRange(MinCurveSpread, MaxCurveSpread);

	// 현재 기본 퍼짐이 곡선의 최소 퍼짐까지 회복되었는지 검사.
	if (!FMath::IsNearlyEqual(BaseSpreadAngle, FMath::Max(MinCurveSpread, 0.0f),KINDA_SMALL_NUMBER))
	{
		return false;
	}

	// 현재 조준 모드에서 정지했을 때의 목표 배율.
	float StationaryAimMultiplier = 1.0f;

	if (bScope)
	{
		StationaryAimMultiplier = ScopeSpreadMultiplier;
	}
	else if (bShoulder)
	{
		StationaryAimMultiplier = ShoulderSpreadMultiplier;
	}

	StationaryAimMultiplier = FMath::Max(StationaryAimMultiplier, 0.0f);

	// 이동/조준 전환 중 남아 있는 배율이 충분히 안정됐는지 검사.
	constexpr float MultiplierTolerance = 0.01f;

	return FMath::IsNearlyEqual(CurrentSpreadMultiplier, StationaryAimMultiplier, MultiplierTolerance);
}

void UDCRangedWeaponInstance::RefreshSpreadAngle()
{
	const float BaseSpreadAngle = FMath::Max(HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat, 2.0f), 0.0f);

	bHasFirstShotAccuracy = CanUseFirstShotAccuracy(BaseSpreadAngle);

	const float EffectiveMultiplier = bHasFirstShotAccuracy ? 0.0f : CurrentSpreadMultiplier;

	CurrentSpreadAngle = FMath::Clamp(BaseSpreadAngle * EffectiveMultiplier, 0.0f, 179.0f);
}

float UDCRangedWeaponInstance::GetDistanceDamageMultiplier(float DistanceCm) const
{
	const FRichCurve* Curve = DistanceDamageFalloff.GetRichCurveConst();

	if (!Curve->HasAnyData())
	{
		return 1.0f;
	}

	return FMath::Max(Curve->Eval(FMath::Max(DistanceCm, 0.0f)), 0.0f);
}
