// Isolated checks for the original-derived team observer and PlayerState owner getter.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "DreamCatcherPlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/DCPlayerState.h"
#include "Teams/DCLyraAsyncAction_ObserveTeamColors.h"
#include "Teams/DCTeamSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCTeamColorObserverContractTest,
	"DreamCatcher.R5.TeamColor.ObserverAndController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCTeamColorObserverContractTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCR5IsolatedAutomation")))
	{
		AddError(TEXT("Use a separate UnrealEditor-Cmd process with -DCR5IsolatedAutomation, not the active editor."));
		return false;
	}

	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game))
	{
		Fixture.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	UDCTeamSubsystem* Teams = World->GetSubsystem<UDCTeamSubsystem>();
	ADCPlayerState* State = World->SpawnActor<ADCPlayerState>();
	ADreamCatcherPlayerController* Controller = World->SpawnActor<ADreamCatcherPlayerController>();
	APlayerController* GenericController = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Team subsystem"), Teams) || !TestNotNull(TEXT("PlayerState"), State)
		|| !TestNotNull(TEXT("Project Controller"), Controller) || !TestNotNull(TEXT("Generic Controller"), GenericController))
	{
		return false;
	}
	// Do not start gameplay, create a HUD, possess a production Pawn, or change any saved assets.
	State->SetOwner(nullptr);
	TestNull(TEXT("No owner returns no project Controller"), State->GetDCPlayerController());
	State->SetOwner(GenericController);
	TestNull(TEXT("Generic Controller does not satisfy the project-specific cast"), State->GetDCPlayerController());
	State->SetOwner(Controller);
	TestTrue(TEXT("Project Controller owner is returned"), State->GetDCPlayerController() == Controller);

	TestNull(TEXT("Null agent produces no async action"), UDCLyraAsyncAction_ObserveTeamColors::ObserveTeamColors(nullptr));
	State->SetGenericTeamId(IntegerToGenericTeamId(1));
	TStrongObjectPtr<UDCLyraAsyncAction_ObserveTeamColors> Observer(
		UDCLyraAsyncAction_ObserveTeamColors::ObserveTeamColors(State));
	if (!TestNotNull(TEXT("Observer"), Observer.Get()))
	{
		return false;
	}
	Observer->Activate();
	TestTrue(TEXT("Observer registered with GameInstance"), Observer->IsActive());
	// Dynamic multicast delegates expose GetAllObjects, not the single-cast IsBoundToObject API.
	TestTrue(TEXT("Team change subscription established"), State->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Observer.Get()));
	TestTrue(TEXT("Initial display asset subscription established"), Teams->GetTeamDisplayAssetChangedDelegate(1).GetAllObjects().Contains(Observer.Get()));

	State->SetGenericTeamId(IntegerToGenericTeamId(2));
	TestFalse(TEXT("Previous team's display subscription released"), Teams->GetTeamDisplayAssetChangedDelegate(1).GetAllObjects().Contains(Observer.Get()));
	TestTrue(TEXT("New team's display subscription established"), Teams->GetTeamDisplayAssetChangedDelegate(2).GetAllObjects().Contains(Observer.Get()));
	State->SetGenericTeamId(FGenericTeamId::NoTeam);
	TestFalse(TEXT("NoTeam releases the previous display subscription"), Teams->GetTeamDisplayAssetChangedDelegate(2).GetAllObjects().Contains(Observer.Get()));
	Observer->Cancel();
	TestFalse(TEXT("Canceled observer inactive"), Observer->IsActive());
	TestFalse(TEXT("Cancel releases the agent team-change subscription"), State->GetTeamChangedDelegateChecked().GetAllObjects().Contains(Observer.Get()));

	AActor* NonTeamActor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Non-team actor"), NonTeamActor))
	{
		return false;
	}
	TStrongObjectPtr<UDCLyraAsyncAction_ObserveTeamColors> NonTeamObserver(
		UDCLyraAsyncAction_ObserveTeamColors::ObserveTeamColors(NonTeamActor));
	if (!TestNotNull(TEXT("Non-team observer"), NonTeamObserver.Get()))
	{
		return false;
	}
	NonTeamObserver->Activate();
	TestFalse(TEXT("Non-team observer ends after its initial notification path"), NonTeamObserver->IsActive());
	AddInfo(TEXT("Checks owner-cast and subscription routing only. Actual color output, cancellation while still assigned to a team, Blueprint integration, replication and asset-copy stability remain separate."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
