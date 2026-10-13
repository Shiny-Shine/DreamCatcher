// Copyright Epic Games, Inc. All Rights Reserved.
#include "Settings/CustomSettings/DCLyraSettingValueDiscrete_PerfStat.h"

#include "CommonUIVisibilitySubsystem.h"
#include "Performance/DCLyraPerformanceSettings.h"
#include "Performance/DCLyraPerformanceStatTypes.h"
#include "Settings/DCLyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraSettingValueDiscrete_PerfStat)

class ULocalPlayer;

#define LOCTEXT_NAMESPACE "LyraSettings"

//////////////////////////////////////////////////////////////////////

class FGameSettingEditCondition_PerfStatAllowed : public FGameSettingEditCondition
{
public:
	static TSharedRef<FGameSettingEditCondition_PerfStatAllowed> Get(EDCLyraDisplayablePerformanceStat Stat)
	{
		return MakeShared<FGameSettingEditCondition_PerfStatAllowed>(Stat);
	}

	FGameSettingEditCondition_PerfStatAllowed(EDCLyraDisplayablePerformanceStat Stat)
		: AssociatedStat(Stat)
	{
	}

	//~FGameSettingEditCondition interface
	virtual void GatherEditState(const ULocalPlayer* InLocalPlayer, FGameSettingEditableState& InOutEditState) const override
	{
		const FGameplayTagContainer& VisibilityTags = UCommonUIVisibilitySubsystem::GetChecked(InLocalPlayer)->GetVisibilityTags();

		bool bCanShowStat = false;
		for (const FDCLyraPerformanceStatGroup& Group : GetDefault<UDCLyraPerformanceSettings>()->UserFacingPerformanceStats) //@TODO: Move this stuff to per-platform instead of doing vis queries too?
		{
			if (Group.AllowedStats.Contains(AssociatedStat))
			{
				const bool bShowGroup = (Group.VisibilityQuery.IsEmpty() || Group.VisibilityQuery.Matches(VisibilityTags));
				if (bShowGroup)
				{
					bCanShowStat = true;
					break;
				}
			}
		}

		if (!bCanShowStat)
		{
			InOutEditState.Hide(TEXT("Stat is not listed in ULyraPerformanceSettings or is suppressed by current platform traits"));
		}
	}
	//~End of FGameSettingEditCondition interface

private:
	EDCLyraDisplayablePerformanceStat AssociatedStat;
};

//////////////////////////////////////////////////////////////////////

UDCLyraSettingValueDiscrete_PerfStat::UDCLyraSettingValueDiscrete_PerfStat()
{
}

void UDCLyraSettingValueDiscrete_PerfStat::SetStat(EDCLyraDisplayablePerformanceStat InStat)
{
	StatToDisplay = InStat;
	SetDevName(FName(*FString::Printf(TEXT("PerfStat_%d"), (int32)StatToDisplay)));
	AddEditCondition(FGameSettingEditCondition_PerfStatAllowed::Get(StatToDisplay));
}

void UDCLyraSettingValueDiscrete_PerfStat::AddMode(FText&& Label, EDCLyraStatDisplayMode Mode)
{
	Options.Emplace(MoveTemp(Label));
	DisplayModes.Add(Mode);
}

void UDCLyraSettingValueDiscrete_PerfStat::OnInitialized()
{
	Super::OnInitialized();

	AddMode(LOCTEXT("PerfStatDisplayMode_None", "None"), EDCLyraStatDisplayMode::Hidden);
	AddMode(LOCTEXT("PerfStatDisplayMode_TextOnly", "Text Only"), EDCLyraStatDisplayMode::TextOnly);
	AddMode(LOCTEXT("PerfStatDisplayMode_GraphOnly", "Graph Only"), EDCLyraStatDisplayMode::GraphOnly);
	AddMode(LOCTEXT("PerfStatDisplayMode_TextAndGraph", "Text and Graph"), EDCLyraStatDisplayMode::TextAndGraph);
}

void UDCLyraSettingValueDiscrete_PerfStat::StoreInitial()
{
	const UDCLyraSettingsLocal* Settings = UDCLyraSettingsLocal::Get();
	InitialMode = Settings->GetPerfStatDisplayState(StatToDisplay);
}

void UDCLyraSettingValueDiscrete_PerfStat::ResetToDefault()
{
	UDCLyraSettingsLocal* Settings = UDCLyraSettingsLocal::Get();
	Settings->SetPerfStatDisplayState(StatToDisplay, EDCLyraStatDisplayMode::Hidden);
	NotifySettingChanged(EGameSettingChangeReason::ResetToDefault);
}

void UDCLyraSettingValueDiscrete_PerfStat::RestoreToInitial()
{
	UDCLyraSettingsLocal* Settings = UDCLyraSettingsLocal::Get();
	Settings->SetPerfStatDisplayState(StatToDisplay, InitialMode);
	NotifySettingChanged(EGameSettingChangeReason::RestoreToInitial);
}

void UDCLyraSettingValueDiscrete_PerfStat::SetDiscreteOptionByIndex(int32 Index)
{
	if (DisplayModes.IsValidIndex(Index))
	{
		const EDCLyraStatDisplayMode DisplayMode = DisplayModes[Index];

		UDCLyraSettingsLocal* Settings = UDCLyraSettingsLocal::Get();
		Settings->SetPerfStatDisplayState(StatToDisplay, DisplayMode);
	}
	NotifySettingChanged(EGameSettingChangeReason::Change);
}

int32 UDCLyraSettingValueDiscrete_PerfStat::GetDiscreteOptionIndex() const
{
	const UDCLyraSettingsLocal* Settings = UDCLyraSettingsLocal::Get();
	return DisplayModes.Find(Settings->GetPerfStatDisplayState(StatToDisplay));
}

TArray<FText> UDCLyraSettingValueDiscrete_PerfStat::GetDiscreteOptions() const
{
	return Options;
}

#undef LOCTEXT_NAMESPACE
