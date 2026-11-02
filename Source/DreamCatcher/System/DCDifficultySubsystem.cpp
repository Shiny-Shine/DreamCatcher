#include "System/DCDifficultySubsystem.h"

#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossLineShot.h"
#include "AI/Boss/DCBossCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameModes/DCStageRunGameMode.h"
#include "GameplayEffect.h"
#include "Inventory/DCInventoryItemDefinition.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCDifficultySubsystem)

bool UDCDifficultySubsystem::SelectDifficulty(EDCDifficulty Difficulty)
{
	if (Difficulty != EDCDifficulty::Easy && Difficulty != EDCDifficulty::Normal && Difficulty != EDCDifficulty::Hard) { return false; }
	SelectedDifficulty = Difficulty;
	return true;
}

bool UDCDifficultySubsystem::BeginStoryRun(const UDCDifficultyCatalog* Catalog, FString& OutError)
{
	OutError.Reset();
	if (!Catalog) { OutError = TEXT("Difficulty catalog is missing."); return false; }
	if (!Catalog->Validate(OutError)) { return false; }
	const UDCDifficultyPreset* Preset = Catalog->GetPreset(SelectedDifficulty);
	if (!Preset) { OutError = TEXT("Selected difficulty is invalid."); return false; }
	ActiveValues = Preset->Values;
	ActiveDifficulty = SelectedDifficulty;
	ActiveCatalog = Catalog;
	return true;
}

bool UDCDifficultySubsystem::EnsureStoryRun(const UDCDifficultyCatalog* Catalog, FString& OutError)
{
	OutError.Reset();
	if (!ActiveCatalog) { return BeginStoryRun(Catalog, OutError); }
	if (ActiveCatalog != Catalog) { OutError = TEXT("Story maps must share the same difficulty catalog."); return false; }
	return true;
}

void UDCDifficultySubsystem::EndStoryRun()
{
	ActiveCatalog = nullptr;
	ActiveValues = FDCDifficultyValues();
	ActiveDifficulty = EDCDifficulty::Normal;
}

UDCDifficultySubsystem* UDCDifficultySubsystem::GetForStage(const UWorld* World)
{
	if (!World || World->GetNetMode() != NM_Standalone) { return nullptr; }
	const ADCStageRunGameMode* Mode = World->GetAuthGameMode<ADCStageRunGameMode>();
	UGameInstance* Instance = World->GetGameInstance();
	UDCDifficultySubsystem* Difficulty = Instance ? Instance->GetSubsystem<UDCDifficultySubsystem>() : nullptr;
	return Mode && Mode->GetDifficultyCatalog() && Difficulty
		&& Difficulty->ActiveCatalog == Mode->GetDifficultyCatalog() ? Difficulty : nullptr;
}

bool UDCDifficultySubsystem::TryOverrideBaseDamage(const AActor* SourceAvatar, const FGameplayEffectSpec& Spec, float& OutDamage) const
{
	if (!SourceAvatar || !ActiveCatalog || !Spec.Def) { return false; }
	const UGameplayAbility* Ability = Spec.GetContext().GetAbility();
	if (SourceAvatar->IsA<ADCBossCharacter>())
	{
		if (Ability && Ability->IsA<UDCGameplayAbility_BossLineShot>()) { OutDamage = ActiveValues.BossLineDamage; return true; }
		if (Ability && Ability->IsA<UDCGameplayAbility_BossGroundStrike>()) { OutDamage = ActiveValues.BossGroundDamage; return true; }
		return false;
	}
	const bool bSMG = Spec.Def->GetClass() == ActiveCatalog->SMGDamageEffect.Get();
	const bool bPistol = Spec.Def->GetClass() == ActiveCatalog->PistolDamageEffect.Get();
	const APawn* Pawn = Cast<APawn>(SourceAvatar);
	if (Pawn && Pawn->GetController() && Pawn->GetController()->IsA<ADCLyraPlayerBotController>() && (bSMG || bPistol))
	{
		OutDamage = ActiveValues.EnemyDamage; return true;
	}
	if (SourceAvatar->IsA<ADCGASEnemyCharacter>() && Ability && Ability->IsA<UDCGameplayAbility_EnemyRangedAttack>())
	{
		OutDamage = ActiveValues.EnemyDamage; return true;
	}
	if (Pawn && Pawn->IsPlayerControlled())
	{
		if (bSMG) { OutDamage = ActiveValues.PlayerSMGDamage; return true; }
		if (bPistol) { OutDamage = ActiveValues.PlayerPistolDamage; return true; }
	}
	// Preserve unrecognized effects, environmental damage and diagnostic kill effects.
	return false;
}

bool UDCDifficultySubsystem::ApplyStartingAmmo(UDCInventoryItemInstance* Item) const
{
	if (!Item || !ActiveCatalog) { return false; }
	const bool bSMG = Item->GetItemDef() == ActiveCatalog->SMGItem;
	const bool bPistol = Item->GetItemDef() == ActiveCatalog->PistolItem;
	if (!bSMG && !bPistol) { return true; }
	const FGameplayTag CapacityTag = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineSize"));
	const FGameplayTag LoadedTag = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineAmmo"));
	const FGameplayTag SpareTag = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo"));
	const int32 Loaded = bSMG ? ActiveValues.SMGLoadedAmmo : ActiveValues.PistolLoadedAmmo;
	const int32 Spare = bSMG ? ActiveValues.SMGSpareAmmo : ActiveValues.PistolSpareAmmo;
	if (Loaded < 0 || Loaded > Item->GetStatTagStackCount(CapacityTag) || Spare < 0) { return false; }
	Item->RemoveStatTagStack(LoadedTag, Item->GetStatTagStackCount(LoadedTag));
	Item->RemoveStatTagStack(SpareTag, Item->GetStatTagStackCount(SpareTag));
	Item->AddStatTagStack(LoadedTag, Loaded);
	Item->AddStatTagStack(SpareTag, Spare);
	return true;
}
