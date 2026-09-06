#include "AbilitySystem/Attributes/DCHealthSet.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UDCHealthSet::UDCHealthSet() :
	bOutOfHealth(false), HealthBeforeAttributeChange(0.0f), MaxHealthBeforeAttributeChange(0.0f),
	Health(100.0f), MaxHealth(100.0f), Damage(0.0f), Healing(0.0f)

{
}

void UDCHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, Health, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UDCHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, Health, OldValue);

	const float PreviousHealth = OldValue.GetCurrentValue();
	const float CurrentHealth = GetHealth();
	const float ChangeMagnitude = CurrentHealth - PreviousHealth;

	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, ChangeMagnitude, PreviousHealth, CurrentHealth);

	if (!bOutOfHealth && CurrentHealth <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, ChangeMagnitude, PreviousHealth, CurrentHealth);
	}

	bOutOfHealth = CurrentHealth <= 0.0f;
}

void UDCHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, MaxHealth, OldValue);

	const float PreviousMaxHealth = OldValue.GetCurrentValue();
	const float CurrentMaxHealth = GetMaxHealth();
	const float ChangeMagnitude = CurrentMaxHealth - PreviousMaxHealth;

	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, ChangeMagnitude, PreviousMaxHealth, CurrentMaxHealth);
}

bool UDCHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// 들어오는 Damage Meta Attribute를 변경하려는데 대상에게 DamageImmunity가 있으면 Effect 실행을 차단.
	if (Data.EvaluatedData.Attribute == GetDamageAttribute() && Data.EvaluatedData.Magnitude > 0.0f && Data.Target.
		HasMatchingGameplayTag(DCGameplayTags::Gameplay_DamageImmunity))
	{
		Data.EvaluatedData.Magnitude = 0.0f;

		return false;
	}

	// Effect 적용 전 값을 저장.
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	return true;
}

void UDCHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UDCHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UDCHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최대 체력이 감소해서 현재 체력보다 작아졌다면 현재 체력도 새 최대 체력에 맞춤.
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		if (UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent())
		{
			AbilitySystemComponent->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	// 회복이나 초기화로 다시 체력이 생긴 경우 죽음 감지 상태를 해제.
	if (bOutOfHealth && GetHealth() > 0.0f)
	{
		bOutOfHealth = false;
	}
}

void UDCHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();

	AActor* EffectInstigator = EffectContext.GetOriginalInstigator();

	AActor* EffectCauser = EffectContext.GetEffectCauser();

	// Damage는 실제 상태가 아니라 이번 Effect가 전달한 임시 데미지 합계.
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float IncomingDamage = FMath::Max(GetDamage(), 0.0f);

		// Meta Attribute는 처리 후 반드시 0으로 돌림.
		SetDamage(0.0f);

		if (IncomingDamage > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - IncomingDamage, 0.0f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		const float IncomingHealing = FMath::Max(GetHealing(), 0.0f);

		SetHealing(0.0f);

		if (IncomingHealing > 0.0f && GetHealth() > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() + IncomingHealing, 0.0f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(EffectInstigator, EffectCauser, &Data.EffectSpec, Data.EvaluatedData.Magnitude,
		                             MaxHealthBeforeAttributeChange, GetMaxHealth());
	}

	// 실제 Health가 변경된 경우 HealthComponent에 알림.
	if (GetHealth() != HealthBeforeAttributeChange)
	{
		OnHealthChanged.Broadcast(EffectInstigator, EffectCauser, &Data.EffectSpec, Data.EvaluatedData.Magnitude,
		                          HealthBeforeAttributeChange, GetHealth());
	}

	// Health가 처음 0에 도달했을 때만 죽음 이벤트를 발생.
	if (GetHealth() <= 0.0f && !bOutOfHealth)
	{
		OnOutOfHealth.Broadcast(EffectInstigator, EffectCauser, &Data.EffectSpec, Data.EvaluatedData.Magnitude,
		                        HealthBeforeAttributeChange, GetHealth());
	}

	bOutOfHealth = GetHealth() <= 0.0f;
}

void UDCHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
