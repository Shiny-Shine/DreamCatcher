// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingValueDiscrete.h"
#include "DCLyraSettingValueDiscrete_PerfStat.generated.h"

enum class EDCLyraDisplayablePerformanceStat : uint8;
enum class EDCLyraStatDisplayMode : uint8;

class UObject;

UCLASS()
class UDCLyraSettingValueDiscrete_PerfStat : public UGameSettingValueDiscrete
{
	GENERATED_BODY()
	
public:

	UDCLyraSettingValueDiscrete_PerfStat();

	void SetStat(EDCLyraDisplayablePerformanceStat InStat);

	/** UGameSettingValue */
	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	/** UGameSettingValueDiscrete */
	virtual void SetDiscreteOptionByIndex(int32 Index) override;
	virtual int32 GetDiscreteOptionIndex() const override;
	virtual TArray<FText> GetDiscreteOptions() const override;

protected:
	/** UGameSettingValue */
	virtual void OnInitialized() override;
	
	void AddMode(FText&& Label, EDCLyraStatDisplayMode Mode);
protected:
	TArray<FText> Options;
	TArray<EDCLyraStatDisplayMode> DisplayModes;

	EDCLyraDisplayablePerformanceStat StatToDisplay;
	EDCLyraStatDisplayMode InitialMode;
};
