// Copyright Epic Games, Inc. All Rights Reserved.
#include "Settings/CustomSettings/DCLyraSettingValueDiscrete_MobileFPSType.h"
#include "Performance/DCLyraPerformanceSettings.h"
#include "Settings/DCLyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraSettingValueDiscrete_MobileFPSType)

#define LOCTEXT_NAMESPACE "LyraSettings"

UDCLyraSettingValueDiscrete_MobileFPSType::UDCLyraSettingValueDiscrete_MobileFPSType()
{
}

void UDCLyraSettingValueDiscrete_MobileFPSType::OnInitialized()
{
	Super::OnInitialized();

	const UDCLyraPlatformSpecificRenderingSettings* PlatformSettings = UDCLyraPlatformSpecificRenderingSettings::Get();
	const UDCLyraSettingsLocal* UserSettings = UDCLyraSettingsLocal::Get();

	for (int32 TestLimit : PlatformSettings->MobileFrameRateLimits)
	{
		if (UDCLyraSettingsLocal::IsSupportedMobileFramePace(TestLimit))
		{
			FPSOptions.Add(TestLimit, MakeLimitString(TestLimit));
		}
	}

	const int32 FirstFrameRateWithQualityLimit = UserSettings->GetFirstFrameRateWithQualityLimit();
	if (FirstFrameRateWithQualityLimit > 0)
	{
		SetWarningRichText(FText::Format(LOCTEXT("MobileFPSType_Note", "<strong>Note: Changing the framerate setting to {0} or higher might lower your Quality Presets.</>"), MakeLimitString(FirstFrameRateWithQualityLimit)));
	}
}

int32 UDCLyraSettingValueDiscrete_MobileFPSType::GetDefaultFPS() const
{
	return UDCLyraSettingsLocal::GetDefaultMobileFrameRate();
}

FText UDCLyraSettingValueDiscrete_MobileFPSType::MakeLimitString(int32 Number)
{
	return FText::Format(LOCTEXT("MobileFrameRateOption", "{0} FPS"), FText::AsNumber(Number));
}

void UDCLyraSettingValueDiscrete_MobileFPSType::StoreInitial()
{
	InitialValue = GetValue();
}

void UDCLyraSettingValueDiscrete_MobileFPSType::ResetToDefault()
{
	SetValue(GetDefaultFPS(), EGameSettingChangeReason::ResetToDefault);
}

void UDCLyraSettingValueDiscrete_MobileFPSType::RestoreToInitial()
{
	SetValue(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UDCLyraSettingValueDiscrete_MobileFPSType::SetDiscreteOptionByIndex(int32 Index)
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);

	int32 NewMode = FPSOptionsModes.IsValidIndex(Index) ? FPSOptionsModes[Index] : GetDefaultFPS();

	SetValue(NewMode, EGameSettingChangeReason::Change);
}

int32 UDCLyraSettingValueDiscrete_MobileFPSType::GetDiscreteOptionIndex() const
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);
	return FPSOptionsModes.IndexOfByKey(GetValue());
}

TArray<FText> UDCLyraSettingValueDiscrete_MobileFPSType::GetDiscreteOptions() const
{
	TArray<FText> Options;
	FPSOptions.GenerateValueArray(Options);

	return Options;
}

int32 UDCLyraSettingValueDiscrete_MobileFPSType::GetValue() const
{
	return UDCLyraSettingsLocal::Get()->GetDesiredMobileFrameRateLimit();
}

void UDCLyraSettingValueDiscrete_MobileFPSType::SetValue(int32 NewLimitFPS, EGameSettingChangeReason InReason)
{
	UDCLyraSettingsLocal::Get()->SetDesiredMobileFrameRateLimit(NewLimitFPS);

	NotifySettingChanged(InReason);
}

#undef LOCTEXT_NAMESPACE

