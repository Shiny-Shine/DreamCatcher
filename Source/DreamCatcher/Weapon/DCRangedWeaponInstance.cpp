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

void UDCRangedWeaponInstance::RefreshSpreadAngle()
{
	const float BaseSpreadAngle = FMath::Max(HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat, 2.0f), 0.0f);

	// 최종 퍼짐각을 한곳에서 계산.
	// 반각이 90도에 도달하지 않도록 전체 각도를 제한.
	CurrentSpreadAngle = FMath::Clamp(BaseSpreadAngle * CurrentSpreadMultiplier, 0.0f, 179.0f);
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
