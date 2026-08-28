#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "DCAttributeSet.generated.h"

class AActor;
class UDCAbilitySystemComponent;
class UWorld;
struct FGameplayEffectSpec;

/**
 * Attribute 접근 함수를 한 번에 생성.
 *
 * 예:
 * DC_ATTRIBUTE_ACCESSORS(UDCHealthSet, Health)
 *
 * 생성되는 함수:
 * - GetHealthAttribute()
 * - GetHealth()
 * - SetHealth()
 * - InitHealth()
 */
#define DC_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Attribute 변경에 대한 내부 C++ 이벤트.
 *
 * Blueprint에는 이 Delegate를 직접 노출하지 않음.
 * 이후 UDCHealthComponent가 이 이벤트를 받아 기존 HUD용
 * BlueprintAssignable 이벤트로 변환.
 */
DECLARE_MULTICAST_DELEGATE_SixParams(
	FDCAttributeEvent,
	AActor* /* EffectInstigator */,
	AActor* /* EffectCauser */,
	const FGameplayEffectSpec* /* EffectSpec */,
	float /* EffectMagnitude */,
	float /* OldValue */,
	float /* NewValue */
);

/**
 * DreamCatcher AttributeSet의 공통 기반 클래스.
 *
 * 직접 데이터 에셋이나 PlayerState에 넣는 클래스가 아닌, HealthSet, CombatSet, ResourceSet이 상속받는 기반 클래스.
 */
UCLASS(Abstract)
class DREAMCATCHER_API UDCAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UDCAttributeSet();

	/**
	 * AttributeSet의 Outer를 기준으로 현재 World를 반환.
	 *
	 * PlayerState가 AttributeSet을 소유해도 정상적으로 게임 World에 접근할 수 있음.
	 */
	virtual UWorld* GetWorld() const override;

	// 이 AttributeSet을 등록한 ASC를 DreamCatcher 전용 타입으로 반환.
	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const;
};