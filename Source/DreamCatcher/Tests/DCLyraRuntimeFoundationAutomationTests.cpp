// Tests for the isolated native runtime prerequisite layer. No content saves or production activation.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Audio/DCLyraAudioMixEffectsSubsystem.h"
#include "Audio/DCLyraAudioSettings.h"
#include "DCLyraRuntimeDiagnostics.h"
#include "Development/DCLyraPlatformEmulationSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "LoadingProcessInterface.h"
#include "LoadingProcessTask.h"
#include "LoadingScreenManager.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Settings/DCLyraSettingsLocal.h"
#include "Tests/AutomationCommon.h"
#include "Tickable.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

struct FDCLyraLoadingTestAccess
{
	static int32 ProcessorCount(const ULoadingScreenManager& Manager)
	{
		return Manager.ExternalLoadingProcessors.Num();
	}
};

namespace DCLyraRuntimeTests
{
// Engine registration contract probe, not a replacement LoadingScreenManager.
// It can Tick only for the unique inactive test world passed to TickObjects.
class FTickRegistrationProbe final : public FTickableGameObject
{
public:
	explicit FTickRegistrationProbe(UWorld* InWorld)
		: FTickableGameObject(ETickableTickType::Never)
		, ProbeWorld(InWorld)
	{
	}

	virtual void Tick(float DeltaTime) override { ++TickCount; }
	virtual bool IsTickable() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return ProbeWorld; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FDCLyraTickRegistrationProbe, STATGROUP_Tickables);
	}

	int32 GetTickCount() const { return TickCount; }

private:
	UWorld* ProbeWorld;
	int32 TickCount = 0;
};

bool CheckContext(FAutomationTestBase& Test)
{
	if (!GIsEditor || IsRunningCommandlet() || !FApp::IsUnattended()
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCR5RuntimeIsolatedAutomation")))
	{
		Test.AddError(TEXT("Use a separate unattended UnrealEditor-Cmd (not -run=commandlet) with -DCR5RuntimeIsolatedAutomation."));
		return false;
	}
	return true;
}

bool CheckUnconfiguredAudio(FAutomationTestBase& Test)
{
	// Refuse to create a diagnostic world if later Config work would activate real audio assets.
	const UDCLyraAudioSettings* Audio = GetDefault<UDCLyraAudioSettings>();
	return Test.TestTrue(TEXT("Audio must remain unconfigured for this prerequisite fixture"),
		Audio->DefaultControlBusMix.IsNull() && Audio->LoadingScreenControlBusMix.IsNull()
		&& Audio->UserSettingsControlBusMix.IsNull() && Audio->OverallVolumeControlBus.IsNull()
		&& Audio->MusicVolumeControlBus.IsNull() && Audio->SoundFXVolumeControlBus.IsNull()
		&& Audio->DialogueVolumeControlBus.IsNull() && Audio->VoiceChatVolumeControlBus.IsNull()
		&& Audio->HDRAudioSubmixEffectChain.IsEmpty() && Audio->LDRAudioSubmixEffectChain.IsEmpty());
}

bool CreateWorld(FAutomationTestBase& Test, FTestWorldWrapper& Fixture)
{
	if (!CheckContext(Test) || !CheckUnconfiguredAudio(Test))
	{
		return false;
	}
	if (!Fixture.CreateTestWorld(EWorldType::Game))
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	UGameInstance* Instance = Fixture.GetTestWorld()->GetGameInstance();
	return Test.TestNotNull(TEXT("Transient GameInstance"), Instance)
		&& Test.TestNull(TEXT("No viewport: this fixture must never show a loading widget"), Instance->GetGameViewportClient());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraRuntimeGateTest,
	"DreamCatcher.R5.RuntimeFoundation.DiagnosticGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCLyraRuntimeGateTest::RunTest(const FString& Parameters)
{
	for (int32 Mask = 0; Mask < 16; ++Mask)
	{
		const bool bEditor = (Mask & 1) != 0;
		const bool bCommandlet = (Mask & 2) != 0;
		const bool bUnattended = (Mask & 4) != 0;
		const bool bRequested = (Mask & 8) != 0;
		TestEqual(FString::Printf(TEXT("Opt-in context %d"), Mask),
			DCLyraRuntimeDiagnostics::IsAllowedForContext(bEditor, bCommandlet, bUnattended, bRequested), Mask == 13);
	}
	TestEqual(TEXT("Actual process policy matches explicit context"), DCLyraRuntimeDiagnostics::IsEnabled(),
		GIsEditor && !IsRunningCommandlet() && FApp::IsUnattended()
		&& FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraTickRegistrationTest,
	"DreamCatcher.R5.RuntimeFoundation.TickRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCLyraTickRegistrationTest::RunTest(const FString& Parameters)
{
	if (!DCLyraRuntimeTests::CheckContext(*this))
	{
		return false;
	}
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Inactive))
	{
		Fixture.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	if (!TestNotNull(TEXT("Unique inactive world for the tick probe"), World)
		|| !TestNull(TEXT("Tick probe world has no GameInstance or loading/audio activation"), World->GetGameInstance()))
	{
		return false;
	}
	// Probe is destroyed before Fixture; its base destructor also unregisters defensively.
	DCLyraRuntimeTests::FTickRegistrationProbe Probe(World);
	auto TickProbeWorld = [World]()
	{
		// Never pass nullptr: that would dispatch global tickables. No world/actor Tick or BeginPlay here.
		FTickableGameObject::TickObjects(World, LEVELTICK_All, false, 0.0f);
	};

	TickProbeWorld();
	TestEqual(TEXT("Never at construction does not register a tick"), Probe.GetTickCount(), 0);
	Probe.SetTickableTickType(ETickableTickType::Conditional);
	Probe.SetTickableTickType(ETickableTickType::Never);
	TickProbeWorld();
	TestEqual(TEXT("Never removes a pending registration before its first dispatch"), Probe.GetTickCount(), 0);

	Probe.SetTickableTickType(ETickableTickType::Conditional);
	TickProbeWorld();
	TestEqual(TEXT("Registered probe really receives a tick"), Probe.GetTickCount(), 1);
	TickProbeWorld();
	TestEqual(TEXT("Registered probe remains eligible"), Probe.GetTickCount(), 2);

	Probe.SetTickableTickType(ETickableTickType::Never);
	TestTrue(TEXT("Initial policy getter is not the current registration state"), Probe.GetTickableTickType() == ETickableTickType::Conditional);
	TickProbeWorld();
	TickProbeWorld();
	TestEqual(TEXT("Never removes an active registration from subsequent dispatches"), Probe.GetTickCount(), 2);

	Probe.SetTickableTickType(ETickableTickType::Conditional);
	TickProbeWorld();
	TestEqual(TEXT("Re-registration resumes dispatch as a positive control"), Probe.GetTickCount(), 3);
	Probe.SetTickableTickType(ETickableTickType::Never);
	TickProbeWorld();
	TestEqual(TEXT("Final unregister stops dispatch again"), Probe.GetTickCount(), 3);
	AddInfo(TEXT("Engine tick-registration API contract only. Real loading manager lifecycle/delegates are checked separately; actual viewport ticking/map travel remains unverified."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraRuntimeSettingsDefaultsTest,
	"DreamCatcher.R5.RuntimeFoundation.SettingsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCLyraRuntimeSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	if (!DCLyraRuntimeTests::CheckContext(*this) || !TestNotNull(TEXT("Engine"), GEngine))
	{
		return false;
	}
	UGameUserSettings* GlobalSettings = GEngine->GetGameUserSettings();
	if (!TestNotNull(TEXT("Existing global settings"), GlobalSettings))
	{
		return false;
	}
	TestFalse(TEXT("Prerequisite must not replace global GameUserSettings"), GlobalSettings->IsA<UDCLyraSettingsLocal>());
	const float BeforeGamma = GEngine->DisplayGamma;
	// Query the CDO only. Do NOT call the new Get(), LoadSettings(), setters, or any Apply/Save method.
	const UDCLyraSettingsLocal* Settings = GetDefault<UDCLyraSettingsLocal>();
	TestEqual(TEXT("Original default gamma"), Settings->GetDisplayGamma(), 2.2f);
	TestEqual(TEXT("Original menu FPS data"), Settings->GetFrameRateLimit_InMenu(), 144.0f);
	TestEqual(TEXT("Original background FPS data"), Settings->GetFrameRateLimit_WhenBackgrounded(), 30.0f);
	TestEqual(TEXT("Original battery FPS data"), Settings->GetFrameRateLimit_OnBattery(), 60.0f);
	TestEqual(TEXT("Default Overall volume"), Settings->GetOverallVolume(), 1.0f);
	TestEqual(TEXT("Default Music volume"), Settings->GetMusicVolume(), 1.0f);
	TestEqual(TEXT("Default SFX volume"), Settings->GetSoundFXVolume(), 1.0f);
	TestEqual(TEXT("Default Dialogue volume"), Settings->GetDialogueVolume(), 1.0f);
	TestEqual(TEXT("Default VoiceChat volume"), Settings->GetVoiceChatVolume(), 1.0f);
	TestFalse(TEXT("Headphone mode starts off"), Settings->IsHeadphoneModeEnabled());
	TestFalse(TEXT("HDR audio starts off"), Settings->IsHDRAudioModeEnabled());
	TestFalse(TEXT("Automatic replay recording starts off"), Settings->ShouldAutoRecordReplays());
	TestEqual(TEXT("Original replay retention data"), Settings->GetNumberOfReplaysToKeep(), 5);
	TestFalse(TEXT("Safe zone starts unset"), Settings->IsSafeZoneSet());
	TestTrue(TEXT("No default platform emulation"), GetDefault<UDCLyraPlatformEmulationSettings>()->GetPretendPlatformName().IsNone());
	TestTrue(TEXT("Global settings object unchanged"), GEngine->GetGameUserSettings() == GlobalSettings);
	TestEqual(TEXT("Reading defaults does not apply gamma"), GEngine->DisplayGamma, BeforeGamma);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraRuntimeSubsystemLifetimeTest,
	"DreamCatcher.R5.RuntimeFoundation.SubsystemsAndLoadingTask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCLyraRuntimeSubsystemLifetimeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCLyraRuntimeTests::CreateWorld(*this, Fixture))
	{
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	TStrongObjectPtr<UGameInstance> Instance(World->GetGameInstance());
	ULoadingScreenManager* Loading = Instance->GetSubsystem<ULoadingScreenManager>();
	UDCLyraAudioMixEffectsSubsystem* Audio = World->GetSubsystem<UDCLyraAudioMixEffectsSubsystem>();
	const bool bEnabled = DCLyraRuntimeDiagnostics::IsEnabled();
	TestEqual(TEXT("LoadingScreen creation follows process opt-in"), Loading != nullptr, bEnabled);
	TestEqual(TEXT("AudioMix creation follows process opt-in"), Audio != nullptr, bEnabled);
	TStrongObjectPtr<ULoadingScreenManager> HoldLoading(Loading);
	if (bEnabled && Loading)
	{
		TestFalse(TEXT("No loading screen displayed in the headless fixture"), Loading->GetLoadingScreenDisplayStatus());
		TestFalse(TEXT("No viewport means loading manager cannot tick UI"), Loading->IsTickable());
		TestTrue(TEXT("Manager registered the contextual pre-load delegate"), FCoreUObjectDelegates::PreLoadMapWithContext.IsBoundToObject(Loading));
		TestTrue(TEXT("Manager registered the post-load delegate"), FCoreUObjectDelegates::PostLoadMapWithWorld.IsBoundToObject(Loading));
		const int32 Before = FDCLyraLoadingTestAccess::ProcessorCount(*Loading);
		TStrongObjectPtr<ULoadingProcessTask> Task(ULoadingProcessTask::CreateLoadingScreenProcessTask(World, TEXT("DC isolated loading task")));
		if (TestNotNull(TEXT("Task created through original public factory"), Task.Get()))
		{
			TestTrue(TEXT("Task belongs to the isolated loading manager"), Task->GetOuter() == Loading);
			TestEqual(TEXT("Task registered"), FDCLyraLoadingTestAccess::ProcessorCount(*Loading), Before + 1);
			FString Reason;
			TestTrue(TEXT("Original loading interface reports task reason"), ILoadingProcessInterface::ShouldShowLoadingScreen(Task.Get(), Reason));
			TestEqual(TEXT("Reason preserved"), Reason, FString(TEXT("DC isolated loading task")));
			Task->SetShowLoadingScreenReason(TEXT("DC updated reason"));
			ILoadingProcessInterface::ShouldShowLoadingScreen(Task.Get(), Reason);
			TestEqual(TEXT("Reason update preserved"), Reason, FString(TEXT("DC updated reason")));
			Task->Unregister();
			TestEqual(TEXT("Task unregistered"), FDCLyraLoadingTestAccess::ProcessorCount(*Loading), Before);
		}
	}
	else if (!bEnabled)
	{
		TestNull(TEXT("Task factory cannot opt into a disabled manager"), ULoadingProcessTask::CreateLoadingScreenProcessTask(World, TEXT("Must stay disabled")));
	}
	if (!Fixture.DestroyTestWorld(false))
	{
		Fixture.ForwardErrorMessages(this);
	}
	TestNull(TEXT("GameInstance shutdown removes loading subsystem"), Instance->GetSubsystem<ULoadingScreenManager>());
	if (HoldLoading.IsValid())
	{
		// Keep the object alive so delegate removal cannot pass merely because a weak object expired.
		TestFalse(TEXT("Shutdown removes the same contextual pre-load delegate"), FCoreUObjectDelegates::PreLoadMapWithContext.IsBoundToObject(HoldLoading.Get()));
		TestFalse(TEXT("Shutdown removes the post-load delegate"), FCoreUObjectDelegates::PostLoadMapWithWorld.IsBoundToObject(HoldLoading.Get()));
	}
	AddInfo(TEXT("Run once without and once with -DCLyraRuntimeDiagnostics. No audio bindings, viewport, saved map, or real loading screen is used."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraRuntimeGameStateTest,
	"DreamCatcher.R5.RuntimeFoundation.GameStateInitialState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCLyraRuntimeGameStateTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCLyraRuntimeTests::CreateWorld(*this, Fixture))
	{
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	if (!TestTrue(TEXT("Create fixture-only GameMode"), World->SetGameMode(FURL())))
	{
		return false;
	}
	AGameModeBase* Mode = World->GetAuthGameMode();
	if (!TestNotNull(TEXT("Fixture GameMode"), Mode))
	{
		return false;
	}
	// Change this transient instance, never the project GameMode or a production CDO.
	Mode->GameStateClass = ADCLyraGameState::StaticClass();
	if (!Fixture.BeginPlayInTestWorld())
	{
		Fixture.ForwardErrorMessages(this);
		return false;
	}
	ADCLyraGameState* State = World->GetGameState<ADCLyraGameState>();
	if (!TestNotNull(TEXT("Original-derived GameState spawned"), State))
	{
		return false;
	}
	UDCAbilitySystemComponent* ASC = State->GetLyraAbilitySystemComponent();
	UDCLyraExperienceManagerComponent* Experience = State->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	if (!TestNotNull(TEXT("GameState ASC"), ASC) || !TestNotNull(TEXT("Experience component"), Experience))
	{
		return false;
	}
	TestTrue(TEXT("ASC replicated by original constructor"), ASC->GetIsReplicated());
	TestTrue(TEXT("ASC owner is GameState"), ASC->GetOwnerActor() == State);
	TestTrue(TEXT("ASC avatar is GameState"), ASC->GetAvatarActor() == State);
	TestTrue(TEXT("Experience component replicated by default"), Experience->GetIsReplicated());
	TestFalse(TEXT("No Experience selected implicitly"), Experience->IsExperienceLoaded());
	FString Reason;
	TestTrue(TEXT("Unloaded Experience reports a loading requirement"), Experience->ShouldShowLoadingScreen(Reason));
	TestEqual(TEXT("Original loading reason"), Reason, FString(TEXT("Experience still loading")));
	TestNull(TEXT("No replay recorder initially"), State->GetRecorderPlayerState());
	AddInfo(TEXT("Initial state only. Does not select/complete an Experience, apply user settings, activate GameFeatures, import assets, or verify networking."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
