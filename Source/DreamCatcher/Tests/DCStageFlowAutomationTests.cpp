#include "Tests/DCStageFlowTestTypes.h"
#include "AI/DCEnemySpawner.h"
#include "Player/DCPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCStageFlowTestTypes)

ADCR11DirectorFixture::ADCR11DirectorFixture() { bAutoStartStage = false; }
void ADCR11DirectorFixture::Configure(const TArray<FDCStagePhaseDefinition>& InPhases) { Phases = InPhases; }
void ADCR11EncounterFixture::Configure(float Delay, ADCEnemySpawner* Spawner)
{
	StartDelay = Delay;
	Spawners.Reset();
	if (Spawner) { Spawners.Add(Spawner); }
}

ADCR11ModeFixture::ADCR11ModeFixture() { PlayerStateClass = ADCPlayerState::StaticClass(); }
void ADCR11ModeFixture::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	AGameModeBase::InitGame(MapName, Options, ErrorMessage);
}
void ADCR11ModeFixture::InitGameState() { AGameModeBase::InitGameState(); }
void ADCR11ModeFixture::StartPlay() { AGameModeBase::StartPlay(); }
void ADCR11ModeFixture::PerformLevelTravel(FName LevelName) { ++TravelCalls; LastTravel = LevelName; }
void UDCR11Observer::PhaseStarted(ADCStageDirector*, int32 Index, const FText&, EDCStagePhaseType) { Indices.Add(Index); }
void UDCR11Observer::StageEnded(ADCStageDirector*, bool bSucceeded) { ++EndCalls; bWon = bSucceeded; }

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "Tests/DCLyraFiniteSpawnerTestTypes.h"
#include "UObject/StrongObjectPtr.h"

struct FDCStageFlowTestAccess
{
	static void Completion(ADCStageDirector* Stage, ADCEncounterController* Encounter) { Stage->HandleEncounterCompleted(Encounter); }
	static void TeamsReady(ADCStageRunGameMode* Mode) { Mode->bExperienceReady = true; }
	static void EndRun(ADCStageRunGameMode* Mode, bool bSucceeded) { Mode->EndRun(bSucceeded); }
	static void ObserveDirector(ADCStageRunGameMode* Mode, ADCStageDirector* Stage)
	{
		Mode->Director = Stage;
		Stage->OnStageEnded.AddUniqueDynamic(Mode, &ADCStageRunGameMode::HandleStageEnded);
		Stage->OnDestroyed.AddUniqueDynamic(Mode, &ADCStageRunGameMode::HandleDirectorDestroyed);
	}
};

namespace DCStageFlowTests
{
FDCStagePhaseDefinition Phase(EDCStagePhaseType Type, ADCEncounterController* Encounter = nullptr, float Delay = 0.f)
{
	FDCStagePhaseDefinition Value;
	Value.PhaseType = Type;
	Value.Encounter = Encounter;
	Value.PhaseDelay = Delay;
	return Value;
}

struct FFixture
{
	FTestWorldWrapper World;
	TStrongObjectPtr<UDCR11Observer> Observer { NewObject<UDCR11Observer>() };
	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR11StageIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate unattended editor with -DCR11StageIsolatedAutomation; no assets or maps are saved."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR11ModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return true;
	}
	template<class T> T* Spawn() { return World.GetTestWorld()->SpawnActor<T>(); }
	ADCR11DirectorFixture* Director(const TArray<FDCStagePhaseDefinition>& Phases)
	{
		auto* Stage = Spawn<ADCR11DirectorFixture>();
		if (Stage)
		{
			Stage->Configure(Phases);
			Stage->OnStagePhaseStarted.AddDynamic(Observer.Get(), &UDCR11Observer::PhaseStarted);
			Stage->OnStageEnded.AddDynamic(Observer.Get(), &UDCR11Observer::StageEnded);
		}
		return Stage;
	}
	ADCR11EncounterFixture* Encounter(float Delay = .25f, ADCEnemySpawner* Spawner = nullptr)
	{
		auto* E = Spawn<ADCR11EncounterFixture>();
		if (E) { E->Configure(Delay, Spawner); }
		return E;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageSequenceTest, "DreamCatcher.R11.Stage.SequenceAndStaleCallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageSequenceTest::RunTest(const FString&)
{
	using namespace DCStageFlowTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* First = F.Encounter(); auto* Second = F.Encounter(); auto* Boss = F.Encounter();
	if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Second"), Second) || !TestNotNull(TEXT("Boss"), Boss)) { return false; }
	auto* S = F.Director({Phase(EDCStagePhaseType::Delay), Phase(EDCStagePhaseType::Encounter, First),
		Phase(EDCStagePhaseType::Encounter, Second), Phase(EDCStagePhaseType::BossEncounter, Boss), Phase(EDCStagePhaseType::Complete)});
	if (!TestNotNull(TEXT("Director"), S)) { return false; }
	S->StartStage(); S->StartStage();
	TestEqual(TEXT("Zero delay begins once"), S->GetCurrentPhaseIndex(), 0);
	F.World.TickTestWorld(.01f);
	TestEqual(TEXT("Zero delay advances next tick"), S->GetCurrentPhaseIndex(), 1);
	FDCEncounterCompletedSignature Replay = First->OnEncounterCompleted;
	Replay.Broadcast(Second); Replay.Broadcast(First); Replay.Broadcast(nullptr);
	TestEqual(TEXT("Foreign and forged completion ignored"), S->GetCurrentPhaseIndex(), 1);
	for (int32 ExpectedIndex = 2; ExpectedIndex <= 4; ++ExpectedIndex)
	{
		F.World.TickTestWorld(.01f); // Activate a timer set by a callback in the preceding tick.
		F.World.TickTestWorld(.30f);
		F.World.TickTestWorld(.01f);
		TestEqual(TEXT("Next sequential phase"), S->GetCurrentPhaseIndex(), ExpectedIndex);
		Replay.Broadcast(First);
		FDCStageFlowTestAccess::Completion(S, First);
		TestEqual(TEXT("Old phase cannot skip the current one"), S->GetCurrentPhaseIndex(), ExpectedIndex);
	}
	TestTrue(TEXT("Run completed"), S->GetRunState() == EDCStageRunState::Completed);
	TestEqual(TEXT("One result"), F.Observer->EndCalls, 1);
	TestTrue(TEXT("Successful result"), F.Observer->bWon);
	TestEqual(TEXT("Five phase notifications"), F.Observer->Indices.Num(), 5);
	S->StartStage(); S->FailStage(); Replay.Broadcast(First);
	F.World.TickTestWorld(.30f);
	TestEqual(TEXT("Terminal result is immutable"), F.Observer->EndCalls, 1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageAbortTest, "DreamCatcher.R11.Stage.AbortStopsOwnedEncounters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageAbortTest::RunTest(const FString&)
{
	using namespace DCStageFlowTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* A = F.Spawn<ADCR10FiniteSpawnerFixture>(); auto* B = F.Spawn<ADCR10FiniteSpawnerFixture>(); auto* Other = F.Spawn<ADCR10FiniteSpawnerFixture>();
	if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("Other"), Other)) { return false; }
	A->ConfigureLyra(); B->ConfigureLyra(); Other->ConfigureLyra();
	auto* First = F.Encounter(1.f, A); auto* Future = F.Encounter(1.f, B); auto* Unused = F.Encounter(1.f, Other);
	if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Future"), Future) || !TestNotNull(TEXT("Unused"), Unused)) { return false; }
	auto* S = F.Director({Phase(EDCStagePhaseType::Encounter, First), Phase(EDCStagePhaseType::BossEncounter, Future),
		Phase(EDCStagePhaseType::Delay, Unused)});
	if (!TestNotNull(TEXT("Director"), S)) { return false; }
	S->StartStage(); S->FailStage(); S->FailStage();
	TestTrue(TEXT("Stage failed"), S->GetRunState() == EDCStageRunState::Failed);
	TestTrue(TEXT("Active and future encounters aborted"), First->IsEncounterAborted() && Future->IsEncounterAborted());
	TestTrue(TEXT("Only owned spawners destroyed"), A->IsActorBeingDestroyed() && B->IsActorBeingDestroyed());
	TestFalse(TEXT("Unrelated spawner preserved"), Other->IsActorBeingDestroyed());
	TestFalse(TEXT("Unused reference in a Delay phase is not owned"), Unused->IsEncounterAborted());
	First->StartEncounter(); Future->StartEncounter();
	F.World.TickTestWorld(1.1f); F.World.TickTestWorld(.02f);
	TestEqual(TEXT("One failure"), F.Observer->EndCalls, 1);
	TestFalse(TEXT("Abort is not success"), F.Observer->bWon);
	TestFalse(TEXT("Aborted encounter did not clear"), First->IsEncounterCompleted());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageInvalidConfigTest, "DreamCatcher.R11.Stage.InvalidConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageInvalidConfigTest::RunTest(const FString&)
{
	using namespace DCStageFlowTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* E = F.Encounter(); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	TArray<TArray<FDCStagePhaseDefinition>> Cases {
		{Phase(EDCStagePhaseType::Encounter)},
		{Phase(EDCStagePhaseType::Delay, nullptr, -1.f)},
		{Phase(EDCStagePhaseType::Encounter, E), Phase(EDCStagePhaseType::BossEncounter, E)}
	};
	AddExpectedError(TEXT("Invalid stage phase configuration"), EAutomationExpectedErrorFlags::Contains, Cases.Num());
	for (const auto& Data : Cases)
	{
		auto* S = F.Director(Data); if (!TestNotNull(TEXT("Invalid director"), S)) { return false; }
		S->StartStage();
		TestTrue(TEXT("Invalid data fails instead of skipping to victory"), S->GetRunState() == EDCStageRunState::Failed);
	}
	TestEqual(TEXT("Each invalid run ends once"), F.Observer->EndCalls, Cases.Num());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageDirectorRemovalTest, "DreamCatcher.R11.Stage.DirectorRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageDirectorRemovalTest::RunTest(const FString&)
{
	using namespace DCStageFlowTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* E = F.Encounter(1.f); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	auto* S = F.Director({Phase(EDCStagePhaseType::Encounter, E)});
	if (!TestNotNull(TEXT("Director"), S)) { return false; }
	auto* Mode = F.World.GetTestWorld()->GetAuthGameMode<ADCR11ModeFixture>();
	if (!TestNotNull(TEXT("Mode"), Mode)) { return false; }
	FDCStageFlowTestAccess::ObserveDirector(Mode, S);
	S->StartStage(); S->Destroy();
	TestTrue(TEXT("Removing a running director aborts its encounter"), E->IsEncounterAborted());
	TestTrue(TEXT("GameMode observes failure"), Mode->HasRunEnded() && !Mode->WasRunSuccessful());
	F.World.TickTestWorld(1.1f);
	TestEqual(TEXT("No automatic restart or travel"), Mode->TravelCalls, 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStageTeamTravelTest, "DreamCatcher.R11.Stage.FixedTeamsAndExplicitTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStageTeamTravelTest::RunTest(const FString&)
{
	using namespace DCStageFlowTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* Mode = F.World.GetTestWorld()->GetAuthGameMode<ADCR11ModeFixture>();
	auto* PC = F.Spawn<ADCR10TeamPlayerControllerFixture>();
	auto* BotA = F.Spawn<ADCLyraPlayerBotController>(); auto* BotB = F.Spawn<ADCLyraPlayerBotController>();
	if (!TestNotNull(TEXT("Mode"), Mode) || !TestNotNull(TEXT("Player"), PC)
		|| !TestNotNull(TEXT("Bot A"), BotA) || !TestNotNull(TEXT("Bot B"), BotB)) { return false; }
	FDCStageFlowTestAccess::TeamsReady(Mode);
	Mode->GenericPlayerInitialization(PC); Mode->GenericPlayerInitialization(BotA); Mode->GenericPlayerInitialization(BotB);
	if (!TestNotNull(TEXT("Player state"), PC->GetPlayerState<ADCPlayerState>())
		|| !TestNotNull(TEXT("Bot A state"), BotA->GetPlayerState<ADCPlayerState>())
		|| !TestNotNull(TEXT("Bot B state"), BotB->GetPlayerState<ADCPlayerState>())) { return false; }
	TestEqual(TEXT("Player team"), PC->GetPlayerState<ADCPlayerState>()->GetTeamId(), 1);
	TestEqual(TEXT("First enemy team"), BotA->GetPlayerState<ADCPlayerState>()->GetTeamId(), 2);
	TestEqual(TEXT("Second enemy team is not balanced onto player side"), BotB->GetPlayerState<ADCPlayerState>()->GetTeamId(), 2);
	TestFalse(TEXT("Cannot retry an unfinished run"), Mode->RequestRestartStage());
	FDCStageFlowTestAccess::EndRun(Mode, true);
	F.World.TickTestWorld(.02f);
	TestEqual(TEXT("Ending alone does not respawn/travel"), Mode->TravelCalls, 0);
	TestTrue(TEXT("Explicit retry accepted"), Mode->RequestRestartStage());
	TestFalse(TEXT("Double-click rejected"), Mode->RequestRestartStage());
	F.World.TickTestWorld(.02f); F.World.TickTestWorld(.02f);
	TestEqual(TEXT("Exactly one deferred travel request"), Mode->TravelCalls, 1);
	TestFalse(TEXT("Current map name retained"), Mode->LastTravel.IsNone());
	AddInfo(TEXT("Native state/team/request contracts only. Actual map reload, UI buttons and GAS/Equipment/HUD reset require PIE."));
	return !HasAnyErrors();
}

#endif
