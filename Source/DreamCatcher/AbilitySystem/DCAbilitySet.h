#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "DCAbilitySet.generated.h"

class UAttributeSet;
class UGameplayEffect;
class UDCAbilitySystemComponent;
class UDCGameplayAbility;

// AbilitySet이 부여할 Gameplay Ability 정보.
USTRUCT(BlueprintType)
struct FDCAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	// 부여할 Ability 클래스.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UDCGameplayAbility> Ability;

	// 부여할 Ability 레벨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/*
	 * 이 Ability를 활성화할 입력 태그.
	 *
	 * 예:
	 * InputTag.Dodge
	 * InputTag.Weapon.Fire
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

// AbilitySet이 적용할 Gameplay Effect 정보.
USTRUCT(BlueprintType)
struct FDCAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	// ASC 자신에게 적용할 Gameplay Effect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	// 적용할 Gameplay Effect 레벨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0.0"))
	float EffectLevel = 1.0f;
};

// AbilitySet이 생성할 AttributeSet 정보.
USTRUCT(BlueprintType)
struct FDCAbilitySet_AttributeSet
{
	GENERATED_BODY()

	// ASC에 추가할 AttributeSet 클래스.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
	TSubclassOf<UAttributeSet> AttributeSet;
};

// AbilitySet이 부여한 항목의 Handle을 보관, 장비 해제처럼 나중에 부여한 항목을 회수해야 할 때 사용.
USTRUCT()
struct DREAMCATCHER_API FDCAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);

	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);

	void AddAttributeSet(UAttributeSet* AttributeSet);

	// 이 Handle 묶음이 부여했던 항목을 ASC에서 제거.
	void TakeFromAbilitySystem(UDCAbilitySystemComponent* AbilitySystemComponent);

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};

// Ability, Effect, AttributeSet을 묶어서 부여하는 Data Asset.
UCLASS(BlueprintType, Const)
class DREAMCATCHER_API UDCAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDCAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 이 AbilitySet의 모든 항목을 ASC에 부여.
	 *
	 * OutGrantedHandles를 전달하면 나중에 부여한 항목을 회수할 수 있음.
	 * SourceObject는 장비처럼 Ability를 부여한 출처를 기록할 때 사용.
	 */
	void GiveToAbilitySystem(UDCAbilitySystemComponent* AbilitySystemComponent,
		FDCAbilitySet_GrantedHandles* OutGrantedHandles = nullptr,
		UObject* SourceObject = nullptr
	) const;

protected:
	// 이 AbilitySet이 부여할 Ability 목록입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Abilities", meta = (TitleProperty = "Ability"))
	TArray<FDCAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// 이 AbilitySet이 적용할 Effect 목록입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects", meta = (TitleProperty = "GameplayEffect"))
	TArray<FDCAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// 이 AbilitySet이 생성할 AttributeSet 목록입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute Sets", meta = (TitleProperty = "AttributeSet"))
	TArray<FDCAbilitySet_AttributeSet> GrantedAttributes;
};
