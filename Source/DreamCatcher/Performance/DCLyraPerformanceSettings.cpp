// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraPerformanceSettings.h"

#include "Engine/PlatformSettingsManager.h"
#include "Misc/EnumRange.h"
#include "Performance/DCLyraPerformanceStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraPerformanceSettings)

//////////////////////////////////////////////////////////////////////

UDCLyraPlatformSpecificRenderingSettings::UDCLyraPlatformSpecificRenderingSettings()
{
	MobileFrameRateLimits.Append({ 20, 30, 45, 60, 90, 120 });
}

const UDCLyraPlatformSpecificRenderingSettings* UDCLyraPlatformSpecificRenderingSettings::Get()
{
	UDCLyraPlatformSpecificRenderingSettings* Result = UPlatformSettingsManager::Get().GetSettingsForPlatform<ThisClass>();
	check(Result);
	return Result;
}

//////////////////////////////////////////////////////////////////////

UDCLyraPerformanceSettings::UDCLyraPerformanceSettings()
{
	PerPlatformSettings.Initialize(UDCLyraPlatformSpecificRenderingSettings::StaticClass());

	CategoryName = TEXT("Game");

	DesktopFrameRateLimits.Append({ 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 });

	// Default to all stats are allowed
	FDCLyraPerformanceStatGroup& StatGroup = UserFacingPerformanceStats.AddDefaulted_GetRef();
	for (EDCLyraDisplayablePerformanceStat PerfStat : TEnumRange<EDCLyraDisplayablePerformanceStat>())
	{
		StatGroup.AllowedStats.Add(PerfStat);
	}
}
