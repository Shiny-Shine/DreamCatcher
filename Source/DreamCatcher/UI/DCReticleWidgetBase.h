#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCReticleWidgetBase.generated.h"

class UDCEquipmentManagerComponent;
class UDCRangedWeaponInstance;

// 현재 장착 무기의 실제 퍼짐을 표시하는 크로스헤어 기본 위젯.
// 무기 데이터는 WeaponInstance가 소유. 이 위젯은 해당 값을 화면상의 반경으로 변환.

// 장비가 바뀔 때는 이벤트로 관찰 대상을 바꾸고, 화면 반경은 매 프레임 계산. 사격을 멈춰도 Heat 회복이나 FOV 전환으로 크로스헤어 크기가 계속 변하기 때문.
// SpreadRadius에는 DPI 보정까지 들어 있으므로 Blueprint에서 다시 DPI로 나누거나 임의의 퍼짐 배율을 곱하지 않음.
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCReticleWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Reticle")
	UDCRangedWeaponInstance* GetWeaponInstance() const
	{
		return WeaponInstance.Get();
	}

	// 상태 배율까지 적용된 전체 퍼짐각.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Reticle")
	float ComputeSpreadAngle() const;

	// DPI 스케일을 보정한 UMG 좌표 단위의 반경.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Reticle")
	float ComputeSpreadRadius() const;

	// 무기가 없거나 Scope/사망 상태라면 false.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Reticle")
	bool ShouldShowReticle() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Blueprint는 이 값을 받아 그림의 위치와 표시 여부만 변경.
	UFUNCTION(BlueprintImplementableEvent, Category = "DreamCatcher|Reticle")
	void BP_OnReticleUpdated(float SpreadRadius, float SpreadAngle, bool bShowReticle);

private:
	void RefreshEquipmentBinding();
	void UnbindEquipment();

	UFUNCTION()
	void HandleEquipmentChanged();

	UPROPERTY(Transient)
	TWeakObjectPtr<UDCEquipmentManagerComponent> ObservedEquipment;

	UPROPERTY(Transient)
	TObjectPtr<UDCRangedWeaponInstance> WeaponInstance;
};
