// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

class UObject;

DREAMCATCHER_API DECLARE_LOG_CATEGORY_EXTERN(LogDC, Log, All);
DREAMCATCHER_API DECLARE_LOG_CATEGORY_EXTERN(LogDCExperience, Log, All);
DREAMCATCHER_API DECLARE_LOG_CATEGORY_EXTERN(LogDCAbilitySystem, Log, All);
DREAMCATCHER_API DECLARE_LOG_CATEGORY_EXTERN(LogDCTeams, Log, All);

DREAMCATCHER_API FString GetClientServerContextString(UObject* ContextObject = nullptr);