#pragma once

#include "CoreMinimal.h"

namespace DCRifleMigration
{
inline constexpr int32 MaxPackagesPerRun = 16;
inline constexpr const TCHAR* DiagnosticsRoot = TEXT("/Game/LyraMigration/Rifle/Diagnostics/Explicit");

// Pure name validation for the bridge and non-mutating automation tests.
bool ValidateNames(const FString& RunName, const TMap<FString, FString>& SourceToDestination,
	FString& OutRunRoot, FString& OutError);
}
