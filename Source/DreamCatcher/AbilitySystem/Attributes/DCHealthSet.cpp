// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCHealthSet.h"
#include "AbilitySystem/Attributes/DCAttributeSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCAbilitySourceInterface.h"
#include "AbilitySystem/DCGameplayEffectContext.h"
#include "Engine/World.h"
#include "GameplayEffectExtension.h"
#include "Messages/DCVerbMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Character/DCLyraHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCHealthSet)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Damage, "Gameplay.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageSelfDestruct, "Gameplay.Damage.SelfDestruct");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_FellOutOfWorld, "Gameplay.Damage.FellOutOfWorld");
UE_DEFINE_GAMEPLAY_TAG(TAG_Lyra_Damage_Message, "Lyra.Damage.Message");

UDCHealthSet::UDCHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Armor(0.0f)
	, MaxArmor(0.0f)
{
	bOutOfHealth = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
}

void UDCHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDCHealthSet, MaxArmor, COND_None, REPNOTIFY_Always);
}

void UDCHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, Health, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	// These events on the client should not be changing attributes

	const float CurrentHealth = GetHealth();
	const float EstimatedMagnitude = CurrentHealth - OldValue.GetCurrentValue();

	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);

	if (!bOutOfHealth && CurrentHealth <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);
	}

	bOutOfHealth = (CurrentHealth <= 0.0f);
}

void UDCHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, MaxHealth, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

void UDCHealthSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	{
		TGuardValue<bool> Replication(bHandlingArmorReplication, true);
		TGuardValue<bool> Defer(bDeferringArmorEvents, true);
		GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, Armor, OldValue);
	}
	OnArmorChanged.Broadcast(nullptr, nullptr, nullptr, GetArmor() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetArmor());
}

void UDCHealthSet::OnRep_MaxArmor(const FGameplayAttributeData& OldValue)
{
	{
		TGuardValue<bool> Replication(bHandlingArmorReplication, true);
		TGuardValue<bool> Defer(bDeferringArmorEvents, true);
		GAMEPLAYATTRIBUTE_REPNOTIFY(UDCHealthSet, MaxArmor, OldValue);
	}
	OnMaxArmorChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxArmor() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxArmor());
}

bool UDCHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// Handle modifying incoming normal damage
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			const UDCLyraHealthComponent* TargetHealth = UDCLyraHealthComponent::FindHealthComponent(Data.Target.GetAvatarActor());
			if (TargetHealth && TargetHealth->ShouldRejectPostDeathDamage())
			{
				// Reject before meta/resource changes and damage messages. Original weapon
				// GEs require a successful modifier before executing their damage Cue.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}

			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);

			if (Data.Target.HasMatchingGameplayTag(DCGameplayTags::Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any health.
				const float BlockedDamage = Data.EvaluatedData.Magnitude;
				Data.EvaluatedData.Magnitude = 0.0f;
				OnDamageBlocked.Broadcast(Data.EffectSpec, BlockedDamage);
				return false;
			}

#if !UE_BUILD_SHIPPING
			// Check GodMode cheat, unlimited health is checked below
			if (Data.Target.HasMatchingGameplayTag(DCGameplayTags::Cheat_GodMode) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any health.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
#endif // #if !UE_BUILD_SHIPPING
		}
	}

	// Save the current health
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	return true;
}

void UDCHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	// Reentrant GE applications must not overwrite this execution's old values.
	const float OldHealth = HealthBeforeAttributeChange;
	const float OldMaxHealth = MaxHealthBeforeAttributeChange;
	const float OldArmor = GetArmor();
	bool bArmorChangedByThisExecution = false;

	const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);
	float MinimumHealth = 0.0f;

#if !UE_BUILD_SHIPPING
	// Godmode and unlimited health stop death unless it's a self destruct
	if (!bIsDamageFromSelfDestruct &&
		(Data.Target.HasMatchingGameplayTag(DCGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(DCGameplayTags::Cheat_UnlimitedHealth) ))
	{
		MinimumHealth = 1.0f;
	}
#endif // #if !UE_BUILD_SHIPPING

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float RawDamage = FMath::IsFinite(GetDamage()) ? GetDamage() : 0.f;
		float HealthDamage = RawDamage;
		float NewArmor = OldArmor;
		if (RawDamage > 0.f && OldArmor > 0.f && !bIsDamageFromSelfDestruct)
		{
			float Ratio = .8f;
			if (const FDCGameplayEffectContext* Context = FDCGameplayEffectContext::ExtractEffectContext(EffectContext))
			{
				if (const IDCAbilitySourceInterface* Source = Context->GetAbilitySource()) { Ratio = Source->GetDamageToArmorRatio(); }
			}
			if (!FMath::IsFinite(Ratio) || Ratio < 0.f || Ratio > 1.f) { Ratio = .8f; }
			NewArmor = OldArmor - FMath::Min(OldArmor, RawDamage * Ratio);
			HealthDamage = RawDamage * (1.f - Ratio); // The unfulfilled armor share is discarded.
		}
		// Clear the single incoming meta value before any resource setter or observer
		// can apply another GE. No second ArmorDamage modifier/channel is involved.
		SetDamage(0.0f);
		{
			TGuardValue<bool> Defer(bDeferringArmorEvents, true);
			if (NewArmor != OldArmor) { SetArmor(NewArmor); }
			SetHealth(FMath::Clamp(GetHealth() - HealthDamage, MinimumHealth, GetMaxHealth()));
		}
		bArmorChangedByThisExecution = GetArmor() != OldArmor;
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		const float IncomingHealing = GetHealing();
		SetHealing(0.0f);
		SetHealth(FMath::Clamp(GetHealth() + IncomingHealing, MinimumHealth, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Clamp and fall into out of health handling below
		SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
	}
	const bool bHealthChangedByThisExecution = GetHealth() != OldHealth;
	// The first actual HP-zero execution reserves its own cause before any raw
	// message can apply another GE. A later heal/death cycle invalidates this token.
	uint64 ReservedOutOfHealth = 0;
	if (GetHealth() <= 0.f && !bOutOfHealth)
	{
		bOutOfHealth = true;
		ReservedOutOfHealth = ++OutOfHealthNotificationSerial;
	}
	// Preserve the original raw message payload and its order before health/death
	// notifications, while observers now see both committed resources and empty meta.
	if (Data.EvaluatedData.Attribute == GetDamageAttribute() && Data.EvaluatedData.Magnitude > 0.0f)
	{
		FDCVerbMessage Message;
		Message.Verb = TAG_Lyra_Damage_Message;
		Message.Instigator = EffectContext.GetEffectCauser();
		Message.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
		Message.Target = GetOwningActor();
		Message.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
		Message.Magnitude = Data.EvaluatedData.Magnitude;
		UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(Message.Verb, Message);
	}
	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldMaxHealth, GetMaxHealth());
	}

	// If health has actually changed activate callbacks
	if (bHealthChangedByThisExecution)
	{
		OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldHealth, GetHealth());
	}

	if (ReservedOutOfHealth != 0 && ReservedOutOfHealth == OutOfHealthNotificationSerial && GetHealth() <= 0.f)
	{
		OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldHealth, GetHealth());
	}

	// Check health again in case an event above changed it.
	bOutOfHealth = (GetHealth() <= 0.0f);
	if (bArmorChangedByThisExecution)
	{
		OnArmorChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldArmor, GetArmor());
	}
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
	if (Attribute == GetArmorAttribute() && !bDeferringArmorEvents && !bHandlingArmorReplication && OldValue != NewValue)
	{
		OnArmorChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);
	}
	else if (Attribute == GetMaxArmorAttribute() && !bHandlingArmorReplication)
	{
		const float OldArmor = GetArmor();
		{
			TGuardValue<bool> Defer(bDeferringArmorEvents, true);
			if (GetArmor() > NewValue)
			{
				UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent();
				check(ASC);
				ASC->ApplyModToAttribute(GetArmorAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
		if (!bDeferringArmorEvents)
		{
			if (OldValue != NewValue) { OnMaxArmorChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue); }
			if (OldArmor != GetArmor()) { OnArmorChanged.Broadcast(nullptr, nullptr, nullptr, GetArmor() - OldArmor, OldArmor, GetArmor()); }
		}
	}

	if (Attribute == GetMaxHealthAttribute())
	{
		// Make sure current health is not greater than the new max health.
		if (GetHealth() > NewValue)
		{
			UDCAbilitySystemComponent* LyraASC = GetDCAbilitySystemComponent();
			check(LyraASC);

			LyraASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	if (bOutOfHealth && (GetHealth() > 0.0f))
	{
		bOutOfHealth = false;
	}
}

void UDCHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		// Do not allow health to go negative or above max health.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Do not allow max health to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetArmorAttribute())
	{
		const float Maximum = FMath::IsFinite(GetMaxArmor()) ? FMath::Max(GetMaxArmor(), 0.f) : 0.f;
		NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.f, Maximum) : 0.f;
	}
	else if (Attribute == GetMaxArmorAttribute())
	{
		NewValue = FMath::IsFinite(NewValue) ? FMath::Max(NewValue, 0.f) : 0.f;
	}
}
