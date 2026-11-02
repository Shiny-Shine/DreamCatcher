#include "GameModes/DCDifficultyData.h"

#include "GameplayEffect.h"
#include "Inventory/DCInventoryFragment_SetStats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCDifficultyData)

bool FDCDifficultyValues::Validate(int32 SMGCapacity, int32 PistolCapacity, FString& OutError) const
{
	OutError.Reset();
	for (float Damage : { EnemyDamage, BossLineDamage, BossGroundDamage, PlayerSMGDamage, PlayerPistolDamage })
	{
		if (!FMath::IsFinite(Damage) || Damage < 0.f)
		{
			OutError = TEXT("Damage must be finite and nonnegative."); return false;
		}
	}
	if (SMGCapacity <= 0 || PistolCapacity <= 0 || SMGLoadedAmmo < 0 || SMGLoadedAmmo > SMGCapacity
		|| PistolLoadedAmmo < 0 || PistolLoadedAmmo > PistolCapacity || SMGSpareAmmo < 0 || PistolSpareAmmo < 0)
	{
		OutError = TEXT("Loaded ammo must fit the existing magazine; reserve ammo must be nonnegative."); return false;
	}
	return true;
}

const UDCDifficultyPreset* UDCDifficultyCatalog::GetPreset(EDCDifficulty Difficulty) const
{
	switch (Difficulty)
	{
	case EDCDifficulty::Easy: return Easy;
	case EDCDifficulty::Normal: return Normal;
	case EDCDifficulty::Hard: return Hard;
	default: return nullptr;
	}
}

bool UDCDifficultyCatalog::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!Easy || !Normal || !Hard || Easy == Normal || Easy == Hard || Normal == Hard
		|| !SMGDamageEffect || !PistolDamageEffect || SMGDamageEffect == PistolDamageEffect
		|| !SMGItem || !PistolItem || SMGItem == PistolItem)
	{
		OutError = TEXT("Assign three distinct presets and both existing weapon effects/item definitions."); return false;
	}
	const auto Capacity = [](TSubclassOf<UDCInventoryItemDefinition> ItemClass)
	{
		const UDCInventoryItemDefinition* Definition = ItemClass.GetDefaultObject();
		const UDCInventoryFragment_SetStats* Stats = Definition ? Cast<UDCInventoryFragment_SetStats>(
			Definition->FindFragmentByClass(UDCInventoryFragment_SetStats::StaticClass())) : nullptr;
		return Stats ? Stats->GetItemStatByTag(FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineSize"))) : 0;
	};
	const int32 SMGCapacity = Capacity(SMGItem);
	const int32 PistolCapacity = Capacity(PistolItem);
	for (const UDCDifficultyPreset* Preset : { Easy.Get(), Normal.Get(), Hard.Get() })
	{
		if (!Preset->Values.Validate(SMGCapacity, PistolCapacity, OutError))
		{
			OutError = Preset->GetPathName() + TEXT(": ") + OutError; return false;
		}
	}
	return true;
}

FString UDCDifficultyCatalog::GetValidationError() const
{
	FString Error; Validate(Error); return Error;
}
