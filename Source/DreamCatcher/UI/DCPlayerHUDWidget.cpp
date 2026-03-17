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
		BoundCombatComponent->OnUltimateChargeChanged.AddDynamic(this, &UDCPlayerHUDWidget::HandleUltimateChargeChanged);
		HandleUltimateChargeChanged(BoundCombatComponent->GetUltimateChargeNormalized());
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
		BoundCombatComponent->OnUltimateChargeChanged.RemoveDynamic(this, &UDCPlayerHUDWidget::HandleUltimateChargeChanged);
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