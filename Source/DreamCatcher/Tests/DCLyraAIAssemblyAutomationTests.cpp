#include "Tests/DCLyraAIAssemblyTestTypes.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Player/DCPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraAIAssemblyTestTypes)

ADCR10AssemblyGameModeFixture::ADCR10AssemblyGameModeFixture()
{
	PlayerStateClass = ADCPlayerState::StaticClass();
	DefaultPawnClass = ACharacter::StaticClass();
}

void ADCR10AssemblyGameModeFixture::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// Keep the real GameMode team delegate, but do not select a saved Experience.
	AGameModeBase::InitGame(MapName, Options, ErrorMessage);
}

void ADCR10AssemblyGameModeFixture::InitGameState()
{
	AGameModeBase::InitGameState();
}

UClass* ADCR10AssemblyGameModeFixture::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// The original PlayerStart occupancy query needs a real capsule CDO.
	return ACharacter::StaticClass();
}

void UDCR10TeamCreationFixture::ConfigureTwoTeams()
{
	// Insert in reverse order to exercise the original lower-ID tie break.
	TeamsToCreate.Add(2, nullptr);
	TeamsToCreate.Add(1, nullptr);
}

#if WITH_SERVER_CODE
void UDCR10TeamCreationFixture::CreateTeamsForTest() { ServerCreateTeams(); }
void UDCR10TeamCreationFixture::AssignExistingForTest() { ServerAssignPlayersToTeams(); }
void UDCR10TeamCreationFixture::ChooseTeamForTest(ADCPlayerState* State) { ServerChooseTeamForPlayer(State); }
#endif

void ADCR10TeamPlayerControllerFixture::AssignFixturePlayerState(APlayerState* NewState)
{
	SetPlayerState(NewState);
	OnRep_PlayerState();
}

void ADCR10TeamPlayerControllerFixture::CleanupFixturePlayerState()
{
	CleanupPlayerState();
}

void ADCR10TeamPlayerControllerFixture::BeginPlay()
{
	APlayerController::BeginPlay();
}

void ADCR10TeamPlayerControllerFixture::OnPossess(APawn* InPawn)
{
	APlayerController::OnPossess(InPawn);
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/Lyra/DCLyraPlayerStart.h"
#include "Player/Lyra/DCLyraTDMPlayerSpawningManagementComponent.h"
#include "Teams/DCLyraAsyncAction_ObserveTeam.h"
#include "Teams/DCTeamSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIFoundationTestTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace DCLyraAIAssemblyTests
{
bool StartWorld(FAutomationTestBase& Test, FTestWorldWrapper& Fixture)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR10LyraAIIsolatedAutomation")))
	{
		Test.AddError(TEXT("Use an isolated unattended editor with -DCR10LyraAIIsolatedAutomation; no assets are saved."));
		return false;
	}
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(&Test); return false; }
	UWorld* World = Fixture.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = ADCR10AssemblyGameModeFixture::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(&Test); return false; }
	return Test.TestTrue(TEXT("Actor callbacks enabled after world initialization"), World->AreActorsInitialized())
		&& Test.TestNotNull(TEXT("Original-derived GameState"), World->GetGameState<ADCLyraGameState>());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAITeamCreationTest,
	"DreamCatcher.R10.LyraAI.Assembly.TeamCreationAndAssignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAITeamCreationTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCLyraAIAssemblyTests::StartWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	ADCLyraGameState* GameState = World->GetGameState<ADCLyraGameState>();
	UDCTeamSubsystem* Teams = World->GetSubsystem<UDCTeamSubsystem>();
	if (!TestNotNull(TEXT("Team subsystem"), Teams)) { return false; }
	UDCLyraExperienceManagerComponent* Experience = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	if (!TestNotNull(TEXT("Real Experience component"), Experience)) { return false; }
	TestFalse(TEXT("Fixture does not fabricate a loaded Experience"), Experience->IsExperienceLoaded());
	UDCR10TeamCreationFixture* Creator = NewObject<UDCR10TeamCreationFixture>(GameState);
	Creator->ConfigureTwoTeams();
	Creator->RegisterComponent();
	TestEqual(TEXT("No automatic teams before Experience completion"), Teams->GetTeamIDs().Num(), 0);
	Creator->CreateTeamsForTest(); // Exercise native rules, not the untested asset loading path.
	TestEqual(TEXT("Two original configured teams registered"), Teams->GetTeamIDs().Num(), 2);
	TestTrue(TEXT("Team 1 exists"), Teams->DoesTeamExist(1));
	TestTrue(TEXT("Team 2 exists"), Teams->DoesTeamExist(2));

	ADCPlayerState* First = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* Second = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* Spectator = World->SpawnActor<ADCPlayerState>();
	if (!TestNotNull(TEXT("First state"), First) || !TestNotNull(TEXT("Second state"), Second)
		|| !TestNotNull(TEXT("Spectator"), Spectator)) { return false; }
	Spectator->SetIsOnlyASpectator(true);
	Spectator->SetGenericTeamId(IntegerToGenericTeamId(2));
	// Clear spectator membership before testing population order, as the original
	// ChooseTeam contract does; don't silently change how the source counts players.
	Creator->ChooseTeamForTest(Spectator);
	TestEqual(TEXT("Spectator stripped of team"), Spectator->GetTeamId(), INDEX_NONE);
	Creator->AssignExistingForTest();
	TestTrue(TEXT("Existing players assigned to opposite teams"),
		(First->GetTeamId() == 1 && Second->GetTeamId() == 2)
		|| (First->GetTeamId() == 2 && Second->GetTeamId() == 1));
	TestEqual(TEXT("Spectator remains unassigned"), Spectator->GetTeamId(), INDEX_NONE);

	ADCR10BotControllerFixture* NewBot = World->SpawnActor<ADCR10BotControllerFixture>();
	ADCPlayerState* NewState = World->SpawnActor<ADCPlayerState>();
	if (!TestNotNull(TEXT("New controller"), NewBot) || !TestNotNull(TEXT("New state"), NewState)) { return false; }
	NewState->SetOwner(NewBot);
	NewBot->AssignFixturePlayerState(NewState);
	World->GetAuthGameMode<ADCLyraGameMode>()->GenericPlayerInitialization(NewBot);
	TestEqual(TEXT("New-player delegate uses lower-ID tie break"), NewState->GetTeamId(), 1);

	ADCPlayerState* Extra = World->SpawnActor<ADCPlayerState>();
	if (!TestNotNull(TEXT("Extra state"), Extra)) { return false; }
	Creator->ChooseTeamForTest(Extra);
	TestEqual(TEXT("Least populated team chosen"), Extra->GetTeamId(), 2);
	ADCPlayerState* Inactive = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* Tie = World->SpawnActor<ADCPlayerState>();
	if (!TestNotNull(TEXT("Inactive state"), Inactive) || !TestNotNull(TEXT("Tie state"), Tie)) { return false; }
	Inactive->SetGenericTeamId(IntegerToGenericTeamId(1));
	Inactive->SetIsInactive(true);
	Creator->ChooseTeamForTest(Tie);
	TestEqual(TEXT("Inactive members excluded from tied population"), Tie->GetTeamId(), 1);
	TestFalse(TEXT("No implicit match phase or Experience start"), Experience->IsExperienceLoaded());
	AddInfo(TEXT("Native team creation/balancing and GameMode delegate only; asset activation, colors and replication remain untested."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAITeamSpawnTest,
	"DreamCatcher.R10.LyraAI.Assembly.TeamSpawnSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAITeamSpawnTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCLyraAIAssemblyTests::StartWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	ADCR10BotControllerFixture* Spawning = World->SpawnActor<ADCR10BotControllerFixture>();
	ADCR10BotControllerFixture* Enemy = World->SpawnActor<ADCR10BotControllerFixture>();
	ADCPlayerState* FriendlyState = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* EnemyState = World->SpawnActor<ADCPlayerState>();
	APawn* EnemyPawn = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Spawning controller"), Spawning) || !TestNotNull(TEXT("Enemy controller"), Enemy)
		|| !TestNotNull(TEXT("Friendly state"), FriendlyState) || !TestNotNull(TEXT("Enemy state"), EnemyState)
		|| !TestNotNull(TEXT("Enemy pawn"), EnemyPawn)) { return false; }
	FriendlyState->SetOwner(Spawning);
	FriendlyState->SetGenericTeamId(IntegerToGenericTeamId(1));
	Spawning->AssignFixturePlayerState(FriendlyState);
	EnemyState->SetOwner(Enemy);
	EnemyState->SetGenericTeamId(IntegerToGenericTeamId(2));
	Enemy->AssignFixturePlayerState(EnemyState);
	Enemy->Possess(EnemyPawn);
	if (!TestTrue(TEXT("Enemy PlayerState owns the pawn used for distance"), EnemyState->GetPawn() == EnemyPawn)) { return false; }
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCLyraPlayerStart* Near = World->SpawnActor<ADCLyraPlayerStart>(FVector(1000, 0, 100), FRotator::ZeroRotator, Spawn);
	ADCLyraPlayerStart* Far = World->SpawnActor<ADCLyraPlayerStart>(FVector(5000, 0, 100), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Near start"), Near) || !TestNotNull(TEXT("Far start"), Far)) { return false; }
	TestTrue(TEXT("Near start has room"), Near->GetLocationOccupancy(Spawning) == EDCLyraPlayerStartLocationOccupancy::Empty);
	TestTrue(TEXT("Far start has room"), Far->GetLocationOccupancy(Spawning) == EDCLyraPlayerStartLocationOccupancy::Empty);
	UDCLyraTDMPlayerSpawningManagementComponent* Rules = NewObject<UDCLyraTDMPlayerSpawningManagementComponent>(World->GetGameState());
	Rules->RegisterComponent();
	TArray<ADCLyraPlayerStart*> Starts { Near, Far };
	TestTrue(TEXT("Furthest available start chosen"), Rules->OnChoosePlayerStart(Spawning, Starts) == Far);
	TestTrue(TEXT("Claim far start"), Far->TryClaim(Enemy));
	TestTrue(TEXT("Available start preferred over claimed fallback"), Rules->OnChoosePlayerStart(Spawning, Starts) == Near);
	TestTrue(TEXT("Claim near start"), Near->TryClaim(Enemy));
	TestTrue(TEXT("Furthest claimed fallback retained"), Rules->OnChoosePlayerStart(Spawning, Starts) == Far);
	EnemyState->SetGenericTeamId(IntegerToGenericTeamId(1));
	TestNull(TEXT("No enemies lets original base spawning policy decide"), Rules->OnChoosePlayerStart(Spawning, Starts));
	Starts.Reset();
	TestNull(TEXT("No candidates returns null"), Rules->OnChoosePlayerStart(Spawning, Starts));
	AddInfo(TEXT("Native team-distance/claim selection only; actual spawn/restart, navigation and blocked geometry require integration."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAIPlayerTeamRelayTest,
	"DreamCatcher.R10.LyraAI.Assembly.PlayerTeamRelayAndAttitude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAIPlayerTeamRelayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCLyraAIAssemblyTests::StartWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	ADCR10TeamPlayerControllerFixture* Player = World->SpawnActor<ADCR10TeamPlayerControllerFixture>();
	ADCR10BotControllerFixture* Bot = World->SpawnActor<ADCR10BotControllerFixture>();
	if (!TestNotNull(TEXT("Player controller"), Player) || !TestNotNull(TEXT("Bot controller"), Bot)) { return false; }
	ADCPlayerState* First = Player->GetPlayerState<ADCPlayerState>();
	ADCPlayerState* Second = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* BotState = World->SpawnActor<ADCPlayerState>();
	APawn* TargetPawn = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("InitPlayerState created compatible state"), First) || !TestNotNull(TEXT("Second state"), Second)
		|| !TestNotNull(TEXT("Bot state"), BotState) || !TestNotNull(TEXT("Target pawn"), TargetPawn)) { return false; }
	TestTrue(TEXT("InitPlayerState binds the original team callback"), First->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Player));
	First->SetGenericTeamId(IntegerToGenericTeamId(1));
	BotState->SetOwner(Bot);
	BotState->SetGenericTeamId(IntegerToGenericTeamId(2));
	Bot->AssignFixturePlayerState(BotState);
	Player->Possess(TargetPawn);
	if (!TestTrue(TEXT("Player controls observed target pawn"), TargetPawn->GetController() == Player)) { return false; }
	TStrongObjectPtr<UDCR10AIObserver> Listener(NewObject<UDCR10AIObserver>());
	TStrongObjectPtr<UDCLyraAsyncAction_ObserveTeam> Observer(UDCLyraAsyncAction_ObserveTeam::ObserveTeam(Player));
	if (!TestNotNull(TEXT("Original observer accepts team-aware player"), Observer.Get())) { return false; }
	Observer->OnTeamChanged.AddDynamic(Listener.Get(), &UDCR10AIObserver::TeamChanged);
	Observer->Activate();
	TestEqual(TEXT("Initial player team"), Listener->LastTeam, 1);
	TestTrue(TEXT("Original bot now sees opposing player as hostile"), Bot->GetTeamAttitudeTowards(*TargetPawn) == ETeamAttitude::Hostile);
	First->SetGenericTeamId(IntegerToGenericTeamId(2));
	TestEqual(TEXT("PlayerState team change forwarded"), Listener->LastTeam, 2);
	TestTrue(TEXT("Same-team player is friendly"), Bot->GetTeamAttitudeTowards(*TargetPawn) == ETeamAttitude::Friendly);
	Second->SetOwner(Player);
	Second->SetGenericTeamId(IntegerToGenericTeamId(1));
	Player->AssignFixturePlayerState(Second);
	TestFalse(TEXT("Old team delegate detached"), First->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Player));
	TestTrue(TEXT("New team delegate bound"), Second->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Player));
	TestEqual(TEXT("Replacement state observed"), Listener->LastTeam, 1);
	const int32 CallsBefore = Listener->TeamCalls;
	First->SetGenericTeamId(IntegerToGenericTeamId(3));
	TestEqual(TEXT("Old state cannot emit controller notifications"), Listener->TeamCalls, CallsBefore);
	Player->CleanupFixturePlayerState();
	TestNull(TEXT("Cleanup removes current PlayerState"), Player->PlayerState.Get());
	TestTrue(TEXT("No PlayerState reports NoTeam"), Player->GetGenericTeamId() == FGenericTeamId::NoTeam);
	TestEqual(TEXT("Cleanup notifies observer of no team"), Listener->LastTeam, INDEX_NONE);
	TestFalse(TEXT("Cleanup detaches replacement state"), Second->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Player));
	Observer->Cancel();
	Player->UnPossess();
	TestTrue(TEXT("Original bot returns neutral for unpossessed target"), Bot->GetTeamAttitudeTowards(*TargetPawn) == ETeamAttitude::Neutral);
	AddInfo(TEXT("Original team methods in opt-in player subclass only; no production UI/loadout, network replication or BT/Perception run."));
	return !HasAnyErrors();
}

#endif
