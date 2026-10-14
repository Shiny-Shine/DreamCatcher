#include "AI/DCGASEnemyCharacter.h"

#include "AIController.h"
#include "AbilitySystem/Abilities/DCLyraGameplayAbility_Death.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCAbilityTagRelationshipMapping.h"
#include "BrainComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DreamCatcher.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Physics/DCLyraCollisionChannels.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCGASEnemyCharacter)

ADCGASEnemyCharacter::ADCGASEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UDCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	HealthSet = CreateDefaultSubobject<UDCHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UDCCombatSet>(TEXT("CombatSet"));
	HealthComponent = CreateDefaultSubobject<UDCLyraHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::HandleHealthChanged);
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::HandleDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::HandleDeathFinished);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	// Reuse the existing weapon channels without changing project collision config.
	GetCapsuleComponent()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Capsule, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Multi, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Capsule, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Multi, ECR_Block);
}

UAbilitySystemComponent* ADCGASEnemyCharacter::GetAbilitySystemComponent() const { return AbilitySystemComponent; }
FGenericTeamId ADCGASEnemyCharacter::GetGenericTeamId() const { return FGenericTeamId(TeamId); }
FOnDCTeamIndexChangedDelegate* ADCGASEnemyCharacter::GetOnTeamIndexChangedDelegate() { return &OnTeamChanged; }
bool ADCGASEnemyCharacter::IsDeadOrDying() const { return bDeathReported || HealthComponent->IsDeadOrDying(); }

void ADCGASEnemyCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (!HasAuthority() || NewTeamID == FGenericTeamId::NoTeam) { return; }
	const FGenericTeamId OldTeam = GetGenericTeamId();
	TeamId = NewTeamID.GetId();
	ConditionalBroadcastTeamChanged(this, OldTeam, GetGenericTeamId());
	ForceNetUpdate();
}

void ADCGASEnemyCharacter::OnRep_TeamId(uint8 OldTeamId)
{
	ConditionalBroadcastTeamChanged(this, FGenericTeamId(OldTeamId), GetGenericTeamId());
}

void ADCGASEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADCGASEnemyCharacter, TeamId);
}

void ADCGASEnemyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Components/attributes are registered; initialize before Blueprint BeginPlay.
	if (!GetWorld() || !GetWorld()->IsGameWorld()) { return; }
	if (bInitializationAttempted || bEndingPlay) { return; }
	bInitializationAttempted = true;
	if (!DeathAbilityClass || DeathAbilityClass->HasAnyClassFlags(CLASS_Abstract)
		|| !FMath::IsFinite(InitialHealth) || InitialHealth < 1.0f || GetGenericTeamId() == FGenericTeamId::NoTeam)
	{
		FailInitialization(TEXT("requires a concrete Lyra-derived Death ability, positive health, and a valid team"));
		return;
	}
	for (const UDCAbilitySet* Set : AbilitySets)
	{
		if (!IsValid(Set)) { FailInitialization(TEXT("contains a null AbilitySet")); return; }
	}
	// Unlike the player/bot PlayerState, this ASC has exactly the NPC's lifetime.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->SetTagRelationshipMapping(TagRelationshipMapping);
	if (HasAuthority())
	{
		AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetMaxHealthAttribute(), InitialHealth);
		GrantedDeathAbility = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, INDEX_NONE, this));
		if (!GrantedDeathAbility.IsValid()) { FailInitialization(TEXT("could not grant Death ability")); return; }
	}
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	bHealthInitialized = true;
	if (HasAuthority())
	{
		for (const UDCAbilitySet* Set : AbilitySets)
		{
			Set->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles, this);
			if (bEndingPlay || IsActorBeingDestroyed()) { return; }
		}
		int32 HealthSets = 0, CombatSets = 0, DeathAbilities = 0;
		for (const UAttributeSet* Set : AbilitySystemComponent->GetSpawnedAttributes())
		{
			HealthSets += IsValid(Set) && Set->IsA<UDCHealthSet>() ? 1 : 0;
			CombatSets += IsValid(Set) && Set->IsA<UDCCombatSet>() ? 1 : 0;
		}
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			DeathAbilities += Spec.Ability && Spec.Ability->IsA<UDCLyraGameplayAbility_Death>() ? 1 : 0;
		}
		if (HealthSets != 1 || CombatSets != 1 || DeathAbilities != 1)
		{
			FailInitialization(TEXT("requires exactly one HealthSet, CombatSet, and Death ability"));
			return;
		}
		if (bAutoRangedAttack || RangedAttackAbilityClass)
		{
			int32 AttackAbilities = 0;
			for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
			{
				AttackAbilities += Spec.Ability && Spec.Ability->IsA<UDCGameplayAbility_EnemyRangedAttack>() ? 1 : 0;
			}
			if (!RangedAttackAbilityClass || RangedAttackAbilityClass->HasAnyClassFlags(CLASS_Abstract)
				|| AttackAbilities != 1 || !AbilitySystemComponent->FindAbilitySpecFromClass(RangedAttackAbilityClass))
			{
				FailInitialization(TEXT("ranged attack requires exactly one selected concrete ability granted by AbilitySets"));
				return;
			}
		}
	}
	bGASReady = true;
	UE_LOG(LogDreamCatcher, Log, TEXT("[R10-1] GAS enemy ready: %s Health=%.1f Team=%d"),
		*GetName(), HealthComponent->GetHealth(), TeamId);
}

void ADCGASEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && bAutoRangedAttack && IsGASReady() && !IsDeadOrDying())
	{
		// Preserve the existing single-player target policy. No movement/BT is introduced.
		GetWorldTimerManager().SetTimer(AttackRequestTimer, this, &ThisClass::UpdateRangedAttack, 0.25f, true);
	}
}

void ADCGASEnemyCharacter::UpdateRangedAttack()
{
	if (IsGASReady() && !IsDeadOrDying()) { TryStartRangedAttack(UGameplayStatics::GetPlayerPawn(this, 0)); }
}

bool ADCGASEnemyCharacter::TryStartRangedAttack(AActor* AttackTarget)
{
	if (!HasAuthority() || !IsGASReady() || IsDeadOrDying() || !RangedAttackAbilityClass
		|| !IsValid(AttackTarget) || AttackTarget == this || AttackTarget->GetWorld() != GetWorld()) { return false; }
	FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromClass(RangedAttackAbilityClass);
	if (!Spec || Spec->IsActive()) { return false; }
	RequestedAttackTarget = AttackTarget;
	const bool bActivated = AbilitySystemComponent->TryActivateAbility(Spec->Handle, false);
	RequestedAttackTarget.Reset();
	return bActivated;
}

void ADCGASEnemyCharacter::StopMovementAndCollision()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		if (UBrainComponent* Brain = AI->GetBrainComponent()) { Brain->StopLogic(TEXT("Enemy stopped")); }
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
}

void ADCGASEnemyCharacter::HandleHealthChanged(UDCLyraHealthComponent*, float OldValue, float NewValue, AActor*)
{
	if (bGASReady && !bEndingPlay && NewValue < OldValue) { BP_OnDamaged(HealthComponent->GetHealthNormalized()); }
}

void ADCGASEnemyCharacter::HandleDeathStarted(AActor* OwningActor)
{
	if (OwningActor != this || bDeathReported || bEndingPlay) { return; }
	bDeathReported = true;
	GetWorldTimerManager().ClearTimer(AttackRequestTimer);
	StopMovementAndCollision();
	// Notify the spawner before cosmetic BP code can destroy the actor.
	if (HasAuthority()) { OnEnemyDeath.Broadcast(this); }
	if (!bEndingPlay && !IsActorBeingDestroyed()) { BP_OnDeath(); }
}

void ADCGASEnemyCharacter::HandleDeathFinished(AActor* OwningActor)
{
	if (OwningActor != this || bEndingPlay || bDeathFinishScheduled || !HasAuthority()) { return; }
	bDeathFinishScheduled = true;
	// Do not destroy the ASC in the middle of the original Death ability's EndAbility.
	DeathCleanupTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyAfterDeath);
}

void ADCGASEnemyCharacter::DestroyAfterDeath()
{
	if (!bEndingPlay && HasAuthority()) { Destroy(); }
}

void ADCGASEnemyCharacter::FailInitialization(const TCHAR* Reason)
{
	UE_LOG(LogDreamCatcher, Error, TEXT("[R10-1] GAS enemy initialization failed: %s (%s)"), *GetName(), Reason);
	UninitializeGAS();
	StopMovementAndCollision();
}

void ADCGASEnemyCharacter::UninitializeGAS()
{
	bGASReady = false;
	GetWorldTimerManager().ClearTimer(AttackRequestTimer);
	RequestedAttackTarget.Reset();
	HealthComponent->OnHealthChanged.RemoveAll(this);
	HealthComponent->OnDeathStarted.RemoveAll(this);
	HealthComponent->OnDeathFinished.RemoveAll(this);
	if (bHealthInitialized)
	{
		HealthComponent->UninitializeFromAbilitySystem();
		bHealthInitialized = false;
	}
	if (HasAuthority())
	{
		GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
		if (GrantedDeathAbility.IsValid()) { AbilitySystemComponent->ClearAbility(GrantedDeathAbility); }
		GrantedDeathAbility = FGameplayAbilitySpecHandle();
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->ClearAllAbilities();
	}
	AbilitySystemComponent->ClearAbilityInput();
	AbilitySystemComponent->RemoveAllGameplayCues();
	AbilitySystemComponent->ClearActorInfo();
}

void ADCGASEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(DeathCleanupTimer);
	UninitializeGAS();
	Super::EndPlay(EndPlayReason);
}
