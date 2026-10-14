#include "DCRifleDiagnosticGuard.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleDiagnosticGateTest,
	"DreamCatcher.MigrationTools.ExplicitCopy.EngineDiagnosticGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCRifleDiagnosticGateTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleMigration;
	TestTrue(TEXT("Fresh clean process allowed"), AreEngineDiagnosticsClean(0, 0, 0));
	TestFalse(TEXT("New ensure blocks progress/save"), AreEngineDiagnosticsClean(0, 1, 0));
	TestFalse(TEXT("Prior ensure cannot be normalized into success"), AreEngineDiagnosticsClean(1, 1, 0));
	TestFalse(TEXT("Error log blocks progress/save"), AreEngineDiagnosticsClean(0, 0, 1));
	FEngineDiagnosticGuard Guard;
	TArray<FString> Messages;
	TestTrue(TEXT("Fresh guard starts clean"), Guard.Check(TEXT("test_start"), Messages));
	// Feed the observer directly, not GLog: exercise latching without a real engine
	// error/ensure or poisoning other tests' process-wide counters.
	Guard.Serialize(TEXT("known warning"), ELogVerbosity::Warning, NAME_None);
	TestTrue(TEXT("Warnings do not masquerade as errors"), Guard.Check(TEXT("test_warning"), Messages));
	Guard.Serialize(TEXT("synthetic error"), ELogVerbosity::Error, NAME_None);
	TestFalse(TEXT("Error is latched"), Guard.Check(TEXT("test_error"), Messages));
	TestFalse(TEXT("Later clean stage cannot clear error"), Guard.Check(TEXT("test_save"), Messages));
	TestFalse(TEXT("Rejection includes diagnostic information"), Messages.IsEmpty());
	return !HasAnyErrors();
}
#endif
