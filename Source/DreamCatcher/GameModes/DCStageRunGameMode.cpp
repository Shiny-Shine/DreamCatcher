#include "GameModes/DCStageRunGameMode.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Character/DCLyraHealthComponent.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DCPlayerState.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Teams/DCTeamSubsystem.h"
#include "TimerManager.h"
#include "UI/Stage/DCStageResultWidget.h"
#include "DreamCatcher.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCStageRunGameMode)

#define LOCTEXT_NAMESPACE "DCStageRun"

ADCStageRunGameMode::ADCStageRunGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ResultWidgetClass = UDCStageResultWidget::StaticClass();
	PhaseTitle = LOCTEXT("Preparing", "준비 중");
}

void ADCStageRunGameMode::InitGameState()
{
	Super::InitGameState();
	UDCLyraExperienceManagerComponent* Experience = GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>();
	check(Experience);
	Experience->CallOrRegister_OnExperienceLoaded_LowPriority(
		FOnDCLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnStageExperienceReady));
}

void ADCStageRunGameMode::StartPlay()
{
	Super::StartPlay();
	for (TActorIterator<ADCStageDirector> It(GetWorld()); It; ++It)
	{
		if (Director.IsValid())
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Stage map must contain exactly one director."));
			EndRun(false);
			return;
		}
		Director = *It;
	}
	if (!Director.IsValid())
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Stage director missing."));
		EndRun(false);
		return;
	}
	Director->OnStagePhaseStarted.AddUniqueDynamic(this, &ThisClass::HandlePhaseStarted);
	Director->OnStageEnded.AddUniqueDynamic(this, &ThisClass::HandleStageEnded);
	Director->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleDirectorDestroyed);
	MaybeStartRun();
}

void ADCStageRunGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
	if (bExperienceReady) { ApplyStageTeam(NewPlayer); }
	if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
	{
		if (PC->PlayerState && PC->PlayerState->IsOnlyASpectator()) { return; }
		if (RunPlayer.IsValid() && RunPlayer.Get() != PC)
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Stage run supports one playable controller."));
			EndRun(false);
			return;
		}
		RunPlayer = PC;
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePlayerPawnChanged);
		BindPlayerPawn(PC->GetPawn());
		if (bRunEnded && !bOwnsInputIgnore) { ResultTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::PresentResult); }
	}
}

void ADCStageRunGameMode::OnStageExperienceReady(const UDCLyraExperienceDefinition*)
{
	if (bEndingPlay || bRunEnded || bExperienceReady) { return; }
	const UDCTeamSubsystem* Teams = GetWorld()->GetSubsystem<UDCTeamSubsystem>();
	if (!Teams || PlayerTeamId < 0 || PlayerTeamId > 254 || EnemyTeamId < 0 || EnemyTeamId > 254
		|| PlayerTeamId == EnemyTeamId || !Teams->DoesTeamExist(PlayerTeamId) || !Teams->DoesTeamExist(EnemyTeamId)
		|| !FMath::IsFinite(PlayerReadyTimeout) || PlayerReadyTimeout <= 0.0f)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Invalid team setup or player readiness timeout."));
		EndRun(false);
		return;
	}
	bExperienceReady = true;
	// Original team creation/assignment has run. Only this opt-in GameMode fixes PvE teams.
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It) { ApplyStageTeam(It->Get()); }
	ReadyDeadline = GetWorld()->GetTimeSeconds() + PlayerReadyTimeout;
	GetWorldTimerManager().SetTimer(ReadyTimer, this, &ThisClass::MaybeStartRun, .1f, true);
}

void ADCStageRunGameMode::ApplyStageTeam(AController* Controller)
{
	if (!Controller || !HasAuthority()) { return; }
	ADCPlayerState* PS = Controller->GetPlayerState<ADCPlayerState>();
	if (!PS || PS->IsOnlyASpectator()) { return; }
	if (Controller->IsPlayerController()) { PS->SetGenericTeamId(IntegerToGenericTeamId(PlayerTeamId)); }
	else if (Controller->IsA<ADCLyraPlayerBotController>()) { PS->SetGenericTeamId(IntegerToGenericTeamId(EnemyTeamId)); }
}

void ADCStageRunGameMode::BindPlayerPawn(APawn* Pawn)
{
	if (UDCLyraHealthComponent* OldHealth = PlayerHealth.Get())
	{
		OldHealth->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeath);
	}
	PlayerHealth = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	if (UDCLyraHealthComponent* Health = PlayerHealth.Get())
	{
		Health->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeath);
		if (Health->IsDeadOrDying()) { HandlePlayerDeath(Pawn); }
	}
}

void ADCStageRunGameMode::HandlePlayerPawnChanged(APawn*, APawn* NewPawn)
{
	if (bEndingPlay) { return; }
	BindPlayerPawn(NewPawn);
	if (bStartedRun && !bRunEnded && !NewPawn)
	{
		if (Director.IsValid()) { Director->FailStage(); }
		else { EndRun(false); }
	}
}

void ADCStageRunGameMode::MaybeStartRun()
{
	if (bEndingPlay || bRunEnded || bStartedRun || !bExperienceReady) { return; }
	APlayerController* PC = RunPlayer.Get();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UDCPawnExtensionComponent* Extension = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (Director.IsValid() && Pawn && Extension && Extension->GetDCAbilitySystemComponent()
		&& PlayerHealth.IsValid() && PlayerHealth->GetMaxHealth() > 0.0f && !PlayerHealth->IsDeadOrDying())
	{
		GetWorldTimerManager().ClearTimer(ReadyTimer);
		if (Director->GetRunState() != EDCStageRunState::NotStarted)
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Disable director auto-start in the stage map; GameMode starts after readiness."));
			Director->FailStage(); EndRun(false); return;
		}
		bStartedRun = true;
		EnsureStageWidget();
		Director->StartStage();
	}
	else if (ReadyDeadline > 0.0 && GetWorld()->GetTimeSeconds() >= ReadyDeadline)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R11] Player/stage initialization did not become ready."));
		if (Director.IsValid()) { Director->FailStage(); }
		EndRun(false);
	}
}

void ADCStageRunGameMode::HandlePlayerDeath(AActor* Pawn)
{
	if (bEndingPlay || bRunEnded || !PlayerHealth.IsValid() || PlayerHealth->GetOwner() != Pawn || !PlayerHealth->IsDeadOrDying()) { return; }
	if (Director.IsValid()) { Director->FailStage(); }
	else { EndRun(false); }
}

void ADCStageRunGameMode::HandlePhaseStarted(ADCStageDirector* Stage, int32, const FText& Title, EDCStagePhaseType)
{
	if (bEndingPlay || bRunEnded || Director.Get() != Stage) { return; }
	PhaseTitle = Title;
	EnsureStageWidget();
	if (StageWidget) { StageWidget->SetStageTitle(Title); }
}

void ADCStageRunGameMode::HandleStageEnded(ADCStageDirector* Stage, bool bSucceeded)
{
	if (Director.Get() == Stage) { EndRun(bSucceeded); }
}

void ADCStageRunGameMode::HandleDirectorDestroyed(AActor* Stage)
{
	if (Director.Get() == Stage && !GetWorld()->bIsTearingDown) { EndRun(false); }
}

void ADCStageRunGameMode::EndRun(bool bSucceeded)
{
	if (bEndingPlay || bRunEnded) { return; }
	bRunEnded = true;
	bRunSucceeded = bSucceeded;
	GetWorldTimerManager().ClearTimer(ReadyTimer);
	if (!bSucceeded && Director.IsValid()) { Director->FailStage(); }
	// Defer presentation/input cancellation outside damage and phase delegate stacks.
	ResultTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::PresentResult);
}

void ADCStageRunGameMode::EnsureStageWidget()
{
	if (bUseExperienceUI) { return; }
	APlayerController* PC = RunPlayer.Get();
	if (StageWidget || !PC || !PC->GetLocalPlayer() || !ResultWidgetClass || GetNetMode() == NM_DedicatedServer) { return; }
	StageWidget = CreateWidget<UDCStageResultWidget>(PC, ResultWidgetClass);
	if (StageWidget)
	{
		StageWidget->SetStageTitle(PhaseTitle);
		StageWidget->OnRetryRequested.AddUniqueDynamic(this, &ThisClass::HandleRetryClicked);
		StageWidget->OnExitRequested.AddUniqueDynamic(this, &ThisClass::HandleExitClicked);
		StageWidget->AddToViewport(20);
	}
}

void ADCStageRunGameMode::PresentResult()
{
	if (bEndingPlay || !bRunEnded || bOwnsInputIgnore || GetWorld()->bIsTearingDown) { return; }
	EnsureStageWidget();
	APlayerController* PC = RunPlayer.Get();
	if (!PC || !PC->GetLocalPlayer() || (!StageWidget && !bUseExperienceUI)) { return; }
	if (ADCPlayerState* PS = PC->GetPlayerState<ADCPlayerState>())
	{
		UDCAbilitySystemComponent* ASC = PS->GetDCAbilitySystemComponent();
		ASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
		InputBlockedASC = ASC;
		ASC->ClearAbilityInput();
		ASC->CancelAimInputAndState();
		// Failure's original Death Ability must survive its full sequence.
		if (PlayerHealth.IsValid() && !PlayerHealth->IsDeadOrDying())
		{
			ASC->CancelAbilitiesByFunc([](const UDCGameplayAbility* Ability, FGameplayAbilitySpecHandle)
			{
				const EDCAbilityActivationPolicy Policy = Ability->GetActivationPolicy();
				return Ability->CanBeCanceled() && (Policy == EDCAbilityActivationPolicy::OnInputTriggered
					|| Policy == EDCAbilityActivationPolicy::WhileInputActive);
			}, false);
		}
	}
	bPreviousMouseCursor = PC->bShowMouseCursor;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	bOwnsInputIgnore = true;
	PC->bShowMouseCursor = true;
	if (!bUseExperienceUI)
	{
		StageWidget->ShowResult(bRunSucceeded);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, StageWidget->GetResultFocusTarget(), EMouseLockMode::DoNotLock, true);
	}
}

bool ADCStageRunGameMode::RequestExitStageFromMenu()
{
	if (!bUseExperienceUI || bEndingPlay || bTravelPending || ReturnMap.IsNull() || GetNetMode() != NM_Standalone) { return false; }
	if (!bRunEnded) { EndRun(false); }
	return RequestExitStage();
}

bool ADCStageRunGameMode::RequestRestartStage()
{
	return QueueTravel(FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

bool ADCStageRunGameMode::RequestExitStage()
{
	if (ReturnMap.IsNull()) { return false; }
	return QueueTravel(FName(*ReturnMap.ToSoftObjectPath().GetLongPackageName()));
}

bool ADCStageRunGameMode::QueueTravel(FName LevelName)
{
	if (!bRunEnded || bEndingPlay || bTravelPending || LevelName.IsNone() || GetNetMode() != NM_Standalone) { return false; }
	bTravelPending = true;
	PendingTravel = LevelName;
	if (StageWidget) { StageWidget->SetTravelPending(true); }
	TravelTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ExecuteTravel);
	return true;
}

void ADCStageRunGameMode::ExecuteTravel()
{
	if (!bEndingPlay && bTravelPending && !GetWorld()->bIsTearingDown) { PerformLevelTravel(PendingTravel); }
}

void ADCStageRunGameMode::PerformLevelTravel(FName LevelName)
{
	UGameplayStatics::OpenLevel(this, LevelName);
}

void ADCStageRunGameMode::HandleRetryClicked() { RequestRestartStage(); }
void ADCStageRunGameMode::HandleExitClicked() { RequestExitStage(); }

void ADCStageRunGameMode::CleanupPresentation()
{
	if (UDCAbilitySystemComponent* ASC = InputBlockedASC.Get())
	{
		ASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	}
	InputBlockedASC.Reset();
	if (StageWidget)
	{
		StageWidget->OnRetryRequested.RemoveDynamic(this, &ThisClass::HandleRetryClicked);
		StageWidget->OnExitRequested.RemoveDynamic(this, &ThisClass::HandleExitClicked);
		StageWidget->RemoveFromParent();
		StageWidget = nullptr;
	}
	if (APlayerController* PC = RunPlayer.Get(); PC && bOwnsInputIgnore)
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->bShowMouseCursor = bPreviousMouseCursor;
		PC->SetInputMode(FInputModeGameOnly());
	}
	bOwnsInputIgnore = false;
}

void ADCStageRunGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(ReadyTimer);
	GetWorldTimerManager().ClearTimer(ResultTimer);
	GetWorldTimerManager().ClearTimer(TravelTimer);
	if (ADCStageDirector* Stage = Director.Get())
	{
		Stage->OnStagePhaseStarted.RemoveDynamic(this, &ThisClass::HandlePhaseStarted);
		Stage->OnStageEnded.RemoveDynamic(this, &ThisClass::HandleStageEnded);
		Stage->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleDirectorDestroyed);
	}
	if (APlayerController* PC = RunPlayer.Get()) { PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePlayerPawnChanged); }
	if (UDCLyraHealthComponent* Health = PlayerHealth.Get()) { Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeath); }
	CleanupPresentation();
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
