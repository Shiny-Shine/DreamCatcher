#include "AI/Boss/DCBossPhaseComponent.h"
#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "AbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/DCStageRunGameMode.h"
#include "UI/Boss/DCBossHealthWidget.h"
#include "DreamCatcher.h"

UDCBossPhaseComponent::UDCBossPhaseComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UDCBossPhaseComponent::InitializeBoss()
{
	if (State != EDCBossRunState::Waiting) { return; }
	Boss = Cast<ADCBossCharacter>(GetOwner());
	ADCBossCharacter* B = Boss.Get();
	if (!B || !B->HasAuthority()) { return; }
	const UDCBossData* Data = B->GetBossData();
	UAbilitySystemComponent* ASC = B->GetAbilitySystemComponent();
	ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	if (!Data || !Data->IsValidConfiguration() || !B->IsGASReady() || !Mode || !ASC
		|| !B->GetLineAbilityClass() || !B->GetGroundAbilityClass())
	{
		FailSetup(TEXT("requires stage GameMode, boss data, initialized GAS and both pattern classes")); return;
	}
	int32 Lines = 0, Grounds = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		Lines += Spec.Ability && Spec.Ability->GetClass() == B->GetLineAbilityClass() ? 1 : 0;
		Grounds += Spec.Ability && Spec.Ability->GetClass() == B->GetGroundAbilityClass() ? 1 : 0;
	}
	if (Lines != 1 || Grounds != 1) { FailSetup(TEXT("requires exactly one granted ability of each selected pattern")); return; }
	UDCLyraHealthComponent* Health = B->GetHealthComponent();
	Health->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HealthChanged);
	Health->OnMaxHealthChanged.AddUniqueDynamic(this, &ThisClass::HealthChanged);
	Health->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::DeathStarted);
	Stage = Mode->GetStageDirector();
	if (Stage.IsValid()) { Stage->OnStageEnded.AddUniqueDynamic(this, &ThisClass::StageEnded); }
	Phase = 1;
	State = EDCBossRunState::Fighting;
	NextAttackAt = GetWorld()->GetTimeSeconds() + Data->OpeningDelay;
	GetWorld()->GetTimerManager().SetTimer(PollTimer, this, &ThisClass::UpdateCombat, .05f, true);
	HealthChanged(Health, Health->GetHealth(), Health->GetHealth(), nullptr);
}

void UDCBossPhaseComponent::FailSetup(const TCHAR* Reason)
{
	UE_LOG(LogDreamCatcher, Error, TEXT("[R12] Boss initialization failed: %s"), Reason);
	ShutdownBoss();
	if (ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>())
	{
		if (ADCStageDirector* Director = Mode->GetStageDirector()) { Director->FailStage(); }
	}
}

void UDCBossPhaseComponent::UpdateCombat()
{
	ADCBossCharacter* B = Boss.Get();
	if (State == EDCBossRunState::Stopped) { return; }
	if (!B || !B->CanRunBossCombat()) { ShutdownBoss(); return; }
	if (!HUD) { RefreshHUD(); }
	if (State != EDCBossRunState::Fighting) { return; }
	UAbilitySystemComponent* ASC = B->GetAbilitySystemComponent();
	const double Now = GetWorld()->GetTimeSeconds();
	if (ActivePattern.IsValid())
	{
		const FGameplayAbilitySpec* Active = ASC->FindAbilitySpecFromHandle(ActivePattern);
		if (Active && Active->IsActive()) { return; }
		ActivePattern = FGameplayAbilitySpecHandle();
		NextAttackAt = Now + (Phase == 1 ? B->GetBossData()->PhaseOneGap : B->GetBossData()->PhaseTwoGap);
		return;
	}
	if (Now < NextAttackAt) { return; }
	ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	APlayerController* PC = Mode ? Mode->GetStagePlayerController() : nullptr;
	APawn* Target = PC ? PC->GetPawn() : nullptr;
	if (!IsValid(Target)) { return; }
	const FRotator Facing = (Target->GetActorLocation() - B->GetActorLocation()).Rotation();
	B->SetActorRotation(FRotator(0, Facing.Yaw, 0));
	FGameplayAbilitySpec* Spec = bGroundNext ? ASC->FindAbilitySpecFromClass(B->GetGroundAbilityClass())
		: ASC->FindAbilitySpecFromClass(B->GetLineAbilityClass());
	if (!Spec) { FailSetup(TEXT("granted pattern was removed during combat")); return; }
	const FGameplayAbilitySpecHandle Handle = Spec->Handle;
	const bool bStarted = bGroundNext ? B->TryStartGroundStrike(Target) : B->TryStartRangedAttack(Target);
	if (State != EDCBossRunState::Fighting) { return; }
	if (bStarted) { ActivePattern = Handle; bGroundNext = !bGroundNext; }
	else { NextAttackAt = Now + .25; }
}

void UDCBossPhaseComponent::HealthChanged(UDCLyraHealthComponent*, float, float, AActor*)
{
	ADCBossCharacter* B = Boss.Get();
	if (!B || State == EDCBossRunState::Stopped) { return; }
	UDCLyraHealthComponent* Health = B->GetHealthComponent();
	if (Health->GetHealth() <= 0.f || Health->IsDeadOrDying()) { ShutdownBoss(); return; }
	if (Phase == 1 && Health->GetHealthNormalized() <= B->GetBossData()->PhaseTwoHealthRatio)
	{
		// Latch before cancelling GAs: damage/ability callbacks may be reentrant.
		Phase = 2;
		State = EDCBossRunState::Transitioning;
		CancelPatterns();
		if (State == EDCBossRunState::Stopped) { return; }
		bGroundNext = true;
		GetWorld()->GetTimerManager().SetTimer(TransitionTimer, this, &ThisClass::FinishTransition,
			B->GetBossData()->TransitionDuration, false);
	}
	RefreshHUD();
}

void UDCBossPhaseComponent::FinishTransition()
{
	if (State != EDCBossRunState::Transitioning) { return; }
	if (!Boss.IsValid() || !Boss->CanRunBossCombat()) { ShutdownBoss(); return; }
	State = EDCBossRunState::Fighting;
	NextAttackAt = GetWorld()->GetTimeSeconds() + Boss->GetBossData()->PhaseTwoGap;
	RefreshHUD();
}

void UDCBossPhaseComponent::CancelPatterns()
{
	ActivePattern = FGameplayAbilitySpecHandle();
	if (ADCBossCharacter* B = Boss.Get())
	{
		UAbilitySystemComponent* ASC = B->GetAbilitySystemComponent();
		TArray<FGameplayAbilitySpecHandle> Handles;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && (Spec.Ability->GetClass() == B->GetLineAbilityClass()
				|| Spec.Ability->GetClass() == B->GetGroundAbilityClass())) { Handles.Add(Spec.Handle); }
		}
		for (FGameplayAbilitySpecHandle Handle : Handles) { ASC->CancelAbilityHandle(Handle); }
	}
}

void UDCBossPhaseComponent::RefreshHUD()
{
	ADCBossCharacter* B = Boss.Get();
	if (!B || State == EDCBossRunState::Stopped || !B->GetBossData()) { return; }
	if (const ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>(); Mode && Mode->UsesExperienceUI()) { return; }
	if (!HUD)
	{
		ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
		APlayerController* PC = Mode ? Mode->GetStagePlayerController() : nullptr;
		if (PC && PC->GetLocalPlayer() && GetNetMode() != NM_DedicatedServer)
		{
			HUD = CreateWidget<UDCBossHealthWidget>(PC, UDCBossHealthWidget::StaticClass());
			if (HUD) { HUD->AddToViewport(15); }
		}
	}
	if (HUD)
	{
		HUD->SetBossStatus(B->GetBossData(), B->GetHealthComponent()->GetHealth(), B->GetHealthComponent()->GetMaxHealth());
	}
}

void UDCBossPhaseComponent::DeathStarted(AActor*) { ShutdownBoss(); }
void UDCBossPhaseComponent::StageEnded(ADCStageDirector*, bool) { ShutdownBoss(); }

void UDCBossPhaseComponent::ShutdownBoss()
{
	if (State == EDCBossRunState::Stopped) { return; }
	State = EDCBossRunState::Stopped;
	GetWorld()->GetTimerManager().ClearTimer(PollTimer);
	GetWorld()->GetTimerManager().ClearTimer(TransitionTimer);
	CancelPatterns(); // Preserve the original Death ability.
	if (Boss.IsValid())
	{
		UDCLyraHealthComponent* Health = Boss->GetHealthComponent();
		Health->OnHealthChanged.RemoveAll(this);
		Health->OnMaxHealthChanged.RemoveAll(this);
		Health->OnDeathStarted.RemoveAll(this);
	}
	if (Stage.IsValid()) { Stage->OnStageEnded.RemoveDynamic(this, &ThisClass::StageEnded); }
	Stage.Reset();
	if (HUD) { HUD->RemoveFromParent(); HUD = nullptr; }
}

void UDCBossPhaseComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ShutdownBoss();
	Super::EndPlay(Reason);
}
