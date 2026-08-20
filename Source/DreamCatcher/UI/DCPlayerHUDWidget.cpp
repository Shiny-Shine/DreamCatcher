#include "UI/DCPlayerHUDWidget.h"
#include "DreamCatcherCharacter.h"
#include "Components/DCCombatComponent.h"
#include "Components/DCHealthComponent.h"

void UDCPlayerHUDWidget::BindToCharacter(ADreamCatcherCharacter* NewCharacter)
{
	UnbindFromCurrentCharacter();

	ObservedCharacter = NewCharacter;
	BP_OnPlayerBound(NewCharacter);

	if (!NewCharacter)
	{
		BP_OnHealthChanged(0.0f, 1.0f);
		BP_OnUltimateChargeChanged(0.0f);
		return;
	}

	BoundHealthComponent = NewCharacter->GetHealthComponent();
	BoundCombatComponent = NewCharacter->GetCombatComponent();

	if (BoundHealthComponent)
	{
		BoundHealthComponent->OnHealthChanged.AddDynamic(this, &UDCPlayerHUDWidget::HandleHealthChanged);
		HandleHealthChanged(BoundHealthComponent->GetCurrentHealth(), BoundHealthComponent->GetMaxHealth());
	}

	if (BoundCombatComponent)
	{
		// 궁극기 게이지가 변경됐을 때 HUD가 알림을 받도록 등록.
		BoundCombatComponent->OnUltimateChargeChanged.AddDynamic(this, &UDCPlayerHUDWidget::HandleUltimateChargeChanged);
		// 조준 모드가 변경됐을 때 HUD가 알림을 받도록 등록.
		BoundCombatComponent->OnAimModeChanged.AddDynamic(this, &UDCPlayerHUDWidget::HandleAimModeChanged);
		// HUD 퍼짐값 전달
		BoundCombatComponent->OnCrosshairSpreadChanged.AddDynamic(this,&UDCPlayerHUDWidget::HandleCrosshairSpreadChanged);
		// 매 발사 순간 HUD가 알림을 받음.
		BoundCombatComponent->OnShotFired.AddDynamic(this, &UDCPlayerHUDWidget::HandleShotFired);

		// HUD가 생성된 직후에도 현재 조준 모드에 맞는 UI를 표시.
		HandleAimModeChanged(BoundCombatComponent->GetAimMode());
		// HUD가 생성된 직후에도 현재 궁극기 게이지를 표시.
		HandleUltimateChargeChanged(BoundCombatComponent->GetUltimateChargeNormalized());

		HandleCrosshairSpreadChanged(BoundCombatComponent->GetCrosshairSpreadNormalized(), BoundCombatComponent->GetCurrentSpreadDegrees());
	}
}

void UDCPlayerHUDWidget::NativeDestruct()
{
	UnbindFromCurrentCharacter();
	Super::NativeDestruct();
}

void UDCPlayerHUDWidget::UnbindFromCurrentCharacter()
{
	if (BoundHealthComponent)
	{
		BoundHealthComponent->OnHealthChanged.RemoveDynamic(this, &UDCPlayerHUDWidget::HandleHealthChanged);
		BoundHealthComponent = nullptr;
	}

	if (BoundCombatComponent)
	{
		// 궁극기 게이지 변경 알림 등록 해제.
		BoundCombatComponent->OnUltimateChargeChanged.RemoveDynamic(this, &UDCPlayerHUDWidget::HandleUltimateChargeChanged);
		// 조준 모드 변경 알림 등록 해제.
		BoundCombatComponent->OnAimModeChanged.RemoveDynamic(this, &UDCPlayerHUDWidget::HandleAimModeChanged);
		
		BoundCombatComponent->OnCrosshairSpreadChanged.RemoveDynamic(this,&UDCPlayerHUDWidget::HandleCrosshairSpreadChanged);
		
		BoundCombatComponent->OnShotFired.RemoveDynamic(this,&UDCPlayerHUDWidget::HandleShotFired);

		BoundCombatComponent = nullptr;
	}

	ObservedCharacter = nullptr;
}

void UDCPlayerHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	BP_OnHealthChanged(CurrentHealth, MaxHealth);
}

void UDCPlayerHUDWidget::HandleUltimateChargeChanged(float NormalizedCharge)
{
	BP_OnUltimateChargeChanged(NormalizedCharge);
}

void UDCPlayerHUDWidget::HandleAimModeChanged(EDCAimMode NewAimMode)
{
	// C++에서 받은 조준 모드를 WBP_PlayerHUD에 전달.
	BP_OnAimModeChanged(NewAimMode);
}

void UDCPlayerHUDWidget::HandleCrosshairSpreadChanged(float NormalizedSpread, float SpreadDegrees)
{
	BP_OnCrosshairSpreadChanged(NormalizedSpread, SpreadDegrees);
}


// ShotSpreadDegrees = 이번 탄이 실제로 사용할 퍼짐 각도
// PitchKickDegrees = 이번 사격의 상하 카메라 반동
// YawKickDegrees = 이번 사격의 좌우 카메라 반동
void UDCPlayerHUDWidget::HandleShotFired(float ShotSpreadDegrees, float PitchKickDegrees, float YawKickDegrees)
{
	BP_OnShotFired(ShotSpreadDegrees, PitchKickDegrees, YawKickDegrees);
}