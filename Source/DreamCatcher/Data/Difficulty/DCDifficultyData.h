#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DCDifficultyData.generated.h"

class UGameplayEffect;
class UDCInventoryItemDefinition;

UENUM(BlueprintType)
enum class EDCDifficulty : uint8
{
	Easy UMETA(DisplayName="쉬움"),
	Normal UMETA(DisplayName="보통"),
	Hard UMETA(DisplayName="어려움")
};

/** Absolute base damage, before the existing hit-zone, distance and armor rules. */
USTRUCT(BlueprintType)
struct DREAMCATCHER_API FDCDifficultyValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0", DisplayName="일반 적 피해"))
	float EnemyDamage = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0", DisplayName="보스 직선 피해"))
	float BossLineDamage = 34.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0", DisplayName="보스 지면 피해"))
	float BossGroundDamage = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0", DisplayName="플레이어 SMG 피해"))
	float PlayerSMGDamage = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0", DisplayName="플레이어 권총 피해"))
	float PlayerPistolDamage = 64.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0", DisplayName="SMG 시작 장전량"))
	int32 SMGLoadedAmmo = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0", DisplayName="SMG 예비탄"))
	int32 SMGSpareAmmo = 120;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0", DisplayName="권총 시작 장전량"))
	int32 PistolLoadedAmmo = 12;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0", DisplayName="권총 예비탄"))
	int32 PistolSpareAmmo = 240;

	bool Validate(int32 SMGCapacity, int32 PistolCapacity, FString& OutError) const;
};

/** Save edits here, then start a NEW story. A running story keeps its copied values. */
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCDifficultyPreset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Difficulty", meta=(ShowOnlyInnerProperties))
	FDCDifficultyValues Values;
};

/** Shared opt-in reference on the frontend and the three production story modes. */
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCDifficultyCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presets") TObjectPtr<UDCDifficultyPreset> Easy;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presets") TObjectPtr<UDCDifficultyPreset> Normal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presets") TObjectPtr<UDCDifficultyPreset> Hard;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Existing Weapons") TSubclassOf<UGameplayEffect> SMGDamageEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Existing Weapons") TSubclassOf<UGameplayEffect> PistolDamageEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Existing Weapons") TSubclassOf<UDCInventoryItemDefinition> SMGItem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Existing Weapons") TSubclassOf<UDCInventoryItemDefinition> PistolItem;

	const UDCDifficultyPreset* GetPreset(EDCDifficulty Difficulty) const;
	bool Validate(FString& OutError) const;
	UFUNCTION(BlueprintPure, Category="Difficulty") FString GetValidationError() const;
};
