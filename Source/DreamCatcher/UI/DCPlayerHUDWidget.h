#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/DCCombatComponent.h"
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
	
	// C++에서 감지한 조준 모드 변경을 Blueprint HUD에 전달하는 이벤트.
	// WBP_PlayerHUD에서 이 이벤트를 구현하여 크로스헤어와 Scope 오버레이 변경.
	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void BP_OnAimModeChanged(EDCAimMode NewAimMode);

private:
	void UnbindFromCurrentCharacter();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleUltimateChargeChanged(float NormalizedCharge);
	
	// CombatComponent의 OnAimModeChanged 델리게이트에 연결되는 내부 함수.
	// 조준 모드가 바뀌면 호출되고, 변경된 값을 BP_OnAimModeChanged로 전달.
	UFUNCTION()
	void HandleAimModeChanged(EDCAimMode NewAimMode);

	TWeakObjectPtr<ADreamCatcherCharacter> ObservedCharacter;
	TObjectPtr<UDCHealthComponent> BoundHealthComponent;
	TObjectPtr<UDCCombatComponent> BoundCombatComponent;
};