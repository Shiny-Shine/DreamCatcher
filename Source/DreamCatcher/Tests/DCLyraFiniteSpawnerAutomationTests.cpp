#include "Tests/DCLyraFiniteSpawnerTestTypes.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Engine/World.h"
#include "Player/DCPlayerState.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraFiniteSpawnerTestTypes)

void ADCR10FiniteSpawnerFixture::ConfigureLyra()
{
	LyraBotControllerClass = ADCLyraPlayerBotController::StaticClass();
	SpawnCount = 1;
	MaxAliveAtOnce = 1;
	SpawnInterval = 0.01f;
}

ADCR10FinitePawnFixture::ADCR10FinitePawnFixture()
{
	AutoPossessAI = EAutoPossessAI::Disabled;
	Health = CreateDefaultSubobject<UDCLyraHealthComponent>(TEXT("Health"));
	Health->OnDeathFinished.AddDynamic(this, &ThisClass::DeathFinished);
}

void ADCR10FinitePawnFixture::DeathFinished(AActor*)
{
	// Exercise deferred cleanup ordering without pretending this fixture contains Hero BP graphs.
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyAfterDeath);
}

void ADCR10FinitePawnFixture::DestroyAfterDeath()
{
	DetachFromControllerPendingDestroy();
	Destroy();
}

void ADCR10FinitePawnFixture::EndPlay(const EEndPlayReason::Type Reason)
{
	Health->UninitializeFromAbilitySystem();
	Super::EndPlay(Reason);
}

APawn* ADCR10FiniteFailureGameModeFixture::SpawnDefaultPawnAtTransform_Implementation(AController*, const FTransform&)
{
	return nullptr; // Force the real Lyra GameMode failure/retry path, without loading assets.
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCEnemyGASTestTypes.h"
#include "UObject/StrongObjectPtr.h"

struct FDCLyraFiniteSpawnerTestAccess
{
	static void Adopt(ADCEnemySpawner* Spawner, ADCR10FinitePawnFixture* Pawn, ADCLyraPlayerBotController* Bot)
	{
		Spawner->bIsActive = true;
		Spawner->OwnedLyraControllers.Add(Bot);
		Bot->OnDestroyed.AddUniqueDynamic(Spawner, &ADCEnemySpawner::HandleLyraControllerDestroyed);
		Spawner->TrackLyraEnemy(Pawn, Bot);
	}
	static void DeliverExperienceCallback(ADCEnemySpawner* Spawner) { Spawner->OnLyraExperienceLoaded(nullptr); }
};

namespace DCLyraFiniteSpawnerTests
{
bool Gone(const AActor* Actor) { return !IsValid(Actor) || Actor->IsActorBeingDestroyed(); }

struct FFixture
{
	FTestWorldWrapper World;
	TStrongObjectPtr<UDCR10Observer> Observer { NewObject<UDCR10Observer>() };
	bool Start(FAutomationTestBase& Test)
	{
		if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR10FiniteLyraIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate unattended editor with -DCR10FiniteLyraIsolatedAutomation; saves no assets."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = ADCR10FiniteFailureGameModeFixture::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		return Test.TestNotNull(TEXT("Lyra GameState in fixture"), World.GetTestWorld()->GetGameState<ADCLyraGameState>());
	}
	template<class T> T* Spawn() { return World.GetTestWorld()->SpawnActor<T>(); }
	ADCR10FiniteSpawnerFixture* Spawner()
	{
		auto* S = Spawn<ADCR10FiniteSpawnerFixture>();
		if (S)
		{
			S->ConfigureLyra();
			S->OnSpawnerCleared.AddDynamic(Observer.Get(), &UDCR10Observer::SpawnerCleared);
		}
		return S;
	}
	ADCR10FinitePawnFixture* Adopt(FAutomationTestBase& Test, ADCEnemySpawner* S, ADCLyraPlayerBotController*& Bot)
	{
		Bot = Spawn<ADCLyraPlayerBotController>();
		auto* Pawn = Spawn<ADCR10FinitePawnFixture>();
		if (!Test.TestNotNull(TEXT("Fixture controller"), Bot) || !Test.TestNotNull(TEXT("Fixture pawn"), Pawn)) { return nullptr; }
		auto* PS = Bot->GetPlayerState<ADCPlayerState>();
		if (!Test.TestNotNull(TEXT("Controller owns PlayerState/ASC"), PS)) { return nullptr; }
		Bot->Possess(Pawn);
		Pawn->Health->InitializeWithAbilitySystem(CastChecked<UDCAbilitySystemComponent>(PS->GetAbilitySystemComponent()));
		FDCLyraFiniteSpawnerTestAccess::Adopt(S, Pawn, Bot);
		return Pawn;
	}
	ADCR10EncounterFixture* Encounter(ADCEnemySpawner* First, ADCEnemySpawner* Second = nullptr)
	{
		auto* E = Spawn<ADCR10EncounterFixture>();
		if (E)
		{
			E->Configure(First, Second);
			E->OnEncounterCompleted.AddDynamic(Observer.Get(), &UDCR10Observer::EncounterCompleted);
		}
		return E;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteDeathCleanupTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.DeathAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteDeathCleanupTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* S = F.Spawner(); if (!TestNotNull(TEXT("Spawner"), S)) { return false; }
	ADCLyraPlayerBotController* Bot = nullptr;
	auto* Pawn = F.Adopt(*this, S, Bot); if (!Pawn) { return false; }
	APlayerState* PS = Bot->PlayerState;
	auto* E = F.Encounter(S); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	E->StartEncounter();
	FDCLyraHealth_DeathEvent Replay = Pawn->Health->OnDeathStarted;
	TestEqual(TEXT("Living pawn is not pending death cleanup"), S->GetPendingLyraCleanupCount(), 0);
	Replay.Broadcast(Pawn); // Forged live notification is not a kill.
	TestEqual(TEXT("Live notification ignored"), S->GetAliveCount(), 1);
	Pawn->Health->StartDeath();
	Replay.Broadcast(Pawn);
	Replay.Broadcast(nullptr);
	TestEqual(TEXT("Kill counted once"), S->GetAliveCount(), 0);
	TestEqual(TEXT("One finite spawn"), S->GetSpawnedCount(), 1);
	TestEqual(TEXT("Death retains cleanup ownership"), S->GetPendingLyraCleanupCount(), 1);
	TestEqual(TEXT("One clear at death start"), F.Observer->Clears, 1);
	TestEqual(TEXT("One encounter completion"), F.Observer->Encounters, 1);
	TestFalse(TEXT("Controller survives death animation"), Gone(Bot));
	TestFalse(TEXT("PlayerState/ASC survives death animation"), Gone(PS));
	Pawn->Health->FinishDeath();
	TestFalse(TEXT("DeathFinished does not destroy ASC in the callback"), Gone(PS));
	F.World.TickTestWorld(.02f);
	TestTrue(TEXT("Pawn removed"), Gone(Pawn));
	TestTrue(TEXT("Controller removed after pawn"), Gone(Bot));
	TestTrue(TEXT("PlayerState removed by original Controller cleanup"), Gone(PS));
	TestEqual(TEXT("Cleanup ownership released"), S->GetPendingLyraCleanupCount(), 0);
	S->ActivateSpawner(); F.World.TickTestWorld(.10f);
	TestEqual(TEXT("No respawn after finite clear"), S->GetSpawnedCount(), 1);
	TestEqual(TEXT("Cleanup cannot complete twice"), F.Observer->Encounters, 1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteEncounterOrderingTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.EncounterRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteEncounterOrderingTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* Empty = F.Spawn<ADCEnemySpawner>(); // Legacy empty configuration clears synchronously.
	auto* S = F.Spawner();
	if (!TestNotNull(TEXT("Empty spawner"), Empty) || !TestNotNull(TEXT("Pending spawner"), S)) { return false; }
	ADCLyraPlayerBotController* Bot = nullptr;
	auto* Pawn = F.Adopt(*this, S, Bot); if (!Pawn) { return false; }
	auto* E = F.Encounter(Empty, S); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	E->StartEncounter();
	TestTrue(TEXT("First spawner cleared inline"), Empty->IsSpawnerCleared());
	TestFalse(TEXT("Later spawner blocks premature completion"), E->IsEncounterCompleted());
	TestEqual(TEXT("One pending spawner"), E->GetRemainingSpawnerCount(), 1);
	Empty->OnSpawnerCleared.Broadcast(Empty);
	Empty->OnSpawnerCleared.Broadcast(nullptr);
	S->OnSpawnerCleared.Broadcast(Empty);
	S->OnSpawnerCleared.Broadcast(S);
	TestEqual(TEXT("Duplicate/foreign/forged callbacks ignored"), E->GetRemainingSpawnerCount(), 1);
	Pawn->Health->StartDeath(); Pawn->Health->FinishDeath();
	TestEqual(TEXT("One completion"), F.Observer->Encounters, 1);
	F.World.TickTestWorld(.02f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteDuplicateSpawnerTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.DuplicateAndAlreadyCleared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteDuplicateSpawnerTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* S = F.Spawner(); if (!TestNotNull(TEXT("Spawner"), S)) { return false; }
	ADCLyraPlayerBotController* Bot = nullptr;
	auto* Pawn = F.Adopt(*this, S, Bot); if (!Pawn) { return false; }
	auto* E = F.Encounter(S, S); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	E->StartEncounter(); E->StartEncounter();
	TestEqual(TEXT("Duplicate configured pointer counted once"), E->GetRemainingSpawnerCount(), 1);
	Pawn->Health->StartDeath(); Pawn->Health->FinishDeath();
	TestTrue(TEXT("Duplicate pointer cannot hang encounter"), E->IsEncounterCompleted());
	auto* Later = F.Encounter(S, S); if (!TestNotNull(TEXT("Later encounter"), Later)) { return false; }
	Later->StartEncounter();
	TestTrue(TEXT("Already-cleared spawner needs no second broadcast"), Later->IsEncounterCompleted());
	TestEqual(TEXT("Each encounter completes once"), F.Observer->Encounters, 2);
	F.World.TickTestWorld(.02f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteUnexpectedRemovalTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.UnexpectedRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteUnexpectedRemovalTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* S = F.Spawner(); if (!TestNotNull(TEXT("Spawner"), S)) { return false; }
	ADCLyraPlayerBotController* Bot = nullptr;
	auto* Pawn = F.Adopt(*this, S, Bot); if (!Pawn) { return false; }
	APlayerState* PS = Bot->PlayerState;
	auto* E = F.Encounter(S); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	E->StartEncounter();
	AddExpectedError(TEXT("Lyra pawn removed before its death finished"), EAutomationExpectedErrorFlags::Contains, 1);
	Pawn->Destroy();
	TestTrue(TEXT("Unexpected pawn removal fails spawner"), S->HasSpawnFailure());
	TestTrue(TEXT("Failure propagates to encounter"), E->HasEncounterFailure());
	TestFalse(TEXT("Failure is not a clear"), E->IsEncounterCompleted());
	TestEqual(TEXT("No fake kill/complete"), F.Observer->Encounters, 0);
	TestTrue(TEXT("Orphan controller removed"), Gone(Bot));
	TestTrue(TEXT("Orphan state removed"), Gone(PS));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteTeardownTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.OwnerTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteTeardownTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* S = F.Spawner(); auto* Other = F.Spawner();
	if (!TestNotNull(TEXT("Spawner"), S) || !TestNotNull(TEXT("Other spawner"), Other)) { return false; }
	ADCLyraPlayerBotController* Bot = nullptr; ADCLyraPlayerBotController* OtherBot = nullptr;
	auto* Pawn = F.Adopt(*this, S, Bot); auto* OtherPawn = F.Adopt(*this, Other, OtherBot);
	if (!Pawn || !OtherPawn) { return false; }
	APlayerState* PS = Bot->PlayerState;
	auto* E = F.Encounter(S); if (!TestNotNull(TEXT("Encounter"), E)) { return false; }
	E->StartEncounter();
	S->Destroy();
	TestTrue(TEXT("Removed pending spawner fails encounter"), E->HasEncounterFailure());
	TestFalse(TEXT("Cleanup deferred outside possible death callback"), Gone(PS));
	F.World.TickTestWorld(.02f);
	TestTrue(TEXT("Owned pawn/controller/state removed"), Gone(Pawn) && Gone(Bot) && Gone(PS));
	TestFalse(TEXT("Other spawner pawn preserved"), Gone(OtherPawn));
	TestFalse(TEXT("Other controller preserved"), Gone(OtherBot));
	TestEqual(TEXT("EndPlay cannot emit completion"), F.Observer->Encounters, 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFiniteSpawnRollbackTest, "DreamCatcher.R10.LyraAI.FiniteSpawner.ExperienceGateAndSpawnRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFiniteSpawnRollbackTest::RunTest(const FString&)
{
	using namespace DCLyraFiniteSpawnerTests;
	FFixture F; if (!F.Start(*this)) { return false; }
	auto* S = F.Spawner(); if (!TestNotNull(TEXT("Spawner"), S)) { return false; }
	S->ActivateSpawner(); S->ActivateSpawner();
	F.World.TickTestWorld(.02f);
	TestEqual(TEXT("No spawning before Experience callback"), S->GetSpawnedCount(), 0);
	int32 BotsBefore = 0;
	for (TActorIterator<ADCLyraPlayerBotController> It(F.World.GetTestWorld()); It; ++It) { ++BotsBefore; }
	TestEqual(TEXT("No premature controllers"), BotsBefore, 0);
	int32 StatesBefore = 0;
	for (TActorIterator<ADCPlayerState> It(F.World.GetTestWorld()); It; ++It) { if (!Gone(*It)) { ++StatesBefore; } }
	AddExpectedError(TEXT("Lyra pawn creation/possession failed"), EAutomationExpectedErrorFlags::Contains, 1);
	// Only deliver the callback to test rollback. This is NOT an Experience loading test.
	FDCLyraFiniteSpawnerTestAccess::DeliverExperienceCallback(S);
	F.World.TickTestWorld(.02f);
	TestTrue(TEXT("Failed pawn creation fails the finite spawner"), S->HasSpawnFailure());
	TestFalse(TEXT("Failure does not report cleared"), S->IsSpawnerCleared());
	F.World.TickTestWorld(.02f); F.World.TickTestWorld(.10f);
	int32 BotsAfter = 0;
	for (TActorIterator<ADCLyraPlayerBotController> It(F.World.GetTestWorld()); It; ++It) { if (!Gone(*It)) { ++BotsAfter; } }
	TestEqual(TEXT("Queued original restart cannot create a stray bot"), BotsAfter, 0);
	int32 StatesAfter = 0;
	for (TActorIterator<ADCPlayerState> It(F.World.GetTestWorld()); It; ++It) { if (!Gone(*It)) { ++StatesAfter; } }
	TestEqual(TEXT("Failed spawn leaves no PlayerState"), StatesAfter, StatesBefore);
	TestEqual(TEXT("No counted spawn on failure"), S->GetSpawnedCount(), 0);
	AddInfo(TEXT("Native ownership/count/order and failure tests only. Real Blueprint spawn, inventory, locomotion and replication require the isolated map PIE."));
	return !HasAnyErrors();
}

#endif
