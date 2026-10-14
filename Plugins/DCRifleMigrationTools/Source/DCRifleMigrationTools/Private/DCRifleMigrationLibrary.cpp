#include "DCRifleMigrationLibrary.h"

#include "AssetToolsModule.h"
#include "BlueprintEditorSettings.h"
#include "CoreGlobals.h"
#include "DCRifleMigrationValidation.h"
#include "DCRifleInheritedOverrides.h"
#include "DCRifleLoadDirtyPolicy.h"
#include "DCRifleScopedCopy.h"
#include "DCRifleDiagnosticGuard.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"
#include "Misc/App.h"
#include "Misc/AssertionMacros.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogDCRifleMigration, Log, All);

namespace DCRifleMigration
{
bool ValidateNames(const FString& RunName, const TMap<FString, FString>& SourceToDestination,
	FString& OutRunRoot, FString& OutError)
{
	OutRunRoot.Reset();
	OutError.Reset();
	if (RunName.IsEmpty() || RunName.Len() > 48)
	{
		OutError = TEXT("RunName must contain 1-48 ASCII letters, digits, or underscores.");
		return false;
	}
	for (TCHAR Character : RunName)
	{
		if (!((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z')
			|| (Character >= '0' && Character <= '9') || Character == '_'))
		{
			OutError = TEXT("RunName must contain only ASCII letters, digits, or underscores.");
			return false;
		}
	}
	if (SourceToDestination.IsEmpty() || (SourceToDestination.Num() > MaxPackagesPerRun && !IsExactRifleClosure(SourceToDestination)))
	{
		OutError = FString::Printf(TEXT("Provide 1-%d explicit packages, or the exact approved closure18."), MaxPackagesPerRun);
		return false;
	}
	OutRunRoot = FString(DiagnosticsRoot) / RunName;
	TSet<FName> SeenSources;
	TSet<FName> SeenDestinations;
	for (const TPair<FString, FString>& Pair : SourceToDestination)
	{
		FText Reason;
		// This is a lexical/allowlist check, not a check of this process's mounted plugins.
		// In particular, DreamCatcher's pure tests do not mount the original ShooterCore.
		if (!FPackageName::IsValidTextForLongPackageName(Pair.Key, &Reason)
			|| Pair.Key.Contains(TEXT("//"))
			|| !(Pair.Key.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive)
				|| Pair.Key.StartsWith(TEXT("/ShooterCore/"), ESearchCase::CaseSensitive)))
		{
			OutError = FString::Printf(TEXT("Source must be a valid /Game or /ShooterCore package: %s (%s)"), *Pair.Key, *Reason.ToString());
			return false;
		}
		if (!FPackageName::IsValidTextForLongPackageName(Pair.Value, &Reason)
			|| !FPaths::ValidatePath(Pair.Value, &Reason) || Pair.Value.Contains(TEXT("//"))
			|| !Pair.Value.StartsWith(OutRunRoot + TEXT("/"), ESearchCase::CaseSensitive)
			|| Pair.Key.StartsWith(OutRunRoot + TEXT("/"), ESearchCase::IgnoreCase))
		{
			OutError = FString::Printf(TEXT("Destination must be inside the new run folder, with source outside it: %s (%s)"), *Pair.Value, *Reason.ToString());
			return false;
		}
		const FName SourceName(*Pair.Key);
		const FName DestinationName(*Pair.Value);
		if (SeenSources.Contains(SourceName) || SeenDestinations.Contains(DestinationName))
		{
			OutError = TEXT("Source and destination package names must each be unique, including case-insensitive aliases.");
			return false;
		}
		SeenSources.Add(SourceName);
		SeenDestinations.Add(DestinationName);
	}
	return true;
}

struct FPreflight
{
	TMap<FString, FString> DestinationFiles;
	TMap<FString, FMD5Hash> SourceHashes;
};

bool VerifyDirtyBoundary(const TMap<FString, FString>& Map, bool bAllowKnown, TArray<FString>& Messages)
{
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyPackages(Dirty);
	for (UPackage* Package : Dirty)
	{
		const FString Name = Package->GetName();
		if (Map.Contains(Name) || (!Map.FindKey(Name)
			&& !(bAllowKnown && KnownRifleLoadDirtyPackages().Contains(Package->GetFName()))))
		{
			Messages.Add(TEXT("Dirty package outside destination/fixed load boundary: ") + Name);
			return false;
		}
	}
	return true;
}

bool CheckExecutionContext(FString& Error)
{
	if (!IsInGameThread() || !IsRunningCommandlet() || !FApp::IsUnattended()
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCRifleCopyDiagnostics")))
	{
		Error = TEXT("Use an unattended commandlet on the game thread with -DCRifleCopyDiagnostics; never the active editor or PIE.");
		return false;
	}
	if (FDebug::GetNumEnsureFailures() != 0)
	{
		Error = TEXT("An engine ensure already occurred in this process. Stop; do not copy or save. Use a fresh process only after diagnosis.");
		return false;
	}
	const FString ProjectName = FApp::GetProjectName();
	if (ProjectName != TEXT("LyraStarterGame") && ProjectName != TEXT("DreamCatcher"))
	{
		Error = TEXT("Only the LyraStarterGame and DreamCatcher projects are permitted.");
		return false;
	}
	// Never change process/user save settings. Individual duplication also compiles BPs internally.
	if (GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never)
	{
		Error = TEXT("Save on Compile must be Never for memory-only diagnostics; no settings were changed.");
		return false;
	}
	if (ISourceControlModule::Get().IsEnabled())
	{
		Error = TEXT("Source control must be disabled in this process (-SCCProvider=None -nop4).");
		return false;
	}
	return true;
}

bool Preflight(const FString& RunName, const TMap<FString, FString>& Map, FPreflight& Out, FString& Error,
	const TSet<FName>* VerifiedLoadDirtyDependencies = nullptr)
{
	if (!CheckExecutionContext(Error))
	{
		return false;
	}
	FString RunRoot;
	if (!ValidateNames(RunName, Map, RunRoot, Error))
	{
		return false;
	}
	FString RunDirectory;
	if (!FPackageName::TryConvertLongPackageNameToFilename(RunRoot, RunDirectory))
	{
		Error = TEXT("Run folder could not be resolved through the /Game mount.");
		return false;
	}
	RunDirectory = FPaths::ConvertRelativePathToFull(RunDirectory);
	const FString ContentDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	if (!FPaths::IsUnderDirectory(RunDirectory, ContentDirectory)
		|| IFileManager::Get().DirectoryExists(*RunDirectory) || IFileManager::Get().FileExists(*RunDirectory))
	{
		Error = TEXT("Run folder must be new and resolve inside this project's Content directory.");
		return false;
	}
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		if (It->GetName().Equals(RunRoot, ESearchCase::IgnoreCase)
			|| It->GetName().StartsWith(RunRoot + TEXT("/"), ESearchCase::IgnoreCase))
		{
			Error = TEXT("Run folder already contains an in-memory package. Use a new run name/process.");
			return false;
		}
	}
	TArray<UPackage*> DirtyPackages;
	FEditorFileUtils::GetDirtyPackages(DirtyPackages);
	for (UPackage* Package : DirtyPackages)
	{
		if (!VerifiedLoadDirtyDependencies || !VerifiedLoadDirtyDependencies->Contains(Package->GetFName()))
		{
			Error = FString::Printf(TEXT("Refusing to operate with dirty package: %s. No original is automatically saved or marked clean."), *GetNameSafe(Package));
			return false;
		}
	}
	FAssetToolsModule::GetModule(); // Registers AssetTools CVars; no copy/migration is performed here.
	const IConsoleVariable* HeaderPatching = IConsoleManager::Get().FindConsoleVariable(TEXT("AssetTools.UseHeaderPatchingAdvancedCopy"));
	if (!HeaderPatching || HeaderPatching->GetInt() != 0)
	{
		Error = TEXT("Header patching must be disabled. This bridge uses bounded full-load duplication.");
		return false;
	}
	for (const TPair<FString, FString>& Pair : Map)
	{
		// Real operations still require registered, writable mounts on BOTH sides.
		// Do this before resolving or loading any package; pure name tests need no mounts.
		FText MountReason;
		if (!FPackageName::IsValidLongPackageName(Pair.Key, false, &MountReason))
		{
			Error = FString::Printf(TEXT("Source mount/name is invalid in this process: %s (%s)"), *Pair.Key, *MountReason.ToString());
			return false;
		}
		if (!FPackageName::IsValidLongPackageName(Pair.Value, false, &MountReason))
		{
			Error = FString::Printf(TEXT("Destination mount/name is invalid in this process: %s (%s)"), *Pair.Value, *MountReason.ToString());
			return false;
		}
		FString SourceFile;
		if (!FPackageName::DoesPackageExist(Pair.Key, &SourceFile)
			|| !SourceFile.EndsWith(FPackageName::GetAssetPackageExtension(), ESearchCase::IgnoreCase))
		{
			Error = FString::Printf(TEXT("Source must be an existing .uasset package, not a map: %s"), *Pair.Key);
			return false;
		}
		if (UPackage* LoadedSource = FindPackage(nullptr, *Pair.Key))
		{
			if (LoadedSource->IsDirty() || LoadedSource->ContainsMap())
			{
				Error = FString::Printf(TEXT("Source is dirty or contains a map: %s"), *Pair.Key);
				return false;
			}
		}
		FString DestinationFile;
		if (!FPackageName::TryConvertLongPackageNameToFilename(Pair.Value, DestinationFile, FPackageName::GetAssetPackageExtension()))
		{
			Error = FString::Printf(TEXT("Cannot resolve destination: %s"), *Pair.Value);
			return false;
		}
		DestinationFile = FPaths::ConvertRelativePathToFull(DestinationFile);
		if (!FPaths::IsUnderDirectory(DestinationFile, RunDirectory)
			|| FPackageName::DoesPackageExist(Pair.Value) || FindPackage(nullptr, *Pair.Value))
		{
			Error = FString::Printf(TEXT("Destination exists or escapes the run folder: %s"), *Pair.Value);
			return false;
		}
		for (const TCHAR* Extension : {TEXT("uasset"), TEXT("umap"), TEXT("uexp"), TEXT("ubulk"), TEXT("uptnl")})
		{
			if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(DestinationFile, Extension)))
			{
				Error = FString::Printf(TEXT("Destination or companion file already exists: %s"), *Pair.Value);
				return false;
			}
		}
		Out.DestinationFiles.Add(Pair.Value, DestinationFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		for (const TCHAR* Extension : {TEXT("uasset"), TEXT("uexp"), TEXT("ubulk"), TEXT("uptnl")})
		{
			const FString File = FPaths::ChangeExtension(SourceFile, Extension);
			FMD5Hash Hash; // An invalid hash records that this companion did not exist.
			if (IFileManager::Get().FileExists(*File))
			{
				Hash = FMD5Hash::HashFile(*File);
				if (!Hash.IsValid())
				{
					Error = FString::Printf(TEXT("Cannot hash source file: %s"), *File);
					return false;
				}
			}
			Out.SourceHashes.Add(File, Hash);
		}
	}
	return true;
}
}

bool UDCRifleMigrationLibrary::ValidateDiagnosticContext(FString& OutReport)
{
	if (!DCRifleMigration::CheckExecutionContext(OutReport)) { return false; }
	OutReport = TEXT("Diagnostic commandlet context passed; no assets loaded or saved.");
	return true;
}

int64 UDCRifleMigrationLibrary::GetEngineEnsureFailureCount()
{
	return static_cast<int64>(FDebug::GetNumEnsureFailures());
}

TArray<FString> UDCRifleMigrationLibrary::GetApprovedRifleCorePackages()
{
	return DCRifleMigration::ApprovedRifleCorePackages();
}

TArray<FString> UDCRifleMigrationLibrary::GetKnownRifleLoadDirtyPackages()
{
	TArray<FString> Packages;
	for (FName Package : DCRifleMigration::KnownRifleLoadDirtyPackages())
	{
		Packages.Add(Package.ToString());
	}
	Packages.Sort();
	return Packages;
}

TArray<FString> UDCRifleMigrationLibrary::GetApprovedRifleClosurePackages()
{
	return DCRifleMigration::ApprovedRifleClosurePackages();
}

TArray<FString> UDCRifleMigrationLibrary::GetProtectedRifleSiblingPackages()
{
	return DCRifleMigration::ProtectedRifleSiblingPackages();
}

FDCRifleTypeReferenceReport UDCRifleMigrationLibrary::InspectBlueprintTypeReferences(UBlueprint* Blueprint)
{
	FString Error;
	if (!DCRifleMigration::CheckExecutionContext(Error))
	{
		FDCRifleTypeReferenceReport Result;
		Result.Messages.Add(Error);
		return Result;
	}
	return DCRifleMigration::ReadTypeReferences(Blueprint);
}

bool UDCRifleMigrationLibrary::ValidateCopyPlan(const FString& RunName, const TMap<FString, FString>& SourceToDestination, FString& OutReport)
{
	DCRifleMigration::FPreflight State;
	const bool bValid = DCRifleMigration::Preflight(RunName, SourceToDestination, State, OutReport);
	if (bValid)
	{
		OutReport = FString::Printf(TEXT("Preflight passed for %d explicit packages; no assets copied or saved."), SourceToDestination.Num());
	}
	UE_LOG(LogDCRifleMigration, Display, TEXT("Preflight %s: %s"), bValid ? TEXT("passed") : TEXT("rejected"), *OutReport);
	return bValid;
}

FDCRifleCopyResult UDCRifleMigrationLibrary::CopyPackageSubset(const FString& RunName, const TMap<FString, FString>& SourceToDestination, bool bSaveCopies, bool bPreserveInheritedOverrides, bool bAllowKnownLoadDirtyDependencies)
{
	DCRifleMigration::FEngineDiagnosticGuard Diagnostics;
	FDCRifleCopyResult Result;
	if (!Diagnostics.Check(TEXT("copy_entry"), Result.Messages)) { return Result; }
	Result.bInheritedOverridesRequested = bPreserveInheritedOverrides;
	Result.bKnownLoadDirtyPolicyRequested = bAllowKnownLoadDirtyDependencies;
	DCRifleMigration::FPreflight State;
	FString Error;
	Result.bPreflightPassed = DCRifleMigration::Preflight(RunName, SourceToDestination, State, Error);
	if (!Result.bPreflightPassed)
	{
		Result.Messages.Add(Error);
		UE_LOG(LogDCRifleMigration, Warning, TEXT("Copy rejected: %s"), *Error);
		return Result;
	}
	if (DCRifleMigration::IsExactRifleClosure(SourceToDestination)
		&& (!bPreserveInheritedOverrides || !bAllowKnownLoadDirtyDependencies))
	{
		Result.Messages.Add(TEXT("Closure18 requires inherited-override preservation and the fixed load-dirty/source-file policy."));
		return Result;
	}
	DCRifleMigration::FRifleLoadDirtyPolicy LoadDirtyPolicy;
	if (bAllowKnownLoadDirtyDependencies)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCRifleAllowKnownLoadDirty")))
		{
			Result.Messages.Add(TEXT("Explicit command-line opt-in -DCRifleAllowKnownLoadDirty is required."));
			return Result;
		}
		if (!LoadDirtyPolicy.Begin(FApp::GetProjectName(), bPreserveInheritedOverrides, SourceToDestination, Result.Messages))
		{
			return Result;
		}
	}
	DCRifleMigration::FInheritedOverrideState Overrides;
	if (DCRifleMigration::IsExactRifleClosure(SourceToDestination))
	{
		// These are watched ORIGINALS, not copies or dirty exceptions. Baseline files were
		// fingerprinted by Begin above before any source or sibling was loaded.
		for (const FString& Package : DCRifleMigration::ProtectedRifleSiblingPackages())
		{
			if (!LoadObject<UObject>(nullptr, *(Package + TEXT(".") + FPackageName::GetShortName(Package))))
			{
				Result.Messages.Add(TEXT("Cannot load protected original sibling: ") + Package);
				return Result;
			}
			if (!Diagnostics.Check(TEXT("after_sibling_load"), Result.Messages)) { return Result; }
		}
	}
	if (bPreserveInheritedOverrides)
	{
		if (!Overrides.Capture(SourceToDestination, Result.Messages))
		{
			return Result;
		}
		if (!Diagnostics.Check(TEXT("after_override_capture"), Result.Messages)) { return Result; }
		Result.InheritedOverrideCount = Overrides.Num();
		if (bAllowKnownLoadDirtyDependencies)
		{
			Result.bKnownLoadDirtyPolicyVerified = LoadDirtyPolicy.Verify(false, TEXT("after_capture"),
				Result.ObservedLoadDirtyDependencies, Result.Messages);
			if (!Result.bKnownLoadDirtyPolicyVerified)
			{
				return Result;
			}
		}
		// Loading originals for capture can itself reveal dirty/upgraded packages.
		DCRifleMigration::FPreflight Recheck;
		if (!DCRifleMigration::Preflight(RunName, SourceToDestination, Recheck, Error,
			bAllowKnownLoadDirtyDependencies ? &DCRifleMigration::KnownRifleLoadDirtyPackages() : nullptr))
		{
			Result.Messages.Add(Error);
			return Result;
		}
	}
	SourceToDestination.GenerateValueArray(Result.Destinations);
	Result.Destinations.Sort();
	for (const TPair<FString, FString>& Pair : SourceToDestination)
	{
		UE_LOG(LogDCRifleMigration, Display, TEXT("Explicit copy: %s -> %s"), *Pair.Key, *Pair.Value);
	}
	for (const TPair<FString, FMD5Hash>& Pair : State.SourceHashes)
	{
		const FString Before = Pair.Value.IsValid() ? LexToString(Pair.Value) : TEXT("absent");
		UE_LOG(LogDCRifleMigration, Display, TEXT("Explicit source before: %s md5=%s"), *Pair.Key, *Before);
	}
	Result.bCopyInvoked = true;
	Result.bScopedCopyUsed = true;
	DCRifleMigration::FOriginalBlueprintGuard OriginalGuard;
	UE_LOG(LogDCRifleMigration, Display, TEXT("Scoped memory copy begin: run=%s packages=%d"), *RunName, SourceToDestination.Num());
	Result.bNativeCopyReportedSuccess = DCRifleMigration::CopyScoped(SourceToDestination, OriginalGuard, Result.Messages);
	Result.bEngineDiagnosticsClean = Diagnostics.Check(TEXT("after_scoped_copy"), Result.Messages);
	if (bPreserveInheritedOverrides && Result.bNativeCopyReportedSuccess && Result.bEngineDiagnosticsClean)
	{
		Result.bInheritedOverridesVerified = Overrides.RestoreAndCompile(SourceToDestination, Result.Messages);
	}
	Result.bEngineDiagnosticsClean &= Diagnostics.Check(TEXT("after_override_restore"), Result.Messages);
	Result.bOriginalBlueprintsUnchanged = OriginalGuard.Verify(Result.Messages);
	bool bDirtyBoundaryPassed = DCRifleMigration::VerifyDirtyBoundary(SourceToDestination,
		bAllowKnownLoadDirtyDependencies, Result.Messages);
	if (Result.bNativeCopyReportedSuccess)
	{
		Result.bNativeCopyReportedSuccess = DCRifleMigration::CompareTypeReferences(SourceToDestination, Result.Messages);
	}
	if (bAllowKnownLoadDirtyDependencies)
	{
		const bool bCurrentCheck = LoadDirtyPolicy.Verify(true, TEXT("after_copy_and_repair"),
			Result.ObservedLoadDirtyDependencies, Result.Messages);
		Result.bKnownLoadDirtyPolicyVerified &= bCurrentCheck;
	}

	Result.bSourceFilesUnchanged = true;
	for (const TPair<FString, FMD5Hash>& Pair : State.SourceHashes)
	{
		const bool bExistsNow = IFileManager::Get().FileExists(*Pair.Key);
		const FMD5Hash After = bExistsNow ? FMD5Hash::HashFile(*Pair.Key) : FMD5Hash();
		if (bExistsNow != Pair.Value.IsValid() || (bExistsNow && (!After.IsValid() || After != Pair.Value)))
		{
			Result.bSourceFilesUnchanged = false;
			Result.Messages.Add(TEXT("Source hash changed or became unavailable: ") + Pair.Key);
		}
		const FString AfterText = After.IsValid() ? LexToString(After) : (bExistsNow ? TEXT("unreadable") : TEXT("absent"));
		UE_LOG(LogDCRifleMigration, Display, TEXT("Explicit source after: %s md5=%s"), *Pair.Key, *AfterText);
	}
	bool bDeferredSaveSucceeded = true;
	if (bSaveCopies)
	{
		bDeferredSaveSucceeded = false;
		if (Result.bNativeCopyReportedSuccess && (!bPreserveInheritedOverrides || Result.bInheritedOverridesVerified)
			&& Result.bEngineDiagnosticsClean && Result.bOriginalBlueprintsUnchanged && bDirtyBoundaryPassed && Result.bSourceFilesUnchanged
			&& (!bAllowKnownLoadDirtyDependencies || Result.bKnownLoadDirtyPolicyVerified))
		{
			TArray<UPackage*> CopiesToSave;
			bool bSafeToSave = true;
			for (const TPair<FString, FString>& Pair : State.DestinationFiles)
			{
				UPackage* Package = FindPackage(nullptr, *Pair.Key);
				if (!Package || Package->ContainsMap())
				{
					bSafeToSave = false;
					break;
				}
				for (const TCHAR* Extension : {TEXT("uasset"), TEXT("umap"), TEXT("uexp"), TEXT("ubulk"), TEXT("uptnl")})
				{
					bSafeToSave &= !IFileManager::Get().FileExists(*FPaths::ChangeExtension(Pair.Value, Extension));
				}
				CopiesToSave.Add(Package);
			}
			if (bSafeToSave && !ISourceControlModule::Get().IsEnabled())
			{
				if (bAllowKnownLoadDirtyDependencies)
				{
					const bool bCurrentCheck = LoadDirtyPolicy.Verify(true, TEXT("before_save"),
						Result.ObservedLoadDirtyDependencies, Result.Messages);
					Result.bKnownLoadDirtyPolicyVerified &= bCurrentCheck;
				}
				Result.bOriginalBlueprintsUnchanged &= OriginalGuard.Verify(Result.Messages);
				bDirtyBoundaryPassed &= DCRifleMigration::VerifyDirtyBoundary(SourceToDestination, bAllowKnownLoadDirtyDependencies, Result.Messages);
				Result.bEngineDiagnosticsClean &= Diagnostics.Check(TEXT("immediately_before_save"), Result.Messages);
				if (Result.bEngineDiagnosticsClean && Result.bOriginalBlueprintsUnchanged && bDirtyBoundaryPassed && (!bAllowKnownLoadDirtyDependencies || Result.bKnownLoadDirtyPolicyVerified))
				{
					bDeferredSaveSucceeded = UEditorLoadingAndSavingUtils::SavePackages(CopiesToSave, false);
					Result.bEngineDiagnosticsClean &= Diagnostics.Check(TEXT("after_save"), Result.Messages);
				}
			}
		}
		if (!bDeferredSaveSucceeded)
		{
			Result.Messages.Add(TEXT("Preservation/save gate failed. No intentional save before successful override comparison; preserve any partial diagnostics."));
		}
		// Recheck after the deferred save as well as after native copy/repair.
		for (const TPair<FString, FMD5Hash>& Pair : State.SourceHashes)
		{
			const bool bExists = IFileManager::Get().FileExists(*Pair.Key);
			const FMD5Hash Hash = bExists ? FMD5Hash::HashFile(*Pair.Key) : FMD5Hash();
			if (bExists != Pair.Value.IsValid() || (bExists && (!Hash.IsValid() || Hash != Pair.Value)))
			{
				Result.bSourceFilesUnchanged = false;
				Result.Messages.Add(TEXT("Source changed during deferred save: ") + Pair.Key);
			}
		}
		if (bAllowKnownLoadDirtyDependencies)
		{
			const bool bCurrentCheck = LoadDirtyPolicy.Verify(true, TEXT("after_save_attempt"),
				Result.ObservedLoadDirtyDependencies, Result.Messages);
			Result.bKnownLoadDirtyPolicyVerified &= bCurrentCheck;
		}
		Result.bOriginalBlueprintsUnchanged &= OriginalGuard.Verify(Result.Messages);
		bDirtyBoundaryPassed &= DCRifleMigration::VerifyDirtyBoundary(SourceToDestination, bAllowKnownLoadDirtyDependencies, Result.Messages);
	}
	Result.bDestinationsVerified = true;
	for (const TPair<FString, FString>& Pair : State.DestinationFiles)
	{
		UPackage* Package = FindPackage(nullptr, *Pair.Key);
		UObject* Asset = Package ? FindObject<UObject>(Package, *FPackageName::GetShortName(Pair.Key)) : nullptr;
		const bool bNonEmptyAssetFile = IFileManager::Get().FileSize(*Pair.Value) > 0;
		bool bAnyOutputFile = false;
		for (const TCHAR* Extension : {TEXT("uasset"), TEXT("umap"), TEXT("uexp"), TEXT("ubulk"), TEXT("uptnl")})
		{
			bAnyOutputFile |= IFileManager::Get().FileExists(*FPaths::ChangeExtension(Pair.Value, Extension));
		}
		if (!IsValid(Asset) || (bSaveCopies && (!bNonEmptyAssetFile || Package->IsDirty())) || (!bSaveCopies && bAnyOutputFile))
		{
			Result.bDestinationsVerified = false;
			Result.Messages.Add(TEXT("Destination memory/file state is not as requested: ") + Pair.Key);
		}
	}
	Result.bEngineDiagnosticsClean &= Diagnostics.Check(TEXT("copy_result"), Result.Messages);
	Result.bSucceeded = Result.bEngineDiagnosticsClean && Result.bNativeCopyReportedSuccess && Result.bSourceFilesUnchanged && Result.bDestinationsVerified
		&& Result.bOriginalBlueprintsUnchanged && bDirtyBoundaryPassed
		&& bDeferredSaveSucceeded && (!bPreserveInheritedOverrides || Result.bInheritedOverridesVerified)
		&& (!bAllowKnownLoadDirtyDependencies || Result.bKnownLoadDirtyPolicyVerified);
	Result.bSavedToDisk = bSaveCopies && Result.bSucceeded;
	Result.Messages.Add(bSaveCopies
		? TEXT("Disk checks are not fresh-process reload, dependency-closure, or Blueprint validation. Inspect the native log as well.")
		: TEXT("Memory-only diagnostic. No saved migration output; inspect in this process and discard by exiting."));
	UE_LOG(LogDCRifleMigration, Display, TEXT("Copy result: success=%d sources_unchanged=%d destinations_verified=%d saved=%d overrides_requested=%d overrides_verified=%d overrides=%d"),
		Result.bSucceeded, Result.bSourceFilesUnchanged, Result.bDestinationsVerified, Result.bSavedToDisk,
		Result.bInheritedOverridesRequested, Result.bInheritedOverridesVerified, Result.InheritedOverrideCount);
	UE_LOG(LogDCRifleMigration, Display, TEXT("Load-dirty policy: requested=%d verified=%d observed_dependencies=%d"),
		Result.bKnownLoadDirtyPolicyRequested, Result.bKnownLoadDirtyPolicyVerified, Result.ObservedLoadDirtyDependencies.Num());
	UE_LOG(LogDCRifleMigration, Display, TEXT("Engine diagnostic gate: clean=%d ensures=%llu"),
		Result.bEngineDiagnosticsClean, static_cast<uint64>(FDebug::GetNumEnsureFailures()));
	return Result;
}

FDCRifleOverrideComparison UDCRifleMigrationLibrary::CompareInheritedOverrides(const FString& RunName, const TMap<FString, FString>& SourceToDestination)
{
	DCRifleMigration::FEngineDiagnosticGuard Diagnostics;
	FDCRifleOverrideComparison Result;
	FString Error;
	FString RunRoot;
	if (!DCRifleMigration::CheckExecutionContext(Error)
		|| !DCRifleMigration::ValidateNames(RunName, SourceToDestination, RunRoot, Error))
	{
		Result.Messages.Add(Error);
		return Result;
	}
	DCRifleMigration::FInheritedOverrideState Overrides;
	if (Overrides.Capture(SourceToDestination, Result.Messages))
	{
		Result.ExpectedOverrides = Overrides.Num();
		Result.bSucceeded = Overrides.Compare(SourceToDestination, Result.Messages, Result.ComparedOverrides);
	}
	Result.bSucceeded &= Diagnostics.Check(TEXT("override_comparison"), Result.Messages);
	return Result;
}
