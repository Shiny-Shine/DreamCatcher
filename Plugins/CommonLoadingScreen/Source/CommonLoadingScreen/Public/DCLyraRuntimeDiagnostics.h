// DreamCatcher migration-only isolation. Not part of the upstream Lyra plugin.
#pragma once

#include "CoreGlobals.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace DCLyraRuntimeDiagnostics
{
inline bool IsAllowedForContext(bool bEditor, bool bCommandlet, bool bUnattended, bool bRequested)
{
	return bEditor && !bCommandlet && bUnattended && bRequested;
}

// Remove this temporary gate at the separately approved active-path cutover.
// Merely loading the new native types must not start global UI/audio processing.
inline bool IsEnabled()
{
#if WITH_EDITOR
	return IsAllowedForContext(GIsEditor, IsRunningCommandlet(), FApp::IsUnattended(),
		FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")));
#else
	return false;
#endif
}
}
