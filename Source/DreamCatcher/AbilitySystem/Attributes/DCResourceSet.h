#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCAttributeSet.h"
#include "DCResourceSet.generated.h"

class FLifetimeProperty;

// 플레이어의 전투 자원을 관리.
// 현재는 궁극기 게이지만 포함하지만 다른 캐릭터 자원을 별도 Set으로 추가할 수 있습니다.
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCResourceSet : public UDCAttributeSet
{
	GENERATED_BODY()

public:
	UDCResourceSet();

	DC_ATTRIBUTE_ACCESSORS(UDCResourceSet, UltimateCharge);

	DC_ATTRIBUTE_ACCESSORS(UDCResourceSet, MaxUltimateCharge);

	// 궁극기 게이지가 변경됐을 때 발생.
	mutable FDCAttributeEvent OnUltimateChargeChanged;

	// 최대 궁극기 게이지가 변경됐을 때 발생.
	mutable FDCAttributeEvent OnMaxUltimateChargeChanged;

	float GetUltimateChargeNormalized() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_UltimateCharge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxUltimateCharge(const FGameplayAttributeData& OldValue);

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// 현재 궁극기 게이지.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UltimateCharge, Category = "DreamCatcher|Resource",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData UltimateCharge;

	// 최대 궁극기 게이지.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxUltimateCharge, Category = "DreamCatcher|Resource",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxUltimateCharge;
};
