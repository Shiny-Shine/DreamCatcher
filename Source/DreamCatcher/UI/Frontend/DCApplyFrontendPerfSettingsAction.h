// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFeatureAction.h"

#include "DCApplyFrontendPerfSettingsAction.generated.h"

class UObject;
struct FGameFeatureActivatingContext;
struct FGameFeatureDeactivatingContext;

//////////////////////////////////////////////////////////////////////
// UDCApplyFrontendPerfSettingsAction

/**
 * GameFeatureAction responsible for telling the user settings to apply frontend/menu specific performance settings
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Use Frontend Perf Settings"))
class UDCApplyFrontendPerfSettingsAction final : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	//~UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~End of UGameFeatureAction interface

private:
	static int32 ApplicationCounter;
};
