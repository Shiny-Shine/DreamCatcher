// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"

#include "DCLyraWeaponUserInterface.generated.h"

class UDCLyraWeaponInstance;
class UObject;
struct FGeometry;

UCLASS()
class UDCLyraWeaponUserInterface : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UDCLyraWeaponUserInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponChanged(UDCLyraWeaponInstance* OldWeapon, UDCLyraWeaponInstance* NewWeapon);

private:
	void RebuildWidgetFromWeapon();

private:
	UPROPERTY(Transient)
	TObjectPtr<UDCLyraWeaponInstance> CurrentInstance;
};
