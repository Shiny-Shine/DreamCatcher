#include "UI/Stage/DCStageOverlayWidget.h"
#include "UI/Stage/DCMissionObjectiveWidget.h"
#include "UI/Boss/DCBossHealthWidget.h"
#include "UI/Radar/DCRadarWidget.h"
#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Stage/DCStageDirector.h"

void UDCStageOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!BossDisplay || !MissionDisplay) { return; }
	BossDisplay->SetVisibility(ESlateVisibility::Collapsed);
	if (MissionDisplay) { MissionDisplay->RequestLatestLive.BindUObject(this, &ThisClass::Refresh); }
	if (GetWorld()) { GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::Refresh, .1f, true); }
	Refresh();
}

void UDCStageOverlayWidget::Refresh()
{
	const ADCStageRunGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADCStageRunGameMode>() : nullptr;
	if (!Mode || !Mode->UsesExperienceUI())
	{
		if (BossDisplay) { BossDisplay->SetVisibility(ESlateVisibility::Collapsed); }
		if (MissionDisplay) { MissionDisplay->ClearMissionStatus(); }
		DisconnectMissionSource(); ClearRadarBinding(); return;
	}
	if (!BossDisplay || !MissionDisplay) { return; }
	if (!Boss.IsValid() && !Mode->HasRunEnded())
	{
		for (TActorIterator<ADCBossCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDeadOrDying()) { Boss = *It; break; }
		}
	}
	ADCBossCharacter* B = Boss.Get();
	const bool bShowBoss = B && !Mode->HasRunEnded() && !B->IsDeadOrDying() && B->GetBossData();
	BossDisplay->SetVisibility(bShowBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShowBoss)
	{
		BossDisplay->SetBossStatus(B->GetBossData(), B->GetHealthComponent()->GetHealth(),
			B->GetHealthComponent()->GetMaxHealth());
	}
	RefreshMissions(Mode);
}

UDCRadarWidget* UDCStageOverlayWidget::FindOwningHUDRadar() const
{
	// The extension widget belongs to this HUD's WidgetTree. Never enumerate
	// unrelated/dormant widgets retained by another world or local player.
	static const FString HUDClassPath(TEXT("/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout.W_ShooterHUDLayout_C"));
	for (UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
	{
		UUserWidget* HUD = Cast<UUserWidget>(Outer);
		if (!HUD || HUD->GetWorld() != GetWorld() || HUD->GetOwningPlayer() != GetOwningPlayer()) { continue; }
		bool bSourceHUD = false;
		for (const UClass* Class = HUD->GetClass(); Class; Class = Class->GetSuperClass())
		{
			if (Class->GetPathName() == HUDClassPath) { bSourceHUD = true; break; }
		}
		if (!bSourceHUD) { continue; }
		UDCRadarWidget* Radar = Cast<UDCRadarWidget>(HUD->GetWidgetFromName(TEXT("Radar")));
		return IsValid(Radar) && Radar->GetWorld() == GetWorld() && Radar->GetOwningPlayer() == GetOwningPlayer() ? Radar : nullptr;
	}
	return nullptr;
}

void UDCStageOverlayWidget::ClearRadarBinding()
{
	if (UDCRadarWidget* Radar = ConnectedRadar.Get(); Radar && bOwnsRadarBinding)
	{
		USceneComponent* Current = Radar->GetObjectiveComponent();
		if (!Current || Current == ConnectedObjective.Get()) { Radar->ClearObjective(); }
	}
	ConnectedRadar.Reset(); ConnectedObjective.Reset(); bOwnsRadarBinding = false;
}

void UDCStageOverlayWidget::RefreshMissions(const ADCStageRunGameMode* Mode)
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = IsValid(PC) ? PC->GetPawn() : nullptr;
	const bool bContextChanged = MissionWorld.Get() != World || MissionMode.Get() != Mode
		|| MissionController.Get() != PC || MissionPawn.Get() != Pawn;
	if (bContextChanged)
	{
		DisconnectMissionSource(); ClearRadarBinding(); MissionDisplay->ClearMissionStatus();
		MissionWorld = World; MissionMode = const_cast<ADCStageRunGameMode*>(Mode); MissionController = PC; MissionPawn = Pawn;
	}
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	ADCStageDirector* Director = IsValid(Mode) ? Mode->GetStageDirector() : nullptr;
	const bool bActive = IsValid(World) && World->IsGameWorld() && !World->bIsTearingDown && !World->IsInSeamlessTravel()
		&& IsValid(Mode) && !Mode->HasRunEnded() && !Mode->IsStageTravelPending()
		&& IsValid(PC) && PC->IsLocalController() && !PC->IsActorBeingDestroyed() && PC->GetWorld() == World
		&& PC == Mode->GetStagePlayerController()
		&& IsValid(Pawn) && Pawn->GetWorld() == World && !Pawn->IsActorBeingDestroyed()
		&& IsValid(Health) && !Health->IsDeadOrDying() && FMath::IsFinite(Health->GetHealth())
		&& FMath::IsFinite(Health->GetMaxHealth()) && Health->GetHealth() > 0 && Health->GetMaxHealth() > 0
		&& IsValid(Director) && Director->GetWorld() == World && !Director->IsActorBeingDestroyed()
		&& Director->GetRunState() == EDCStageRunState::Running;
	if (!bActive)
	{
		DisconnectMissionSource(); MissionDisplay->ClearMissionStatus(); ClearRadarBinding(); return;
	}
	ConnectMissionSource(Director);
	MissionDisplay->SetMissionSource(Director);
	const UDCMissionListData* Data = Director->GetCurrentMissionList();
	if (Data) { MissionDisplay->SetLiveMissionStatus(Director, Director->GetCurrentPhaseIndex(), Data, Mode->GetStageTitle(), Director->GetMissionProgress()); }
	else if (!MissionDisplay->IsMissionTransitionActive()) { MissionDisplay->ClearMissionStatus(); }
	UDCRadarWidget* Radar = FindOwningHUDRadar();
	if (ConnectedRadar.Get() != Radar) { ClearRadarBinding(); ConnectedRadar = Radar; }
	if (!Radar) { return; }
	USceneComponent* Target = Director->GetTrackedMissionTargetComponent();
	if (Target)
	{
		Radar->SetObjectiveComponent(Target);
		ConnectedObjective = Target; bOwnsRadarBinding = true;
	}
	else if (bOwnsRadarBinding)
	{
		ClearRadarBinding();
	}
}

void UDCStageOverlayWidget::ConnectMissionSource(ADCStageDirector* Director)
{
	if (MissionSource.Get() == Director) { return; }
	DisconnectMissionSource(); MissionSource = Director;
	MissionCompletedHandle = Director->OnMissionPhaseCompleted.AddUObject(this, &ThisClass::MissionPhaseCompleted);
	Director->OnStagePhaseStarted.AddUniqueDynamic(this, &ThisClass::MissionPhaseStarted);
	Director->OnStageEnded.AddUniqueDynamic(this, &ThisClass::MissionStageEnded);
	Director->OnDestroyed.AddUniqueDynamic(this, &ThisClass::MissionSourceDestroyed);
}

void UDCStageOverlayWidget::DisconnectMissionSource()
{
	if (ADCStageDirector* Director = MissionSource.Get())
	{
		Director->OnMissionPhaseCompleted.Remove(MissionCompletedHandle);
		Director->OnStagePhaseStarted.RemoveDynamic(this, &ThisClass::MissionPhaseStarted);
		Director->OnStageEnded.RemoveDynamic(this, &ThisClass::MissionStageEnded);
		Director->OnDestroyed.RemoveDynamic(this, &ThisClass::MissionSourceDestroyed);
	}
	MissionCompletedHandle.Reset(); MissionSource.Reset();
}

void UDCStageOverlayWidget::MissionPhaseCompleted(ADCStageDirector* Director, const FDCMissionTransitionSnapshot& Snapshot)
{
	if (Director != MissionSource.Get() || !MissionDisplay) { return; }
	const ADCStageRunGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADCStageRunGameMode>() : nullptr;
	APawn* Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	if (!Mode || Mode->GetStageDirector() != Director || Mode->HasRunEnded() || Mode->IsStageTravelPending()
		|| !Health || Health->IsDeadOrDying() || Health->GetHealth() <= 0) { return; }
	MissionDisplay->ShowCompletedPhase(Director, Snapshot);
}

void UDCStageOverlayWidget::MissionPhaseStarted(ADCStageDirector* Director, int32, const FText&, EDCStagePhaseType)
{
	if (Director == MissionSource.Get()) { Refresh(); }
}

void UDCStageOverlayWidget::MissionStageEnded(ADCStageDirector* Director, bool)
{
	if (Director == MissionSource.Get())
	{
		DisconnectMissionSource();
		if (MissionDisplay) { MissionDisplay->ClearMissionStatus(); }
		ClearRadarBinding();
	}
}

void UDCStageOverlayWidget::MissionSourceDestroyed(AActor* Director)
{
	if (Director == MissionSource.Get()) { MissionStageEnded(MissionSource.Get(), false); }
}

void UDCStageOverlayWidget::NativeDestruct()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(UpdateTimer); }
	DisconnectMissionSource();
	ClearRadarBinding();
	if (MissionDisplay) { MissionDisplay->ClearMissionStatus(); MissionDisplay->RequestLatestLive.Unbind(); }
	MissionWorld.Reset(); MissionMode.Reset(); MissionController.Reset(); MissionPawn.Reset();
	Boss.Reset();
	Super::NativeDestruct();
}
