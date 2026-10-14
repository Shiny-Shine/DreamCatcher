#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Engine/World.h"

ADCBossCharacter::ADCBossCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
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
