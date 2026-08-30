#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCAttributeSet.h"
#include "DCHealthSet.generated.h"

class FLifetimeProperty;
struct FGameplayEffectModCallbackData;

/**
 * 플레이어와 전투 Actor의 체력 관련 Attribute를 정의.
 *
 * Health와 MaxHealth는 실제로 유지되는 상태.
 * Damage와 Healing은 Effect 처리 중에만 사용하는 임시 Meta Attribute.
 */
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCHealthSet : public UDCAttributeSet
{
	GENERATED_BODY()

public:
	UDCHealthSet();

	DC_ATTRIBUTE_ACCESSORS(UDCHealthSet, Health);
	DC_ATTRIBUTE_ACCESSORS(UDCHealthSet, MaxHealth);
	DC_ATTRIBUTE_ACCESSORS(UDCHealthSet, Damage);
	DC_ATTRIBUTE_ACCESSORS(UDCHealthSet, Healing);

	// 체력이 변경되었을 때 발생, 이후 UDCHealthComponent가 이 이벤트를 구독해 기존 HUD와 Blueprint 이벤트로 전달.
	mutable FDCAttributeEvent OnHealthChanged;

	// 최대 체력이 변경되었을 때 발생.
	mutable FDCAttributeEvent OnMaxHealthChanged;

	// 체력이 처음으로 0 이하에 도달했을 때 발생.
	mutable FDCAttributeEvent OnOutOfHealth;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	// GameplayEffect가 Attribute를 변경하기 직전에 호출.
	// 무적 태그를 확인하고 변경 전 Health 값을 저장.
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	// GameplayEffect 계산이 끝난 뒤 호출.
	// Damage와 Healing Meta Attribute를 실제 Health로 변환.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// Attribute의 Base Value가 변경되기 전에 범위를 제한.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Attribute의 Current Value가 변경되기 전에 범위를 제한.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// MaxHealth 변경 후 Health가 MaxHealth를 넘지 않도록 처리.
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
	
	// OnOutOfHealth가 중복 발생하지 않게 관리.
	bool bOutOfHealth = false;
	
	// GameplayEffect 적용 직전 값을 저장.
	float HealthBeforeAttributeChange = 0.0f;
	float MaxHealthBeforeAttributeChange = 0.0f;

	// 현재 체력. 기본값 100으로 시작해 이후 GE_DC_InitializePlayer가 초기값을 설정.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "DreamCatcher|Health",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Health;

	// 최대 체력. GameplayEffect를 통해 버프나 디버프로 변경할 수 있음.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "DreamCatcher|Health",
		meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxHealth;

	/**
	 * 들어오는 데미지를 임시로 저장하는 Meta Attribute.
	 *
	 * 최종적으로 Damage만큼 Health를 감소시키고 다시 0으로 초기화.
	 * 디자이너가 일반 GameplayEffect Modifier에서 실수로 직접 변경하지 않도록 하기 위해
	 * Damage에 HideFromModifiers를 넣음.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DreamCatcher|Health",
		meta = (AllowPrivateAccess = "true", HideFromModifiers))
	FGameplayAttributeData Damage;

	/**
	 * 들어오는 회복량을 임시로 저장하는 Meta Attribute.
	 *
	 * 최종적으로 Healing만큼 Health를 증가시키고 다시 0으로 초기화.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DreamCatcher|Health", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Healing;
};
