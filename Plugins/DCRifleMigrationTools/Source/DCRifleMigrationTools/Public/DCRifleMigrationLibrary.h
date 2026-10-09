#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DCRifleMigrationLibrary.generated.h"

class UBlueprint;

/** Copy mechanics only; this is NOT proof of Blueprint, reference-closure, or gameplay correctness. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCRifleCopyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bPreflightPassed = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bCopyInvoked = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bNativeCopyReportedSuccess = false;

	/** Bounded engine duplication/reference update; never global Advanced Copy consolidation. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bScopedCopyUsed = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bOriginalBlueprintsUnchanged = false;

	/** No ensure or error log during this native operation; required before any save. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bEngineDiagnosticsClean = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bInheritedOverridesRequested = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bInheritedOverridesVerified = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	int32 InheritedOverrideCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bKnownLoadDirtyPolicyRequested = false;

	/** All executed dirty/file checks passed; false also means not checked or not requested. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bKnownLoadDirtyPolicyVerified = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> ObservedLoadDirtyDependencies;

	/** Explicit source packages and their known companion files only, not every loaded dependency. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bSourceFilesUnchanged = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bDestinationsVerified = false;

	/** True only when all requested saved copies passed the checks. False does not rule out partial files. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bSavedToDisk = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> Destinations;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> Messages;
};

/** Read-only asset comparison; no repairs or saves. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCRifleOverrideComparison
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	int32 ExpectedOverrides = 0;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	int32 ComparedOverrides = 0;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> Messages;
};

/** Exact macro graph/owner, cast types and pin subtypes via public C++ getters. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCRifleTypeReferenceReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	bool bSucceeded = false;

	/** Same-process identity/status/dirty token for original guards, not copy-to-original comparison. */
	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	FString BlueprintState;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> References;

	UPROPERTY(BlueprintReadOnly, Category="Rifle Copy Diagnostics")
	TArray<FString> Messages;
};

/** Value-free reflected contract row. Type is a structural signature, never a CDO value. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCRifleNativeContractRow
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Kind;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Context;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Owner;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Name;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Type;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString Flags;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") int32 Index = INDEX_NONE;
};

/** Query success means a bounded reflection report was produced, not gameplay compatibility. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCRifleNativeContractReport
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") bool bSucceeded = false;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") FString SubjectPath;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") TArray<FDCRifleNativeContractRow> Rows;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") TArray<FString> ReferencedTypes;
	UPROPERTY(BlueprintReadOnly, Category="Rifle Native Audit") TArray<FString> Messages;
};

/** UI-only value-bearing rows. These do not change the value-free native audit contract. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCUIWidgetTemplateRead
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString Path;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString Name;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ClassPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ParentPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString SlotPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString SlotClassPath;
};

USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCUIObjectFieldRead
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString Name;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString OwnerPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString DeclaredClassPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ValuePath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ValueClassPath;
	/** null is an observed CDO value, NOT a missing runtime BindWidget assignment. */
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString Status;
};

USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCUIGeneratedTreeRead
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ClassPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString TreePath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString RootPath;
};

USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCUICommentRead
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString Path;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") int32 PinCount = 0;
};

/** Bounded loaded-object evidence only; no runtime binding, rendering or copy compatibility claim. */
USTRUCT(BlueprintType)
struct DCRIFLEMIGRATIONTOOLS_API FDCUIBlueprintReadReport
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") bool bSucceeded = false;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") bool bInspectionStarted = false;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") bool bReadOnlyStatePreserved = false;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString SubjectPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString SourceTreePath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString SourceRootPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString GeneratedClassPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString ExistingCDOPath;
	/** First already-loaded generated class with a non-null tree root; empty if none observed. */
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") FString TreeOwnerClassPath;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") TArray<FDCUIGeneratedTreeRead> GeneratedTrees;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") TArray<FDCUIWidgetTemplateRead> Widgets;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") TArray<FDCUIObjectFieldRead> Fields;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") TArray<FDCUICommentRead> Comments;
	UPROPERTY(BlueprintReadOnly, Category="UI Read Audit") TArray<FString> Messages;
};

/** This library exists only in an Editor module, with no dependency on either game's runtime module. */
UCLASS()
class DCRIFLEMIGRATIONTOOLS_API UDCRifleMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Guard process/SCC/compile-save settings before read-only reload/inspection too. */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static bool ValidateDiagnosticContext(FString& OutReport);

	/** Monotonic process counter, never reset by this tool; Python checks every stage/save boundary. */
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Migration Diagnostics")
	static int64 GetEngineEnsureFailureCount();

	/** Fixed, approved core bundle; read-only capability/version check for the staging script. */
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Migration Diagnostics")
	static TArray<FString> GetApprovedRifleCorePackages();

	/** Exact core16 + original audio macro/function libraries (18); not arbitrary extra dependencies. */
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Migration Diagnostics")
	static TArray<FString> GetApprovedRifleClosurePackages();

	/** Original sibling sentinels to fingerprint/load before duplication, never a dirty allowlist. */
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Migration Diagnostics")
	static TArray<FString> GetProtectedRifleSiblingPackages();

	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCRifleTypeReferenceReport InspectBlueprintTypeReferences(UBlueprint* Blueprint);

	/** Already-loaded approved asset22 only; no Load/Compile/Save, no property values/functions executed.
	 * Production: original diagnostic commandlet guard plus -DCRifleNativeContractAudit.
	 * Synthetic tests: unattended Editor + -DCRifleNativeContractAudit -DCRifleNativeContractTests,
	 * transient packages only. Existing copy policy/report formats are unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCRifleNativeContractReport InspectAssetNativeContract(UObject* Asset);

	/** Inspect an already-loaded /Script class, script struct or enum (including inherited reflected members).
	 * Reflection cannot establish non-UFUNCTION C++ behavior or native implementation equivalence.
	 */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCRifleNativeContractReport InspectNativeTypeContract(UObject* NativeType);

	/** Already-loaded fixed source WidgetBlueprint8 only: original diagnostic commandlet +
	 * -DCR5UISourceAudit. Synthetic fixtures: separate unattended/nullrhi DreamCatcher Editor
	 * + -DCR5UIContractTests, uniquely named transient unsaved packages only.
	 * Never loads, compiles, initializes widgets, saves, copies or changes property flags.
	 */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCUIBlueprintReadReport InspectUIBlueprintReadOnly(UObject* Asset);

	/** Fixed fingerprint/dirty list shared with the core staging save guard. */
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Migration Diagnostics")
	static TArray<FString> GetKnownRifleLoadDirtyPackages();

	/** Read-only preflight. Sources/destinations are long package names, not file or object paths.
	 * Requires an unattended commandlet with -DCRifleCopyDiagnostics and disabled source control.
	 * All destinations must be in a new /Game/LyraMigration/Rifle/Diagnostics/Explicit/<RunName>/ folder.
	 */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static bool ValidateCopyPlan(const FString& RunName, const TMap<FString, FString>& SourceToDestination, FString& OutReport);

	/** Copies ONLY the explicit map (up to 16, or the exact approved 18); no automatic expansion.
	 * Dependencies may be loaded by Unreal, but are not automatically copied.
	 * bSaveCopies=false is memory-only and is never migration-ready. A fresh run name is required for every attempt.
	 * Engine crashes are not caught; partial outputs must be preserved and inspected separately.
	 * PreserveInheritedOverrides captures original SCS overrides, restores them against the copied
	 * parent, compiles/compares, and only then saves. Unsupported override kinds reject before copy.
	 * Uses DuplicateSingleObject and destination-only archives, never global ConsolidateObjects.
	 * AllowKnownLoadDirtyDependencies is an opt-in for original Lyra Actor pair or exact core16/closure18.
	 * Also requires -DCRifleAllowKnownLoadDirty and preservation. Initially dirty packages are never
	 * accepted. Four fixed load-dirty dependencies stay unsaved and must retain their file hashes.
	 */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCRifleCopyResult CopyPackageSubset(const FString& RunName, const TMap<FString, FString>& SourceToDestination, bool bSaveCopies = true, bool bPreserveInheritedOverrides = false, bool bAllowKnownLoadDirtyDependencies = false);

	/** Recheck saved or in-memory copies against original SCS override properties. No repair or save. */
	UFUNCTION(BlueprintCallable, Category="DreamCatcher|Migration Diagnostics")
	static FDCRifleOverrideComparison CompareInheritedOverrides(const FString& RunName, const TMap<FString, FString>& SourceToDestination);
};
