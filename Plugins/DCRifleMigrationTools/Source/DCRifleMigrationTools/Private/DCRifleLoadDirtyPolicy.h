#pragma once

#include "CoreMinimal.h"
#include "Misc/SecureHash.h"

namespace DCRifleMigration
{
// Fixed observational allowlist; callers cannot add packages through Python.
const TSet<FName>& KnownRifleLoadDirtyPackages();
const TArray<FString>& ApprovedRifleCorePackages();
const TArray<FString>& ApprovedRifleClosurePackages();
const TArray<FString>& ProtectedRifleSiblingPackages();
bool IsExactRifleClosure(const TMap<FString, FString>& PackageMap);

// Pure decisions shared with tests. File state checks are required separately.
bool ValidateLoadDirtyScope(const FString& ProjectName, bool bPreserveOverrides,
	const TMap<FString, FString>& PackageMap, FString& Error);
bool EvaluateLoadDirtyPackages(const TArray<FName>& InitialDirty, const TArray<FName>& CurrentDirty,
	const TSet<FName>& Sources, const TSet<FName>& Destinations, bool bCopiesCreated,
	TArray<FString>& ObservedDependencies, FString& Error);
bool IsTrackedFileUnchanged(bool bExistedBefore, const FMD5Hash& Before,
	bool bExistsNow, const FMD5Hash& Now);

// Never clears dirty flags, modifies objects, or saves files. Begin must precede source loading.
class FRifleLoadDirtyPolicy
{
public:
	bool Begin(const FString& ProjectName, bool bPreserveOverrides,
		const TMap<FString, FString>& PackageMap, TArray<FString>& Messages);
	bool Verify(bool bCopiesCreated, const TCHAR* Phase,
		TArray<FString>& ObservedDependencies, TArray<FString>& Messages) const;

private:
	bool bInitialized = false;
	TArray<FName> InitialDirty;
	TSet<FName> Sources;
	TSet<FName> Destinations;
	// Invalid hash records absence. A present but unreadable file is always a failure.
	TMap<FString, FMD5Hash> OriginalFiles;
};
}
