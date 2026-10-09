#pragma once

#include "CoreMinimal.h"
#include "DCRifleMigrationLibrary.h"
#include "Misc/SecureHash.h"
#include "UObject/StrongObjectPtr.h"

class UBlueprint;

namespace DCRifleMigration
{
// Public engine graph/pin APIs only. No protected-property reflection or graph edits.
FDCRifleTypeReferenceReport ReadTypeReferences(UBlueprint* Blueprint);

// Captured AFTER loading and BEFORE duplication. Never clears dirty flags or restores originals.
// Holds original classes alive so recompilation cannot hide a changed identity.
class FOriginalBlueprintGuard
{
public:
	bool Capture(TArray<FString>& Messages);
	bool Verify(TArray<FString>& Messages) const;

private:
	bool bInitialized = false;
	struct FEntry
	{
		TStrongObjectPtr<UBlueprint> Blueprint;
		TStrongObjectPtr<UClass> Parent;
		TStrongObjectPtr<UClass> Generated;
		TStrongObjectPtr<UClass> Skeleton;
		uint8 Status = 0;
		bool bDirty = false;
		TArray<FString> TypeReferences;
	};
	TArray<FEntry> Entries;
	TMap<FString, FMD5Hash> Files;
};

// No save, consolidation, global reference replacement, asset deletion, or redirects.
// Caller performs preflight and keeps Guard alive through subsequent repair/save gates.
bool CopyScoped(const TMap<FString, FString>& PackageMap, FOriginalBlueprintGuard& Guard,
	TArray<FString>& Messages);
bool CompareTypeReferences(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages);
}
