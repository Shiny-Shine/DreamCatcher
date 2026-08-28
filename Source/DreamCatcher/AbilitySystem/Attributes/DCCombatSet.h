#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCAttributeSet.h"
#include "DCCombatSet.generated.h"

class FLifetimeProperty;

// 공격이나 회복 Effect를 생성하는 Actor의 기준 수치.
// 실제 Health 감소는 HealthSet이 담당하고, Effect 계산에 사용할 원본 수치를 제공.
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCCombatSet : public UDCAttributeSet
{
	GENERATED_BODY()

public:
	UDCCombatSet();

	DC_ATTRIBUTE_ACCESSORS(UDCCombatSet, BaseDamage);
	DC_ATTRIBUTE_ACCESSORS(UDCCombatSet, BaseHeal);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

private:
	// Damage Execution이 읽을 기본 공격력.
	// 0으로 시작하고 이후 초기화 GameplayEffect로 값을 설정합니다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "DreamCatcher|Combat",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData BaseDamage;

	// 회복 Effect 계산에서 사용할 기본 회복량.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "DreamCatcher|Combat",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData BaseHeal;
};
