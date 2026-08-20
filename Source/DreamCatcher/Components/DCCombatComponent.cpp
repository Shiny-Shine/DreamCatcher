#include "Components/DCCombatComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDCCombatComponent::UDCCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDCCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// HUD가 시작부터 정확한 궁극기 게이지를 알 수 있도록 초기값을 보냄.
	BroadcastUltimateCharge();

	CurrentSpreadDegrees = WeaponHandlingProfile.BaseSpreadDegrees * GetAimSpreadMultiplier();

	BroadcastCrosshairSpread(true);
}

void UDCCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 액터 파괴/레벨 전환 시 타이머를 정리.
	// 작은 프로젝트일수록 이런 정리를 습관화하는 편이 디버깅에 좋다고 함.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrimaryFireTimerHandle);
		World->GetTimerManager().ClearTimer(DodgeCooldownTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UDCCombatComponent::StartPrimaryFire()
{
	// 이미 발사 중이면 중복 타이머를 만들지 않음.
	if (bIsHoldingPrimaryFire)
	{
		return;
	}

	bIsHoldingPrimaryFire = true;

	// 버튼을 누르는 즉시 한 발 나가게 해서 조작감 향상.
	EmitPrimaryFire();

	// 이후에는 일정 간격으로 자동 연사.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PrimaryFireTimerHandle, this, &UDCCombatComponent::EmitPrimaryFire, FireInterval, true, FireInterval);
	}
}

void UDCCombatComponent::StopPrimaryFire()
{
	bIsHoldingPrimaryFire = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrimaryFireTimerHandle);
	}
}

bool UDCCombatComponent::TryDodge()
{
	// 쿨다운이 남아 있으면 회피를 막음.
	if (!bDodgeReady)
	{
		return false;
	}
	ClearAimState();

	bDodgeReady = false;
	OnDodgeRequested.Broadcast();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DodgeCooldownTimerHandle,
			this,
			&UDCCombatComponent::ResetDodge,
			DodgeCooldown,
			false
		);
	}

	return true;
}

bool UDCCombatComponent::TryUltimate()
{
	// 게이지가 다 차지 않았으면 궁극기 사용을 막음.
	if (CurrentUltimateCharge < UltimateChargeMax)
	{
		return false;
	}

	ClearAimState();

	CurrentUltimateCharge = 0.0f;
	BroadcastUltimateCharge();
	OnUltimateRequested.Broadcast();

	return true;
}

void UDCCombatComponent::AddUltimateCharge(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentUltimateCharge = FMath::Clamp(CurrentUltimateCharge + Amount, 0.0f, UltimateChargeMax);
	BroadcastUltimateCharge();
}

float UDCCombatComponent::GetUltimateChargeNormalized() const
{
	return CurrentUltimateCharge / FMath::Max(1.0f, UltimateChargeMax);
}

void UDCCombatComponent::EmitPrimaryFire()
{
	// 이번 발이 실제로 사용할 탄 퍼짐, 이번 발의 Bloom이 추가되기 전 값을 저장.
	const float ShotSpreadDegrees = CurrentSpreadDegrees;

	const float RecoilMultiplier = GetAimRecoilMultiplier();

	const float PitchKickDegrees = FMath::FRandRange(
		WeaponHandlingProfile.RecoilPitchMin,
		WeaponHandlingProfile.RecoilPitchMax
	) * RecoilMultiplier;

	const float YawKickDegrees = FMath::FRandRange(
		-WeaponHandlingProfile.RecoilYawMax,
		WeaponHandlingProfile.RecoilYawMax
	) * RecoilMultiplier;

	// 실제 사격, 카메라 반동, HUD 반동에 같은 값을 전달합니다.
	OnShotFired.Broadcast(
		ShotSpreadDegrees,
		PitchKickDegrees,
		YawKickDegrees
	);

	// 이번 발을 발사한 뒤 다음 탄을 위한 Bloom을 누적합니다.
	CurrentShotSpreadDegrees = FMath::Clamp(
		CurrentShotSpreadDegrees
			+ WeaponHandlingProfile.SpreadPerShotDegrees,
		0.0f,
		WeaponHandlingProfile.MaxShotSpreadDegrees
	);
}

void UDCCombatComponent::ResetDodge()
{
	bDodgeReady = true;
}

void UDCCombatComponent::BroadcastUltimateCharge()
{
	OnUltimateChargeChanged.Broadcast(GetUltimateChargeNormalized());
}

//조준 상태 정의
void UDCCombatComponent::ToggleScopeAim()
{
	if (!bAimAllowed)
	{
		return;
	}

	// Scope 토글은 Hip과 Scope 사이에서만 허용, Shoulder 상태에서는 전환하지 않음.
	if (CurrentAimMode == EDCAimMode::Hip)
	{
		bShoulderHeld = false;
		bScopeToggled = true;
	}
	else if (CurrentAimMode == EDCAimMode::Scope)
	{
		bShoulderHeld = false;
		bScopeToggled = false;
	}
	else
	{
		return;
	}

	RefreshAimMode();
}

void UDCCombatComponent::BeginShoulderAim()
{
	// 조준이 금지됐거나 Hip 상태가 아니면 Shoulder에 진입하지 않음.
	if (!bAimAllowed || CurrentAimMode != EDCAimMode::Hip)
	{
		return;
	}

	// Shoulder와 Scope 상태가 동시에 저장되지 않도록 확실하게 해제.
	bScopeToggled = false;
	bShoulderHeld = true;

	RefreshAimMode();
}

void UDCCombatComponent::EndShoulderAim()
{
	bShoulderHeld = false;
	RefreshAimMode();
}

void UDCCombatComponent::ClearAimState()
{
	bScopeToggled = false;
	bShoulderHeld = false;
	RefreshAimMode();
}

void UDCCombatComponent::SetAimAllowed(bool bAllowed)
{
	bAimAllowed = bAllowed;

	if (!bAimAllowed)
	{
		ClearAimState();
	}
}

void UDCCombatComponent::RefreshAimMode()
{
	EDCAimMode NewAimMode = EDCAimMode::Hip;

	if (bShoulderHeld)
	{
		NewAimMode = EDCAimMode::Shoulder;
	}
	else if (bScopeToggled)
	{
		NewAimMode = EDCAimMode::Scope;
	}

	if (CurrentAimMode == NewAimMode)
	{
		return;
	}

	CurrentAimMode = NewAimMode;
	OnAimModeChanged.Broadcast(CurrentAimMode);
}

// 탄퍼짐 계산
void UDCCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateWeaponSpread(DeltaTime);
}

// 탄퍼짐 전체 계산
void UDCCombatComponent::UpdateWeaponSpread(float DeltaTime)
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();

	if (!Movement)
	{
		return;
	}

	// 수직 속도는 제외하고 평면 이동 속도만 사용합니다.
	const float HorizontalSpeed =
		OwnerCharacter->GetVelocity().Size2D();

	const float MaximumSpeed = FMath::Max(Movement->GetMaxSpeed(), 1.0f);

	const float MovementAlpha = FMath::Clamp(HorizontalSpeed / MaximumSpeed, 0.0f, 1.0f);

	// 먼저 연사 누적 퍼짐을 회복시킵니다.
	CurrentShotSpreadDegrees = FMath::FInterpConstantTo(CurrentShotSpreadDegrees, 0.0f, DeltaTime, WeaponHandlingProfile.SpreadRecoveryPerSecond);

	float TargetSpread = WeaponHandlingProfile.BaseSpreadDegrees + WeaponHandlingProfile.MovementSpreadDegrees * MovementAlpha + CurrentShotSpreadDegrees;

	if (Movement->IsFalling())
	{
		TargetSpread += WeaponHandlingProfile.AirborneSpreadDegrees;
	}

	TargetSpread *= GetAimSpreadMultiplier();

	CurrentSpreadDegrees = FMath::FInterpTo(CurrentSpreadDegrees, TargetSpread, DeltaTime, WeaponHandlingProfile.SpreadInterpSpeed);

	BroadcastCrosshairSpread();
}

// 실제 탄퍼짐
float UDCCombatComponent::GetAimSpreadMultiplier() const
{
	switch (CurrentAimMode)
	{
	case EDCAimMode::Shoulder:
		return WeaponHandlingProfile.ShoulderSpreadMultiplier;

	case EDCAimMode::Scope:
		return WeaponHandlingProfile.ScopeSpreadMultiplier;

	case EDCAimMode::Hip:
	default:
		return 1.0f;
	}
}

// 카메라 반동
float UDCCombatComponent::GetAimRecoilMultiplier() const
{
	switch (CurrentAimMode)
	{
	case EDCAimMode::Shoulder:
		return WeaponHandlingProfile.ShoulderRecoilMultiplier;

	case EDCAimMode::Scope:
		return WeaponHandlingProfile.ScopeRecoilMultiplier;

	case EDCAimMode::Hip:
	default:
		return 1.0f;
	}
}

// 크로스헤어 정규화 값
float UDCCombatComponent::GetCrosshairSpreadNormalized() const
{
	const float FullScaleSpread = FMath::Max(
		WeaponHandlingProfile.CrosshairFullScaleSpreadDegrees,
		0.1f
	);

	return FMath::Clamp(
		CurrentSpreadDegrees / FullScaleSpread,
		0.0f,
		1.0f
	);
}

void UDCCombatComponent::BroadcastCrosshairSpread(bool bForce)
{
	const float NormalizedSpread = GetCrosshairSpreadNormalized();

	if (!bForce && FMath::IsNearlyEqual(NormalizedSpread,LastBroadcastSpreadNormalized,0.001f))
	{
		return;
	}

	LastBroadcastSpreadNormalized = NormalizedSpread;

	OnCrosshairSpreadChanged.Broadcast(NormalizedSpread,CurrentSpreadDegrees);
}