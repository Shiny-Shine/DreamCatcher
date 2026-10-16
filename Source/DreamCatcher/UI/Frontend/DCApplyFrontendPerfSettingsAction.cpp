// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCApplyFrontendPerfSettingsAction.h"

#include "Settings/DCLyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCApplyFrontendPerfSettingsAction)

struct FGameFeatureActivatingContext;
struct FGameFeatureDeactivatingContext;

//////////////////////////////////////////////////////////////////////
// UDCApplyFrontendPerfSettingsAction

// Game user settings (and engine performance/scalability settings they drive)
// are global, so there's no point in tracking this per world for multi-player PIE:
// we just apply it if any PIE world is in the menu.
//
// However, by default we won't apply front-end performance stuff in the editor
// unless the developer setting ApplyFrontEndPerformanceOptionsInPIE is enabled
int32 UDCApplyFrontendPerfSettingsAction::ApplicationCounter = 0;

void UDCApplyFrontendPerfSettingsAction::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	ApplicationCounter++;
	if (ApplicationCounter == 1)
	{
		UDCLyraSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(true);
	}
}

void UDCApplyFrontendPerfSettingsAction::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	ApplicationCounter--;
	check(ApplicationCounter >= 0);

	if (ApplicationCounter == 0)
	{
		UDCLyraSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(false);
	}
}

