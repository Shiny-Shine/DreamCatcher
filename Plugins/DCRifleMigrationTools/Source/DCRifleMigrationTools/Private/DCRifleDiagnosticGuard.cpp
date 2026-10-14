#include "DCRifleDiagnosticGuard.h"

#include "CoreGlobals.h"
#include "Misc/AssertionMacros.h"
#include "Misc/OutputDeviceRedirector.h"

namespace DCRifleMigration
{
bool AreEngineDiagnosticsClean(uint64 InitialEnsures, uint64 CurrentEnsures, int32 LoggedErrors)
{
	return InitialEnsures == 0 && CurrentEnsures == 0 && LoggedErrors == 0;
}

FEngineDiagnosticGuard::FEngineDiagnosticGuard()
	: InitialEnsures(FDebug::GetNumEnsureFailures())
{
	GLog->AddOutputDevice(this);
}

FEngineDiagnosticGuard::~FEngineDiagnosticGuard()
{
	GLog->RemoveOutputDevice(this);
}

void FEngineDiagnosticGuard::Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category)
{
	const ELogVerbosity::Type Level = static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask);
	if (Level == ELogVerbosity::Error || Level == ELogVerbosity::Fatal) { LoggedErrors.Increment(); }
}

bool FEngineDiagnosticGuard::IsClean() const
{
	return AreEngineDiagnosticsClean(InitialEnsures, FDebug::GetNumEnsureFailures(), LoggedErrors.GetValue());
}

bool FEngineDiagnosticGuard::Check(const TCHAR* Phase, TArray<FString>& Messages) const
{
	const uint64 Current = FDebug::GetNumEnsureFailures();
	const int32 Errors = LoggedErrors.GetValue();
	if (!AreEngineDiagnosticsClean(InitialEnsures, Current, Errors))
	{
		Messages.Add(FString::Printf(TEXT("%s: engine diagnostics failed (initial ensures=%llu, current=%llu, errors=%d). No further mutation/save is permitted."),
			Phase, InitialEnsures, Current, Errors));
		return false;
	}
	return true;
}
}
