#include "Components/DCCombatComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UDCCombatComponent::UDCCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDCCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// HUD가 시작부터 정확한 궁극기 게이지를 알 수 있도록 초기값을 보냄.
	BroadcastUltimateCharge();
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
		World->GetTimerManager().SetTimer(
			PrimaryFireTimerHandle,
			this,
			&UDCCombatComponent::EmitPrimaryFire,
			FireInterval,
			true,
			FireInterval
		);
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
	// 실제 탄환 생성은 모르고, "지금 발사하라"는 요청만 보냄.
	OnPrimaryFireRequested.Broadcast();
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