#include "Tests/DCLyraGamePhaseTestTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraGamePhaseTestTypes)

UDCR10RootPhaseFixture::UDCR10RootPhaseFixture()
{
	GamePhaseTag = FGameplayTag::RequestGameplayTag(FName(TEXT("ShooterGame.GamePhase")));
}
UDCR10PostGamePhaseFixture::UDCR10PostGamePhaseFixture()
{
	GamePhaseTag = FGameplayTag::RequestGameplayTag(FName(TEXT("ShooterGame.GamePhase.PostGame")));
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Phases/DCLyraGamePhaseSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCLyraAIFoundationTestTypes.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraGamePhaseValidationTest,
	"DreamCatcher.R10.LyraAI.PhaseDataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraGamePhaseValidationTest::RunTest(const FString& Parameters)
{
	FDataValidationContext EmptyContext;
	const EDataValidationResult Empty = GetDefault<UDCLyraGamePhaseAbility>()->IsDataValid(EmptyContext);
	TestTrue(TEXT("Original phase requires a tag"), Empty == EDataValidationResult::Invalid);
	TestEqual(TEXT("Missing phase tag error"), EmptyContext.GetNumErrors(), uint32(1));
	FDataValidationContext ValidContext;
	TestTrue(TEXT("Original PostGame tag validates"),
		GetDefault<UDCR10PostGamePhaseFixture>()->IsDataValid(ValidContext) == EDataValidationResult::Valid);
	TestEqual(TEXT("Configured phase has no validation error"), ValidContext.GetNumErrors(), uint32(0));
	TestTrue(TEXT("Original pickup Cue tag registered"),
		FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.ShooterGame.Interact.WeaponPickup")), false).IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCLyraGamePhaseLifecycleTest,
	"DreamCatcher.R10.LyraAI.PhaseNestedLifecycleAndGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCLyraGamePhaseLifecycleTest::RunTest(const FString& Parameters)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCR10LyraAIIsolatedAutomation")))
	{
		AddError(TEXT("Use a separate unattended editor with -DCR10LyraAIIsolatedAutomation."));
		return false;
	}
	// Declared before the world so callbacks remain valid during world teardown on an early return.
	TStrongObjectPtr<UDCR10PhaseObserverFixture> Observer(NewObject<UDCR10PhaseObserverFixture>());
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = ADCR10ExperienceWaitGameModeFixture::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	UDCLyraGamePhaseSubsystem* Phases = World->GetSubsystem<UDCLyraGamePhaseSubsystem>();
	UDCAbilitySystemComponent* ASC = World->GetGameState()->FindComponentByClass<UDCAbilitySystemComponent>();
	if (!TestNotNull(TEXT("Original phase subsystem"), Phases) || !TestNotNull(TEXT("GameState ASC"), ASC)) { return false; }
	const FGameplayTag Root = FGameplayTag::RequestGameplayTag(FName(TEXT("ShooterGame.GamePhase")));
	const FGameplayTag PostGame = FGameplayTag::RequestGameplayTag(FName(TEXT("ShooterGame.GamePhase.PostGame")));
	TestFalse(TEXT("No phase is automatically started"), Phases->IsPhaseActive(Root));
	Phases->WhenPhaseStartsOrIsActive(Root, EDCLyraPhaseTagMatchType::PartialMatch,
		FDCLyraGamePhaseTagDelegate::CreateUObject(Observer.Get(), &UDCR10PhaseObserverFixture::Started));
	Phases->WhenPhaseEnds(Root, EDCLyraPhaseTagMatchType::PartialMatch,
		FDCLyraGamePhaseTagDelegate::CreateUObject(Observer.Get(), &UDCR10PhaseObserverFixture::Ended));
	const FDCLyraGamePhaseDelegate EndCallback = FDCLyraGamePhaseDelegate::CreateUObject(Observer.Get(), &UDCR10PhaseObserverFixture::AbilityEnded);
	Phases->StartPhase(UDCR10RootPhaseFixture::StaticClass(), EndCallback);
	TestTrue(TEXT("Parent phase active"), Phases->IsPhaseActive(Root));
	TestFalse(TEXT("Parent alone does not satisfy the pickup PostGame gate"), Phases->IsPhaseActive(PostGame));
	Phases->StartPhase(UDCR10PostGamePhaseFixture::StaticClass(), EndCallback);
	TestTrue(TEXT("Parent retained beside child"), Phases->IsPhaseActive(Root));
	TestTrue(TEXT("Original pickup PostGame gate sees the child"), Phases->IsPhaseActive(PostGame));
	TestEqual(TEXT("Both phase starts observed"), Observer->Starts, 2);
	FGameplayAbilitySpec* Child = ASC->FindAbilitySpecFromClass(UDCR10PostGamePhaseFixture::StaticClass());
	if (!TestNotNull(TEXT("Child ability spec"), Child)) { return false; }
	ASC->CancelAbilityHandle(Child->Handle);
	TestFalse(TEXT("Ending child releases the PostGame gate"), Phases->IsPhaseActive(PostGame));
	TestTrue(TEXT("Ending child preserves parent"), Phases->IsPhaseActive(Root));
	FGameplayAbilitySpec* Parent = ASC->FindAbilitySpecFromClass(UDCR10RootPhaseFixture::StaticClass());
	if (!TestNotNull(TEXT("Parent ability spec"), Parent)) { return false; }
	ASC->CancelAbilityHandle(Parent->Handle);
	TestFalse(TEXT("All fixture phases ended"), Phases->IsPhaseActive(Root));
	TestEqual(TEXT("Both phase ends observed"), Observer->Ends, 2);
	TestEqual(TEXT("Both original completion delegates called"), Observer->AbilityEnds, 2);
	AddInfo(TEXT("Transient GameState/phase lifecycle only. Actual pickup Blueprint, BT, replication and live match flow remain separate."));
	return !HasAnyErrors();
}

#endif
