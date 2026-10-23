#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Engine/World.h"

ADCBossCharacter::ADCBossCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	BossPhase = CreateDefaultSubobject<UDCBossPhaseComponent>(TEXT("BossPhase"));
	AutoPossessAI = EAutoPossessAI::Disabled; // No unused bot controller/PlayerState/BT for this fixed-position boss.
	bAutoRangedAttack = false;
	TeamId = 2;
}

void ADCBossCharacter::PostInitializeComponents()
{
	bAutoRangedAttack = false; // The boss coordinator is the sole requester, including BP children.
	if (BossData && BossData->IsValidConfiguration()) { InitialHealth = BossData->MaxHealth; }
	Super::PostInitializeComponents();
}

void ADCBossCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (HasAuthority()) { BossPhase->InitializeBoss(); }
}

void ADCBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bAlwaysFacePlayer || !CanRunBossCombat() || BossPhase->GetBossRunState() == EDCBossRunState::Stopped) { return; }
	const ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	const APlayerController* PC = Mode->GetStagePlayerController();
	const APawn* Target = IsValid(PC) ? PC->GetPawn() : nullptr;
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Target);
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target->GetWorld() != GetWorld()
		|| !Health || Health->IsDeadOrDying() || Health->GetHealth() <= 0.f) { return; }
	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0;
	if (!Direction.ContainsNaN() && !Direction.IsNearlyZero())
	{
		// Abilities retain their original locked origin/aim/ground during windup.
		SetActorRotation(FRotator(0.0, Direction.Rotation().Yaw, 0.0));
	}
}

FVector ADCBossCharacter::GetRangedAttackOrigin() const
{
	return bUseCustomAttackOrigin && !AttackOriginOffset.ContainsNaN()
		? GetActorTransform().TransformPosition(AttackOriginOffset) : Super::GetRangedAttackOrigin();
}

bool ADCBossCharacter::CanRunBossCombat() const
{
	const ADCStageRunGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADCStageRunGameMode>() : nullptr;
	return HasAuthority() && IsGASReady() && !IsActorBeingDestroyed() && !IsDeadOrDying()
		&& GetHealthComponent()->GetHealth() > 0.f && Mode && !Mode->HasRunEnded();
}

bool ADCBossCharacter::TryStartGroundStrike(AActor* Target)
{
	if (!CanRunBossCombat() || !IsValid(Target) || Target->GetWorld() != GetWorld() || !GroundStrikeAbilityClass) { return false; }
	FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromClass(GroundStrikeAbilityClass);
	if (!Spec || Spec->IsActive()) { return false; }
	RequestedGroundTarget = Target;
	const bool bActivated = AbilitySystemComponent->TryActivateAbility(Spec->Handle, false);
	RequestedGroundTarget.Reset();
	return bActivated;
}

void ADCBossCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	BossPhase->ShutdownBoss();
	RequestedGroundTarget.Reset();
	Super::EndPlay(Reason);
}
