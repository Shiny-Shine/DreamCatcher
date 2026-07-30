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

		// HUD가 생성된 직후에도 현재 궁극기 게이지를 표시.
		HandleUltimateChargeChanged(BoundCombatComponent->GetUltimateChargeNormalized());

		// 조준 모드가 변경됐을 때 HUD가 알림을 받도록 등록.
		BoundCombatComponent->OnAimModeChanged.AddDynamic(this, &UDCPlayerHUDWidget::HandleAimModeChanged);

		// HUD가 생성된 직후에도 현재 조준 모드에 맞는 UI를 표시.
		HandleAimModeChanged(BoundCombatComponent->GetAimMode());
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
		BoundCombatComponent->OnUltimateChargeChanged.RemoveDynamic(
			this, &UDCPlayerHUDWidget::HandleUltimateChargeChanged);
		// 조준 모드 변경 알림 등록 해제.
		BoundCombatComponent->OnAimModeChanged.RemoveDynamic(this, &UDCPlayerHUDWidget::HandleAimModeChanged);

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