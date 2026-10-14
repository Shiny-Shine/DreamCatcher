#pragma once

#include "CoreMinimal.h"
#include "Equipment/DCGameplayAbility_FromEquipment.h"
#include "DCGameplayAbility_RangedWeapon.generated.h"

class UAbilityTask_WaitDelay;
class UDCRangedWeaponInstance;
class UGameplayEffect;

// 플레이어 라이플의 히트스캔 사격 어빌리티. 한 번 활성화할 때 한 발을 발사하고, 발사 간격이 지난 뒤 종료.
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCGameplayAbility_RangedWeapon : public UDCGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:
	UDCGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Weapon")
	UDCRangedWeaponInstance* GetWeaponInstance() const;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Weapon|Debug")
	bool bDrawDebugTrace = true;

private:
	UFUNCTION()
	void HandleRefireDelayFinished();

	// 장비 해제나 사망으로 Ability가 취소되면 함께 정리.
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> RefireDelayTask;
};
