#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/OutputDevice.h"

namespace DCRifleMigration
{
// Pure predicate: a fresh diagnostic process must have no prior/current ensures or errors.
bool AreEngineDiagnosticsClean(uint64 InitialEnsures, uint64 CurrentEnsures, int32 LoggedErrors);

// Observe diagnostics, never suppress them or reset engine counters.
class FEngineDiagnosticGuard final : public FOutputDevice
{
public:
	FEngineDiagnosticGuard();
	virtual ~FEngineDiagnosticGuard() override;
	FEngineDiagnosticGuard(const FEngineDiagnosticGuard&) = delete;
	FEngineDiagnosticGuard& operator=(const FEngineDiagnosticGuard&) = delete;
	bool IsClean() const;
	bool Check(const TCHAR* Phase, TArray<FString>& Messages) const;
	virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override;
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
private:
	uint64 InitialEnsures = 0;
	FThreadSafeCounter LoggedErrors;
};
}
