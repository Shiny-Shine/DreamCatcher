#pragma once

#include "GameModes/DCDifficultyData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DCDifficultySubsystem.generated.h"

class UDCInventoryItemInstance;
struct FGameplayEffectSpec;

/** Session selection and immutable story values; no SaveGame or global CDO edits. */
UCLASS()
class DREAMCATCHER_API UDCDifficultySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Difficulty") EDCDifficulty GetSelectedDifficulty() const { return SelectedDifficulty; }
	UFUNCTION(BlueprintPure, Category="Difficulty") EDCDifficulty GetActiveDifficulty() const { return ActiveDifficulty; }
	UFUNCTION(BlueprintPure, Category="Difficulty") bool HasActiveRun() const { return ActiveCatalog != nullptr; }
	UFUNCTION(BlueprintPure, Category="Difficulty") FDCDifficultyValues GetActiveValues() const { return ActiveValues; }
	bool SelectDifficulty(EDCDifficulty Difficulty);
	bool BeginStoryRun(const UDCDifficultyCatalog* Catalog, FString& OutError);
	bool EnsureStoryRun(const UDCDifficultyCatalog* Catalog, FString& OutError);
	void EndStoryRun();

	/** Only an opted-in authoritative standalone stage can use these values. */
	static UDCDifficultySubsystem* GetForStage(const UWorld* World);
	bool TryOverrideBaseDamage(const AActor* SourceAvatar, const FGameplayEffectSpec& Spec, float& OutDamage) const;
	/** Called exactly once per newly granted starting item, before QuickBar/equip. */
	bool ApplyStartingAmmo(UDCInventoryItemInstance* Item) const;
private:
	UPROPERTY(Transient) EDCDifficulty SelectedDifficulty = EDCDifficulty::Normal;
	UPROPERTY(Transient) EDCDifficulty ActiveDifficulty = EDCDifficulty::Normal;
	UPROPERTY(Transient) FDCDifficultyValues ActiveValues;
	UPROPERTY(Transient) TObjectPtr<const UDCDifficultyCatalog> ActiveCatalog;
};
