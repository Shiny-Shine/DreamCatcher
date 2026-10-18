// Copyright Epic Games, Inc. All Rights Reserved.
#include "DCLyraExperiencePlayerState.h"

#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCPawnData.h"
#include "DCLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameMode.h"
#include "NativeGameplayTags.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraExperiencePlayerState)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Damage_IgnoreWeaponBonus, "Gameplay.Damage.IgnoreWeaponBonus");

ADCLyraExperiencePlayerState::ADCLyraExperiencePlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ADCLyraExperiencePlayerState::PostInitializeComponents()
{
	// Base already constructs/registers ASC and attributes. Never create a second ASC.
	Super::PostInitializeComponents();

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		AGameStateBase* GameState = World->GetGameState();
		check(GameState);
		UDCLyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnDCLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void ADCLyraExperiencePlayerState::OnExperienceLoaded(const UDCLyraExperienceDefinition* CurrentExperience)
{
	if (ADCLyraGameMode* GameMode = GetWorld()->GetAuthGameMode<ADCLyraGameMode>())
	{
		if (const UDCPawnData* NewPawnData = GameMode->GetPawnDataForController(GetOwningController()))
		{
			// Set attributes and source tags before SetPawnData grants abilities and signals ability readiness.
			if (!InitializeRoleBaseDamage(CurrentExperience))
			{
				return;
			}

			// The original SetPawnData is immutable. Preserve an already assigned per-controller loadout.
			if (!GetPawnData<UDCPawnData>())
			{
				SetPawnData(NewPawnData);
			}
		}
		else
		{
			UE_LOG(LogDC, Error, TEXT("Experience ready, but no PawnData for player state [%s]."), *GetNameSafe(this));
		}
	}
}

bool ADCLyraExperiencePlayerState::InitializeRoleBaseDamage(const UDCLyraExperienceDefinition* CurrentExperience)
{
	if (bRoleBaseDamageInitialized)
	{
		return true;
	}
	if (!IsValid(CurrentExperience))
	{
		UE_LOG(LogDC, Error, TEXT("Cannot initialize role BaseDamage for [%s]: no valid Experience."), *GetNameSafe(this));
		return false;
	}
	if (!CurrentExperience->bUseRoleBasedBaseDamage)
	{
		return true;
	}
	if (!HasAuthority())
	{
		UE_LOG(LogDC, Error, TEXT("Cannot initialize role BaseDamage for [%s] on non-authority."), *GetNameSafe(this));
		return false;
	}

	const AController* OwningController = GetOwningController();
	if (!IsValid(OwningController))
	{
		UE_LOG(LogDC, Error, TEXT("Cannot initialize role BaseDamage for [%s]: no valid owning controller."), *GetNameSafe(this));
		return false;
	}
	const bool bPlayerBot = OwningController->IsA<ADCLyraPlayerBotController>();
	if (!bPlayerBot && !OwningController->IsA<APlayerController>())
	{
		// This opt-in only handles player controllers and the existing Lyra player-bot path.
		return true;
	}

	if (!FMath::IsFinite(CurrentExperience->PlayerBaseDamage) || CurrentExperience->PlayerBaseDamage < 0.0f ||
		!FMath::IsFinite(CurrentExperience->EnemyBaseDamage) || CurrentExperience->EnemyBaseDamage < 0.0f)
	{
		UE_LOG(LogDC, Error, TEXT("Cannot initialize role BaseDamage for [%s]: Experience [%s] requires finite, non-negative player and enemy values."),
			*GetNameSafe(this), *GetNameSafe(CurrentExperience));
		return false;
	}

	UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent();
	if (!IsValid(ASC) || !IsValid(GetCombatSet()) || ASC->GetSet<UDCCombatSet>() != GetCombatSet())
	{
		UE_LOG(LogDC, Error, TEXT("Cannot initialize role BaseDamage for [%s]: its CombatSet is not registered with its ASC."), *GetNameSafe(this));
		return false;
	}

	const float InitialBaseDamage = bPlayerBot ? CurrentExperience->EnemyBaseDamage : CurrentExperience->PlayerBaseDamage;
	ASC->SetNumericAttributeBase(UDCCombatSet::GetBaseDamageAttribute(), InitialBaseDamage);
	if (bPlayerBot)
	{
		ASC->SetLooseGameplayTagCount(TAG_Gameplay_Damage_IgnoreWeaponBonus, 1, EGameplayTagReplicationState::TagAndCountToAll);
	}
	bRoleBaseDamageInitialized = true;
	ForceNetUpdate();

	UE_LOG(LogDC, Log, TEXT("Role BaseDamage initialized for [%s]: Role=%s BaseDamage=%.2f IgnoreWeaponBonus=%s Experience=%s."),
		*GetNameSafe(this), bPlayerBot ? TEXT("Enemy") : TEXT("Player"), InitialBaseDamage,
		bPlayerBot ? TEXT("true") : TEXT("false"), *GetNameSafe(CurrentExperience));
	return true;
}
