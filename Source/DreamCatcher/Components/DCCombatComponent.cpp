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