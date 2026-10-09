// Pure policy/hash-state tests. No packages/assets/worlds are created, dirtied, saved, or deleted.
#include "DCRifleLoadDirtyPolicy.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleLoadDirtyPolicyTest,
	"DreamCatcher.MigrationTools.ExplicitCopy.LoadDirtyPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCRifleLoadDirtyPolicyTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleMigration;
	const FName SourceParent(TEXT("/Game/Weapons/B_Weapon"));
	const FName SourceChild(TEXT("/ShooterCore/Weapons/Rifle/B_Rifle"));
	const FString Root(TEXT("/Game/LyraMigration/Rifle/Diagnostics/Explicit/PolicyTest"));
	const FName DestinationParent(*(Root / TEXT("B_Weapon")));
	const FName DestinationChild(*(Root / TEXT("B_Rifle")));
	TMap<FString, FString> Plan;
	Plan.Add(SourceParent.ToString(), DestinationParent.ToString());
	Plan.Add(SourceChild.ToString(), DestinationChild.ToString());
	FString Error;
	TestTrue(TEXT("Exact original pair with preservation accepted"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, Plan, Error));
	TestFalse(TEXT("Other project refused"), ValidateLoadDirtyScope(TEXT("DreamCatcher"), true, Plan, Error));
	TestFalse(TEXT("Non-preservation mode refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), false, Plan, Error));
	TMap<FString, FString> TooBroad = Plan;
	TooBroad.Add(TEXT("/Game/Extra"), Root / TEXT("Extra"));
	TestFalse(TEXT("Additional source refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, TooBroad, Error));
	TMap<FString, FString> WrongPair = Plan;
	WrongPair.Remove(SourceChild.ToString());
	WrongPair.Add(TEXT("/Game/Unrelated"), Root / TEXT("Unrelated"));
	TestFalse(TEXT("Different two-package pair refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, WrongPair, Error));

	TestEqual(TEXT("Core bundle is exactly 16"), ApprovedRifleCorePackages().Num(), 16);
	TMap<FString, FString> CorePlan;
	for (const FString& Source : ApprovedRifleCorePackages())
	{
		CorePlan.Add(Source, Root / FPackageName::GetShortName(Source));
	}
	TestEqual(TEXT("All core source names are distinct"), CorePlan.Num(), 16);
	TestTrue(TEXT("Exact core bundle accepted"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, CorePlan, Error));
	TestFalse(TEXT("Core bundle cannot disable preservation"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), false, CorePlan, Error));
	TestFalse(TEXT("Core bundle cannot target another project"), ValidateLoadDirtyScope(TEXT("DreamCatcher"), true, CorePlan, Error));
	CorePlan.Remove(SourceChild.ToString());
	TestFalse(TEXT("Partial core bundle refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, CorePlan, Error));
	CorePlan.Add(TEXT("/Game/Unapproved"), Root / TEXT("Unapproved"));
	TestFalse(TEXT("Sixteen entries alone do not grant approval"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, CorePlan, Error));
	CorePlan.Add(SourceChild.ToString(), DestinationChild.ToString());
	TestFalse(TEXT("Core plus additional asset refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, CorePlan, Error));
	TMap<FString, FString> ClosurePlan;
	for (const FString& Source : ApprovedRifleClosurePackages())
	{
		ClosurePlan.Add(Source, Root / FPackageName::GetShortName(Source));
	}
	TestEqual(TEXT("Closure includes exactly two original libraries"), ClosurePlan.Num(), 18);
	TestTrue(TEXT("Exact closure18 accepted"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, ClosurePlan, Error));
	TestFalse(TEXT("Closure requires preservation"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), false, ClosurePlan, Error));
	TestFalse(TEXT("Closure requires original project"), ValidateLoadDirtyScope(TEXT("DreamCatcher"), true, ClosurePlan, Error));
	ClosurePlan.Remove(TEXT("/Game/Audio/Blueprints/WeaponAudioMacros"));
	TestFalse(TEXT("Seventeen is not a complete closure"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, ClosurePlan, Error));
	ClosurePlan.Add(TEXT("/Game/Unapproved"), Root / TEXT("Unapproved"));
	TestFalse(TEXT("Arbitrary eighteen refused"), ValidateLoadDirtyScope(TEXT("LyraStarterGame"), true, ClosurePlan, Error));

	const TSet<FName> Sources = {SourceParent, SourceChild};
	const TSet<FName> Destinations = {DestinationParent, DestinationChild};
	const TArray<FName> None;
	TArray<FName> Known;
	for (FName Package : KnownRifleLoadDirtyPackages()) { Known.Add(Package); }
	TestEqual(TEXT("Allowlist remains exactly four packages"), Known.Num(), 4);
	for (const FString& Sibling : ProtectedRifleSiblingPackages())
	{
		TestFalse(TEXT("Protected sibling is not a dirty exception"), KnownRifleLoadDirtyPackages().Contains(FName(*Sibling)));
	}
	if (Known.Num() != 4) { return false; }
	TArray<FString> Observed;
	TestTrue(TEXT("Initially and currently clean accepted"), EvaluateLoadDirtyPackages(None, None, Sources, Destinations, false, Observed, Error));
	TestTrue(TEXT("Four newly load-dirty dependencies accepted"), EvaluateLoadDirtyPackages(None, Known, Sources, Destinations, false, Observed, Error));
	TestEqual(TEXT("All four are reported, not silently cleared"), Observed.Num(), 4);
	TestFalse(TEXT("Already-dirty known dependency refused"), EvaluateLoadDirtyPackages(Known, Known, Sources, Destinations, false, Observed, Error));
	TestFalse(TEXT("Clearing an initial dirty flag cannot bypass policy"), EvaluateLoadDirtyPackages(Known, None, Sources, Destinations, false, Observed, Error));

	TArray<FName> Current = Known;
	Current.Add(SourceParent);
	TestFalse(TEXT("Explicit source becoming dirty refused"), EvaluateLoadDirtyPackages(None, Current, Sources, Destinations, false, Observed, Error));
	Current = Known;
	Current.Add(FName(TEXT("/Game/Effects/Particles/Weapons/Unexpected")));
	TestFalse(TEXT("Same-folder but unlisted dependency refused"), EvaluateLoadDirtyPackages(None, Current, Sources, Destinations, false, Observed, Error));
	Current = Known;
	Current.Add(DestinationParent);
	Current.Add(DestinationChild);
	TestFalse(TEXT("Destination dirty before copy refused"), EvaluateLoadDirtyPackages(None, Current, Sources, Destinations, false, Observed, Error));
	TestTrue(TEXT("Exact new destination packages accepted after copy"), EvaluateLoadDirtyPackages(None, Current, Sources, Destinations, true, Observed, Error));
	TestEqual(TEXT("Destination packages are not reported as load-dirty dependencies"), Observed.Num(), 4);
	Current.Add(FName(*(Root / TEXT("UnexpectedOutput"))));
	TestFalse(TEXT("Other package under run folder refused"), EvaluateLoadDirtyPackages(None, Current, Sources, Destinations, true, Observed, Error));
	TSet<FName> Protected = Sources;
	Protected.Add(Known[0]);
	TestFalse(TEXT("Explicit-source protection outranks allowlist"), EvaluateLoadDirtyPackages(None, Known, Protected, Destinations, true, Observed, Error));

	const auto HashByte = [](uint8 Value)
	{
		FMD5 MD5;
		MD5.Update(&Value, 1);
		FMD5Hash Hash;
		Hash.Set(MD5);
		return Hash;
	};
	const FMD5Hash Original = HashByte(1);
	const FMD5Hash Different = HashByte(2);
	const FMD5Hash Unavailable;
	TestTrue(TEXT("Same readable file hash accepted"), IsTrackedFileUnchanged(true, Original, true, Original));
	TestFalse(TEXT("Changed bytes refused"), IsTrackedFileUnchanged(true, Original, true, Different));
	TestFalse(TEXT("Deleted file refused"), IsTrackedFileUnchanged(true, Original, false, Unavailable));
	TestFalse(TEXT("New companion file refused"), IsTrackedFileUnchanged(false, Unavailable, true, Original));
	TestFalse(TEXT("Unreadable current file refused"), IsTrackedFileUnchanged(true, Original, true, Unavailable));
	TestFalse(TEXT("Unreadable baseline refused"), IsTrackedFileUnchanged(true, Unavailable, true, Original));
	TestTrue(TEXT("Companion absent both times accepted"), IsTrackedFileUnchanged(false, Unavailable, false, Unavailable));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
