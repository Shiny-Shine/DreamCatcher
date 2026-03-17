#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCPlayerHUDWidget.generated.h"

class ADreamCatcherCharacter;
class UDCCombatComponent;
class UDCHealthComponent;

UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 플레이어 캐릭터가 바뀔 때마다 PlayerController가 이 함수를 호출.
	UFUNCTION(BlueprintCallable, Category="HUD")
	void BindToCharacter(ADreamCatcherCharacter* NewCharacter);

protected:
	virtual void NativeDestruct() override;

	// C++는 숫자만 전달하고, 실제 바/아이콘 표현은 블루프린트에서.
	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void BP_OnPlayerBound(ADreamCatcherCharacter* NewCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void BP_OnHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void BP_OnUltimateChargeChanged(float NormalizedCharge);

private:
	void UnbindFromCurrentCharacter();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleUltimateChargeChanged(float NormalizedCharge);

	TWeakObjectPtr<ADreamCatcherCharacter> ObservedCharacter;
	TObjectPtr<UDCHealthComponent> BoundHealthComponent;
	TObjectPtr<UDCCombatComponent> BoundCombatComponent;
};