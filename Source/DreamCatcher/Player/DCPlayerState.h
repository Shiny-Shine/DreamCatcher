#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "DCPlayerState.generated.h"

class UAbilitySystemComponent;
class UDCAbilitySet;
class UDCAbilitySystemComponent;

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

protected:
	virtual void PostInitializeComponents() override;

	/**
	 * PlayerState가 유지되는 동안 함께 유지할 AbilitySet.
	 *
	 * 현재는 Foundation 테스트에 사용.
	 * 무기 AbilitySet은 여기에 넣지 않고 Equipment가 부여.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Ability System")
	TArray<TObjectPtr<const UDCAbilitySet>>
	PlayerStateAbilitySets;

private:
	// PlayerState가 실제로 소유하는 ASC.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDCAbilitySystemComponent>
	AbilitySystemComponent;
};
