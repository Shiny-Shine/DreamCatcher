// Pure path/count guards only. These tests never load, copy, save, or delete assets.
#include "DCRifleMigrationValidation.h"
#include "DCRifleLoadDirtyPolicy.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleCopyNameGuardsTest,
	"DreamCatcher.MigrationTools.ExplicitCopy.NameGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCRifleCopyNameGuardsTest::RunTest(const FString& Parameters)
{
	const FString RunName = TEXT("GuardTest_01");
	const FString RunRoot = FString(DCRifleMigration::DiagnosticsRoot) / RunName;
	FString ActualRoot;
	FString Error;
	const auto Accept = [this, &RunName, &ActualRoot, &Error](const TCHAR* Label, const TMap<FString, FString>& Candidate)
	{
		const bool bValid = DCRifleMigration::ValidateNames(RunName, Candidate, ActualRoot, Error);
		TestTrue(Label, bValid);
		if (!bValid) { AddInfo(FString(Label) + TEXT(" rejected: ") + Error); }
	};
	TMap<FString, FString> Plan;
	Plan.Add(TEXT("/Game/Audio/Blueprints/Example"), RunRoot / TEXT("Audio/Example"));
	Accept(TEXT("One explicit /Game package is accepted"), Plan);
	TestEqual(TEXT("Destination root is fixed"), ActualRoot, RunRoot);
	TestTrue(TEXT("Successful validation has no error"), Error.IsEmpty());
	TMap<FString, FString> ShooterPlan;
	ShooterPlan.Add(TEXT("/ShooterCore/Weapons/Example"), RunRoot / TEXT("Example"));
	Accept(TEXT("ShooterCore name accepted independently of plugin mounting or asset existence"), ShooterPlan);

	const auto Reject = [this, &ActualRoot, &Error](const TCHAR* Label, const FString& Name,
		const TMap<FString, FString>& Candidate)
	{
		TestFalse(Label, DCRifleMigration::ValidateNames(Name, Candidate, ActualRoot, Error));
		TestFalse(FString(Label) + TEXT(" explains rejection"), Error.IsEmpty());
	};
	Reject(TEXT("Empty run name rejected"), TEXT(""), Plan);
	Reject(TEXT("Traversal run name rejected"), TEXT("../Escape"), Plan);
	Reject(TEXT("Whitespace run name rejected"), TEXT("Bad Name"), Plan);
	Reject(TEXT("Overlong run name rejected"), FString::ChrN(49, TEXT('A')), Plan);
	Reject(TEXT("Empty map rejected"), RunName, TMap<FString, FString>());

	const auto RejectPair = [&Reject, &RunName](const TCHAR* Label, const FString& Source, const FString& Destination)
	{
		TMap<FString, FString> Candidate;
		Candidate.Add(Source, Destination);
		Reject(Label, RunName, Candidate);
	};
	RejectPair(TEXT("Engine source rejected"), TEXT("/Engine/Example"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("Unapproved plugin root rejected"), TEXT("/UnapprovedPlugin/Example"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("ShooterCore root-prefix trick rejected"), TEXT("/ShooterCoreOther/Example"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("Source traversal rejected"), TEXT("/ShooterCore/../Example"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("Object path rejected"), TEXT("/Game/Example.Example"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("Filesystem source rejected"), TEXT("E:/Example.uasset"), RunRoot / TEXT("Example"));
	RejectPair(TEXT("Destination outside diagnostics rejected"), TEXT("/Game/Example"), TEXT("/Game/Existing/Example"));
	RejectPair(TEXT("Sibling run-prefix trick rejected"), TEXT("/Game/Example"), RunRoot + TEXT("_Other/Example"));
	RejectPair(TEXT("Traversal destination rejected"), TEXT("/Game/Example"), RunRoot / TEXT("../Example"));
	RejectPair(TEXT("Repeated separator rejected"), TEXT("/Game/Example"), RunRoot + TEXT("//Example"));
	RejectPair(TEXT("Reserved filesystem component rejected"), TEXT("/Game/Example"), RunRoot / TEXT("NUL/Example"));
	RejectPair(TEXT("Self source rejected"), RunRoot / TEXT("Example"), RunRoot / TEXT("Copy"));

	TMap<FString, FString> DuplicateDestination;
	DuplicateDestination.Add(TEXT("/Game/First"), RunRoot / TEXT("Same"));
	DuplicateDestination.Add(TEXT("/Game/Second"), RunRoot / TEXT("same"));
	Reject(TEXT("Case-alias destination rejected"), RunName, DuplicateDestination);

	TMap<FString, FString> AtLimit;
	for (int32 Index = 0; Index < DCRifleMigration::MaxPackagesPerRun; ++Index)
	{
		AtLimit.Add(FString::Printf(TEXT("/Game/Source_%d"), Index),
			RunRoot / FString::Printf(TEXT("Copy_%d"), Index));
	}
	Accept(TEXT("Exactly 16 packages accepted"), AtLimit);
	AtLimit.Add(TEXT("/Game/TooMany"), RunRoot / TEXT("TooMany"));
	Reject(TEXT("Seventeenth package rejected"), RunName, AtLimit);
	AtLimit.Add(TEXT("/Game/Another"), RunRoot / TEXT("Another"));
	Reject(TEXT("Arbitrary eighteen rejected"), RunName, AtLimit);
	TMap<FString, FString> Closure;
	for (const FString& Source : DCRifleMigration::ApprovedRifleClosurePackages())
	{
		Closure.Add(Source, RunRoot / FPackageName::GetShortName(Source));
	}
	Accept(TEXT("Exact approved eighteen accepted"), Closure);
	Closure.Add(TEXT("/Game/Extra"), RunRoot / TEXT("Extra"));
	Reject(TEXT("Closure plus arbitrary extra rejected"), RunName, Closure);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
