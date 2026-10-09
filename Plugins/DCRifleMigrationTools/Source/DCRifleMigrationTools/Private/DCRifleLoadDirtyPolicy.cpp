#include "DCRifleLoadDirtyPolicy.h"

#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogDCRifleLoadDirty, Log, All);

namespace DCRifleMigration
{
const TArray<FString>& ApprovedRifleCorePackages()
{
	static const TArray<FString> Packages = {
		TEXT("/ShooterCore/Weapons/B_WeaponInstance_Base"),
		TEXT("/ShooterCore/Weapons/Rifle/B_WeaponInstance_Rifle"),
		TEXT("/Game/Weapons/B_Weapon"),
		TEXT("/ShooterCore/Weapons/Rifle/B_Rifle"),
		TEXT("/Game/Weapons/GA_Weapon_Fire"),
		TEXT("/ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto"),
		TEXT("/Game/Weapons/GA_Weapon_ReloadMagazine"),
		TEXT("/ShooterCore/Weapons/Rifle/GA_Weapon_Reload_Rifle"),
		TEXT("/Game/Weapons/GA_Weapon_AutoReload"),
		TEXT("/ShooterCore/Weapons/Rifle/W_Reticle_Rifle"),
		TEXT("/ShooterCore/Weapons/Rifle/W_AmmoCounter_Rifle"),
		TEXT("/ShooterCore/Weapons/Rifle/AbilitySet_ShooterRifle"),
		TEXT("/ShooterCore/Weapons/Rifle/WID_Rifle"),
		TEXT("/ShooterCore/Weapons/Rifle/ID_Rifle"),
		TEXT("/ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire"),
		TEXT("/Game/GameplayCueNotifies/GCN_Weapon_Impact")
	};
	return Packages;
}

const TSet<FName>& KnownRifleLoadDirtyPackages()
{
	static const TSet<FName> Packages = {
		FName(TEXT("/Game/Effects/Particles/Weapons/NS_WeaponFire")),
		FName(TEXT("/Game/Effects/Particles/Weapons/NS_WeaponFire_MuzzleFlash_Rifle")),
		FName(TEXT("/Game/Effects/Particles/Weapons/Emitters/NE_MuzzleFlashCards")),
		FName(TEXT("/Game/Effects/Particles/Weapons/Emitters/NE_MuzzleFlashStarBurst"))
	};
	return Packages;
}

const TArray<FString>& ApprovedRifleClosurePackages()
{
	static const TArray<FString> Packages = []
	{
		TArray<FString> Result = ApprovedRifleCorePackages();
		Result.Add(TEXT("/Game/Audio/Blueprints/WeaponAudioMacros"));
		Result.Add(TEXT("/ShooterCore/System/Audio/WeaponAudioFunctions"));
		return Result;
	}();
	return Packages;
}

const TArray<FString>& ProtectedRifleSiblingPackages()
{
	static const TArray<FString> Packages = {
		TEXT("/ShooterCore/Weapons/Pistol/B_Pistol"),
		TEXT("/ShooterCore/Weapons/Pistol/B_WeaponInstance_Pistol"),
		TEXT("/ShooterCore/Weapons/Pistol/GA_Weapon_Fire_Pistol"),
		TEXT("/Game/Weapons/Pistol/GA_Weapon_Reload_Pistol")
	};
	return Packages;
}

bool IsExactRifleClosure(const TMap<FString, FString>& PackageMap)
{
	if (PackageMap.Num() != ApprovedRifleClosurePackages().Num()) { return false; }
	for (const FString& Source : ApprovedRifleClosurePackages())
	{
		if (!PackageMap.Contains(Source)) { return false; }
	}
	return true;
}

bool ValidateLoadDirtyScope(const FString& ProjectName, bool bPreserveOverrides,
	const TMap<FString, FString>& PackageMap, FString& Error)
{
	Error.Reset();
	if (!ProjectName.Equals(TEXT("LyraStarterGame"), ESearchCase::CaseSensitive)
		|| !bPreserveOverrides || (PackageMap.Num() != 2 && PackageMap.Num() != ApprovedRifleCorePackages().Num()
			&& !IsExactRifleClosure(PackageMap)))
	{
		Error = TEXT("Known load-dirty policy requires original Lyra, preservation, and exact Actor2/core16/closure18.");
		return false;
	}
	for (const TPair<FString, FString>& Pair : PackageMap)
	{
		const bool bAllowed = PackageMap.Num() == 2
			? (Pair.Key.Equals(TEXT("/Game/Weapons/B_Weapon"), ESearchCase::CaseSensitive)
				|| Pair.Key.Equals(TEXT("/ShooterCore/Weapons/Rifle/B_Rifle"), ESearchCase::CaseSensitive))
			: (PackageMap.Num() == 18 ? ApprovedRifleClosurePackages() : ApprovedRifleCorePackages()).ContainsByPredicate([&Pair](const FString& Expected)
				{ return Pair.Key.Equals(Expected, ESearchCase::CaseSensitive); });
		if (!bAllowed)
		{
			Error = TEXT("Package is outside the selected fixed Rifle bundle: ") + Pair.Key;
			return false;
		}
	}
	return true;
}

bool EvaluateLoadDirtyPackages(const TArray<FName>& InitialDirty, const TArray<FName>& CurrentDirty,
	const TSet<FName>& Sources, const TSet<FName>& Destinations, bool bCopiesCreated,
	TArray<FString>& ObservedDependencies, FString& Error)
{
	ObservedDependencies.Reset();
	Error.Reset();
	if (!InitialDirty.IsEmpty())
	{
		Error = TEXT("Dirty package existed before source loading; no exception is permitted: ") + InitialDirty[0].ToString();
		return false;
	}
	for (FName Package : CurrentDirty)
	{
		// Source protection takes precedence over every possible exception.
		if (Sources.Contains(Package))
		{
			Error = TEXT("Explicit source became dirty: ") + Package.ToString();
			return false;
		}
		if (bCopiesCreated && Destinations.Contains(Package))
		{
			continue; // Only the exact, newly-created output packages may be saved by the caller.
		}
		if (!KnownRifleLoadDirtyPackages().Contains(Package))
		{
			Error = TEXT("Unexpected dirty package outside the fixed load-dependency list: ") + Package.ToString();
			return false;
		}
		ObservedDependencies.AddUnique(Package.ToString());
	}
	ObservedDependencies.Sort();
	return true;
}

bool IsTrackedFileUnchanged(bool bExistedBefore, const FMD5Hash& Before,
	bool bExistsNow, const FMD5Hash& Now)
{
	return bExistedBefore == bExistsNow
		&& (!bExistsNow || (Before.IsValid() && Now.IsValid() && Before == Now));
}

namespace
{
TArray<FName> ReadDirtyPackageNames()
{
	TArray<UPackage*> Packages;
	FEditorFileUtils::GetDirtyPackages(Packages);
	TArray<FName> Names;
	for (UPackage* Package : Packages)
	{
		Names.Add(Package->GetFName());
	}
	return Names;
}
}

bool FRifleLoadDirtyPolicy::Begin(const FString& ProjectName, bool bPreserveOverrides,
	const TMap<FString, FString>& PackageMap, TArray<FString>& Messages)
{
	bInitialized = false;
	Sources.Reset();
	Destinations.Reset();
	OriginalFiles.Reset();
	InitialDirty = ReadDirtyPackageNames();
	FString Error;
	if (!ValidateLoadDirtyScope(ProjectName, bPreserveOverrides, PackageMap, Error))
	{
		Messages.Add(Error);
		return false;
	}
	TArray<FString> Observed;
	if (!EvaluateLoadDirtyPackages(InitialDirty, InitialDirty, Sources, Destinations, false, Observed, Error))
	{
		Messages.Add(Error);
		return false;
	}
	for (const TPair<FString, FString>& Pair : PackageMap)
	{
		Sources.Add(FName(*Pair.Key));
		Destinations.Add(FName(*Pair.Value));
	}
	TSet<FName> Watched = KnownRifleLoadDirtyPackages();
	if (IsExactRifleClosure(PackageMap))
	{
		for (const FString& Sibling : ProtectedRifleSiblingPackages()) { Watched.Add(FName(*Sibling)); }
	}
	for (FName Package : Watched)
	{
		FString Filename;
		if (!FPackageName::DoesPackageExist(Package.ToString(), &Filename)
			|| !Filename.EndsWith(FPackageName::GetAssetPackageExtension(), ESearchCase::IgnoreCase))
		{
			Messages.Add(TEXT("Cannot resolve watched dependency to a .uasset file: ") + Package.ToString());
			return false;
		}
		Filename = FPaths::ConvertRelativePathToFull(Filename);
		for (const TCHAR* Extension : {TEXT("uasset"), TEXT("uexp"), TEXT("ubulk"), TEXT("uptnl")})
		{
			const FString File = FPaths::ChangeExtension(Filename, Extension);
			const bool bExists = IFileManager::Get().FileExists(*File);
			const FMD5Hash Hash = bExists ? FMD5Hash::HashFile(*File) : FMD5Hash();
			if ((File == Filename && !bExists) || (bExists && !Hash.IsValid()))
			{
				Messages.Add(TEXT("Missing/unreadable watched dependency file: ") + File);
				return false;
			}
			OriginalFiles.Add(File, Hash);
			const FString Value = Hash.IsValid() ? LexToString(Hash) : TEXT("absent");
			UE_LOG(LogDCRifleLoadDirty, Display, TEXT("Dependency before load: %s md5=%s"), *File, *Value);
		}
	}
	bInitialized = true;
	Messages.Add(TEXT("Four fixed load-dirty dependencies fingerprinted before loading; closure18 also fingerprints four protected Pistol siblings. No dirty flags cleared."));
	return true;
}

bool FRifleLoadDirtyPolicy::Verify(bool bCopiesCreated, const TCHAR* Phase,
	TArray<FString>& ObservedDependencies, TArray<FString>& Messages) const
{
	ObservedDependencies.Reset();
	if (!bInitialized)
	{
		Messages.Add(TEXT("Load-dirty policy was not initialized before loading."));
		return false;
	}
	FString Error;
	bool bValid = EvaluateLoadDirtyPackages(InitialDirty, ReadDirtyPackageNames(), Sources, Destinations,
		bCopiesCreated, ObservedDependencies, Error);
	if (!bValid)
	{
		Messages.Add(FString(Phase) + TEXT(": ") + Error);
	}
	// Check all four packages, not only the ones which happen to be dirty now.
	for (const TPair<FString, FMD5Hash>& Pair : OriginalFiles)
	{
		const bool bExists = IFileManager::Get().FileExists(*Pair.Key);
		const FMD5Hash Hash = bExists ? FMD5Hash::HashFile(*Pair.Key) : FMD5Hash();
		if (!IsTrackedFileUnchanged(Pair.Value.IsValid(), Pair.Value, bExists, Hash))
		{
			bValid = false;
			Messages.Add(FString(Phase) + TEXT(": watched dependency changed/was unreadable: ") + Pair.Key);
		}
		const FString Value = Hash.IsValid() ? LexToString(Hash) : (bExists ? TEXT("unreadable") : TEXT("absent"));
		UE_LOG(LogDCRifleLoadDirty, Display, TEXT("Dependency %s: %s md5=%s"), Phase, *Pair.Key, *Value);
	}
	for (const FString& Package : ObservedDependencies)
	{
		UE_LOG(LogDCRifleLoadDirty, Display, TEXT("Load-dirty observed (%s), retained unsaved: %s"), Phase, *Package);
	}
	return bValid;
}
}
