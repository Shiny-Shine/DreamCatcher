// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingAction.h"
#include "GameSettingValueScalarDynamic.h"
#include "DCLyraSettingAction_HDRCalibrationEditor.generated.h"

class UGameSetting;

UCLASS()
class UDCLyraSettingAction_HDRCalibrationEditor : public UGameSettingAction
{
	GENERATED_BODY()
	
public:
	UDCLyraSettingAction_HDRCalibrationEditor();
	virtual TArray<UGameSetting*> GetChildSettings() override;

private:
	UPROPERTY()
	TObjectPtr<UGameSettingValueScalarDynamic> HDRCalibrationValueSetting;
};
