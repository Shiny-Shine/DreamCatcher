#include "Components/DCHealthComponent.h"

UDCHealthComponent::UDCHealthComponent()
{
	// 체력 컴포넌트는 매 프레임 Tick이 불필요.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDCHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 현재 체력을 최대 체력으로 맞춤.
	CurrentHealth = MaxHealth;

	// 시작 직후 UI가 올바른 값을 그릴 수 있도록 한 번 Broadcast.
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UDCHealthComponent::SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);

	// 최대 체력을 바꾼 뒤 현재 체력을 어떻게 처리할지 선택할 수 있게 둠.
	CurrentHealth = bResetCurrentHealth
		? MaxHealth
		: FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

float UDCHealthComponent::ApplyDamage(float Damage, AActor* /*DamageCauser*/)
{
	// 죽은 상태이거나 0 이하 데미지는 무시.
	if (Damage <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	// 체력이 0이 되면 죽음 이벤트를 발생.
	if (IsDead())
	{
		OnDeath.Broadcast(GetOwner());
	}

	return Damage;
}

float UDCHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float Previous = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	return CurrentHealth - Previous;
}

void UDCHealthComponent::ResetToFull()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}