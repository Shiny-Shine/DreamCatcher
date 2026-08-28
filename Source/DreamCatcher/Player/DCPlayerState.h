#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "DCPlayerState.generated.h"

class UAbilitySystemComponent;
class UDCAbilitySet;
class UDCAbilitySystemComponent;
class UDCHealthSet;
class UDCCombatSet;
class UDCResourceSet;

// DreamCatcher 플레이어의 GAS 상태를 소유하는 PlayerState.
// ASC는 PlayerState가 소유하고, 실제 플레이 Character는 ASC의 Avatar로 연결됨.
UCLASS()
class DREAMCATCHER_API ADCPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADCPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface 구현.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// DreamCatcher 전용 ASC 타입으로 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Ability System")
	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}

	// 플레이어의 체력 AttributeSet을 반환.
	const UDCHealthSet* GetHealthSet() const
	{
		return HealthSet;
	}
	
	const UDCCombatSet* GetCombatSet() const
	{
		return CombatSet;
	}

	const UDCResourceSet* GetResourceSet() const
	{
		return ResourceSet;
	}

protected:
	virtual void PostInitializeComponents() override;

	// PlayerState가 유지되는 동안 함께 유지할 AbilitySet.
	// 현재는 Foundation 테스트에 사용, 무기 AbilitySet은 여기에 넣지 않고 Equipment가 부여.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Ability System")
	TArray<TObjectPtr<const UDCAbilitySet>> PlayerStateAbilitySets;

private:
	// PlayerState가 실제로 소유하는 ASC.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDCAbilitySystemComponent> AbilitySystemComponent;
	
	// 플레이어의 Health, MaxHealth, Damage, Healing을 소유.
	// ASC와 동일하게 PlayerState의 기본 Subobject이므로 Pawn이 교체되어도 PlayerState가 유지되면 함께 유지.
	UPROPERTY()
	TObjectPtr<const UDCHealthSet> HealthSet;
	
	// 공격과 회복 계산에 사용할 기본 AttributeSet.
	UPROPERTY()
	TObjectPtr<const UDCCombatSet> CombatSet;

	// 궁극기 게이지를 관리하는 AttributeSet.
	UPROPERTY()
	TObjectPtr<const UDCResourceSet> ResourceSet;
};
