#include "Tests/DCLyraAIFoundationTestTypes.h"

#include "GameModes/Lyra/DCLyraGameState.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraAIFoundationTestTypes)

ADCR10BotControllerFixture::ADCR10BotControllerFixture()
{
	// Inject unsaved PlayerStates explicitly, avoiding production GameMode/login startup in this fixture.
	bWantsPlayerState = false;
}
void ADCR10BotControllerFixture::AssignFixturePlayerState(APlayerState* NewState)
{
	SetPlayerState(NewState);
	OnRep_PlayerState(); // Exercise the original team rebind path without network transport.
}
void UDCR10AIObserver::TeamChanged(bool bTeamSet, int32 TeamId)
{
	++TeamCalls;
	LastTeam = bTeamSet ? TeamId : INDEX_NONE;
}
void UDCR10AIObserver::ExperienceReady() { ++ReadyCalls; }
ADCR10ExperienceWaitGameModeFixture::ADCR10ExperienceWaitGameModeFixture()
{
	GameStateClass = ADCLyraGameState::StaticClass();
}
void ADCR10WeaponSpawnerFixture::StartFixtureCooldown()
{
	CoolDownTime = 0.05f;
	CheckExistingOverlapDelay = 0.01f;
	bIsWeaponAvailable = false;
	SetWeaponPickupVisibility(false);
	StartCoolDown();
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "CoreGlobals.h"
#include "Development/DCLyraDeveloperSettings.h"
#include "Engine/World.h"
#include "Equipment/Lyra/DCLyraPickupDefinition.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/Lyra/DCLyraAsyncAction_ExperienceReady.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Player/Lyra/DCLyraExperiencePlayerState.h"
#include "System/DCLyraDevelopmentStatics.h"
#include "Teams/DCLyraAsyncAction_ObserveTeam.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace DCLyraAITests
{
bool IsIsolated(FAutomationTestBase& Test)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR10LyraAIIsolatedAutomation")))
	{
		Test.AddError(TEXT("Use a separate unattended UnrealEditor-Cmd with -DCR10LyraAIIsolatedAutomation; no assets are saved."));
		return false;
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAINativeDefaultsTest,
	"DreamCatcher.R10.LyraAI.NativeDefaultsAndSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAINativeDefaultsTest::RunTest(const FString& Parameters)
{
	if (!DCLyraAITests::IsIsolated(*this)) { return false; }
	const ADCLyraGameMode* Mode = GetDefault<ADCLyraGameMode>();
	TestTrue(TEXT("Separate Experience PlayerState"), Mode->PlayerStateClass == ADCLyraExperiencePlayerState::StaticClass());
	TestTrue(TEXT("Original-derived GameState"), Mode->GameStateClass == ADCLyraGameState::StaticClass());
	TestTrue(TEXT("PlayerState remains compatible with current GAS"), ADCLyraExperiencePlayerState::StaticClass()->IsChildOf(ADCPlayerState::StaticClass()));
	const ADCLyraPlayerBotController* Bot = GetDefault<ADCLyraPlayerBotController>();
	const FBoolProperty* WantsState = FindFProperty<FBoolProperty>(Bot->GetClass(), TEXT("bWantsPlayerState"));
	if (TestNotNull(TEXT("Original bot state flag"), WantsState))
	{
		TestTrue(TEXT("Original bot requests PlayerState"), WantsState->GetPropertyValue_InContainer(Bot));
	}
	TestTrue(TEXT("Unassigned bot team is NoTeam"), Bot->GetGenericTeamId() == FGenericTeamId::NoTeam);
	UDCLyraDeveloperSettings* Settings = GetMutableDefault<UDCLyraDeveloperSettings>();
	{
		TGuardValue<bool> Restore(Settings->bAllowPlayerBotsToAttack, false);
		TestFalse(TEXT("Original attack setting can disable attacks"), UDCLyraDevelopmentStatics::CanPlayerBotsAttack());
		Settings->bAllowPlayerBotsToAttack = true;
		TestTrue(TEXT("Original attack setting can enable attacks"), UDCLyraDevelopmentStatics::CanPlayerBotsAttack());
	}
	TestNull(TEXT("Null team has no observer"), UDCLyraAsyncAction_ObserveTeam::ObserveTeam(nullptr));
	AddInfo(TEXT("Native defaults only; no Blueprint AI, Experience asset, replication or online hosting tested."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAITeamAndUnPossessTest,
	"DreamCatcher.R10.LyraAI.TeamAndUnPossess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAITeamAndUnPossessTest::RunTest(const FString& Parameters)
{
	if (!DCLyraAITests::IsIsolated(*this)) { return false; }
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	// Actor::ProcessEvent suppresses dynamic callbacks before AreActorsInitialized().
	// Initialize only this transient fixture world; do not load a production Experience/map.
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	if (!TestTrue(TEXT("Fixture actors initialized for native delegate dispatch"), World->AreActorsInitialized())) { return false; }
	ADCR10BotControllerFixture* Bot = World->SpawnActor<ADCR10BotControllerFixture>();
	ADCPlayerState* First = World->SpawnActor<ADCPlayerState>();
	ADCPlayerState* Second = World->SpawnActor<ADCPlayerState>();
	APawn* Pawn = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Bot"), Bot) || !TestNotNull(TEXT("First state"), First)
		|| !TestNotNull(TEXT("Second state"), Second) || !TestNotNull(TEXT("Pawn"), Pawn)) { return false; }
	First->SetOwner(Bot);
	First->SetGenericTeamId(IntegerToGenericTeamId(1));
	Bot->AssignFixturePlayerState(First);
	TStrongObjectPtr<UDCR10AIObserver> Listener(NewObject<UDCR10AIObserver>());
	TStrongObjectPtr<UDCLyraAsyncAction_ObserveTeam> Observer(UDCLyraAsyncAction_ObserveTeam::ObserveTeam(Bot));
	if (!TestNotNull(TEXT("Team observer"), Observer.Get())) { return false; }
	Observer->OnTeamChanged.AddDynamic(Listener.Get(), &UDCR10AIObserver::TeamChanged);
	Observer->Activate();
	TestEqual(TEXT("Initial team emitted"), Listener->LastTeam, 1);
	First->SetGenericTeamId(IntegerToGenericTeamId(2));
	TestEqual(TEXT("PlayerState change forwarded by bot"), Listener->LastTeam, 2);
	Second->SetOwner(Bot);
	Second->SetGenericTeamId(IntegerToGenericTeamId(3));
	Bot->AssignFixturePlayerState(Second);
	TestFalse(TEXT("Old state subscription removed"), First->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Bot));
	TestTrue(TEXT("New state subscription bound"), Second->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Bot));
	TestEqual(TEXT("Reassigned state observed"), Listener->LastTeam, 3);
	Observer->Cancel();
	const int32 CallsBefore = Listener->TeamCalls;
	Second->SetGenericTeamId(IntegerToGenericTeamId(4));
	TestEqual(TEXT("Cancel prevents later callbacks"), Listener->TeamCalls, CallsBefore);
	Bot->Possess(Pawn);
	TestTrue(TEXT("Fixture possession succeeded"), Bot->GetPawn() == Pawn);
	UDCAbilitySystemComponent* ASC = Second->GetDCAbilitySystemComponent();
	ASC->InitAbilityActorInfo(Second, Pawn);
	Bot->UnPossess();
	TestNull(TEXT("Original bot UnPossess clears its ASC avatar"), ASC->GetAvatarActor());
	Bot->AssignFixturePlayerState(nullptr);
	TestFalse(TEXT("Team subscription cleaned up"), Second->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Bot));
	AddInfo(TEXT("Unsaved world, injected PlayerStates; no BT, damage, production initialization or replication."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAIExperienceWaitTest,
	"DreamCatcher.R10.LyraAI.ExperienceWaitContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAIExperienceWaitTest::RunTest(const FString& Parameters)
{
	if (!DCLyraAITests::IsIsolated(*this)) { return false; }
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = ADCR10ExperienceWaitGameModeFixture::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	UDCLyraExperienceManagerComponent* Manager = World->GetGameState()->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	if (!TestNotNull(TEXT("Experience manager"), Manager)) { return false; }
	TStrongObjectPtr<UDCR10AIObserver> Listener(NewObject<UDCR10AIObserver>());
	TStrongObjectPtr<UDCLyraAsyncAction_ExperienceReady> Action(UDCLyraAsyncAction_ExperienceReady::WaitForExperienceReady(World));
	if (!TestNotNull(TEXT("Original async action"), Action.Get())) { return false; }
	Action->OnReady.AddDynamic(Listener.Get(), &UDCR10AIObserver::ExperienceReady);
	Action->Activate();
	Fixture.TickTestWorld(0.01f);
	Fixture.TickTestWorld(0.02f);
	TestFalse(TEXT("No fabricated Experience-loaded state"), Manager->IsExperienceLoaded());
	TestEqual(TEXT("No early readiness broadcast"), Listener->ReadyCalls, 0);
	Action->SetReadyToDestroy();
	AddInfo(TEXT("Waiting path only; real asset loading and ready callback require the post-build AI map."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraAIPickupCooldownTest,
	"DreamCatcher.R10.LyraAI.PickupCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraAIPickupCooldownTest::RunTest(const FString& Parameters)
{
	if (!DCLyraAITests::IsIsolated(*this)) { return false; }
	TestTrue(TEXT("Original pickup mesh scale default"), GetDefault<UDCLyraWeaponPickupDefinition>()->WeaponMeshScale.Equals(FVector::OneVector));
	TestEqual(TEXT("Null item stat fallback"), ADCLyraWeaponSpawner::GetDefaultStatFromItemDef(nullptr, FGameplayTag()), 0);
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	// No BeginPlay: no saved pickup definition, audio, overlap grant or production map is used.
	ADCR10WeaponSpawnerFixture* Spawner = World->SpawnActor<ADCR10WeaponSpawnerFixture>();
	if (!TestNotNull(TEXT("Spawner"), Spawner)) { return false; }
	TestTrue(TEXT("Pickup starts available"), Spawner->IsAvailable());
	Spawner->StartFixtureCooldown();
	// No gameplay BeginPlay in this fixture. Advance only this world's timer and restore the
	// process frame counter, otherwise FTimerManager ignores repeated ticks in the same frame.
	TGuardValue<uint64> RestoreFrameCounter(GFrameCounter, GFrameCounter + 1);
	World->GetTimerManager().Tick(0.001f); // Activate the pending timer before measuring expiry.
	TestFalse(TEXT("Unavailable during cooldown"), Spawner->IsAvailable());
	++GFrameCounter;
	World->GetTimerManager().Tick(0.03f);
	TestFalse(TEXT("Not available early"), Spawner->IsAvailable());
	++GFrameCounter;
	World->GetTimerManager().Tick(0.03f);
	TestTrue(TEXT("Original cooldown restores availability"), Spawner->IsAvailable());
	World->GetTimerManager().ClearAllTimersForObject(Spawner);
	Spawner->Destroy();
	AddInfo(TEXT("Native cooldown only; BP GiveWeapon, EQS discovery, overlap grant and effects remain unverified."));
	return !HasAnyErrors();
}

#endif
