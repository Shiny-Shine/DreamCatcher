#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/StrongObjectPtr.h"

class UBlueprint;
class USCS_Node;

namespace DCRifleMigration
{
// Captured before native Advanced Copy can recompile/reparent the copies.
// Values are immutable text, including owned subobjects, not pointers to the repaired objects.
struct FInheritedOverrideSnapshot
{
	FString SourcePackage;
	FString OwnerPackage;
	FGuid NodeGuid;
	FName VariableName;
	TStrongObjectPtr<UActorComponent> OriginalTemplate;
	TStrongObjectPtr<UActorComponent> Template;
	TMap<FString, FString> Properties;
};

class FInheritedOverrideState
{
public:
	bool Capture(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages);
	bool RestoreAndCompile(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages);
	bool Compare(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages, int32& OutCompared) const;
	int32 Num() const { return Snapshots.Num(); }

private:
	TArray<FInheritedOverrideSnapshot> Snapshots;
	TMap<FString, int32> ExpectedCounts;
};
}
