#include "AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h"

#include "AI/DCGASEnemyCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Teams/DCTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCGameplayAbility_EnemyRangedAttack)

UDCGameplayAbility_EnemyRangedAttack::UDCGameplayAbility_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	ActivationPolicy = EDCAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EDCAbilityActivationGroup::Independent;
	bRetriggerInstancedAbility = false;
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Type.Action")))));
	ActivationBlockedTags.AddTag(DCGameplayTags::Status_Death);
	ActivationBlockedTags.AddTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
}

bool UDCGameplayAbility_EnemyRangedAttack::HasValidConfiguration() const
{
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || !FMath::IsFinite(AttackRange) || AttackRange < 1.0f
		|| !FMath::IsFinite(WindupTime) || WindupTime < 0.01f || !FMath::IsFinite(RecoveryTime) || RecoveryTime < 0.0f)
	{
		return false;
	}
	const UGameplayEffect* Effect = DamageEffectClass.GetDefaultObject();
	if (!Effect || Effect->DurationPolicy != EGameplayEffectDurationType::Instant || !Effect->Modifiers.IsEmpty()
		|| Effect->Executions.Num() != 1 || Effect->Executions[0].CalculationClass != UDCLyraDamageExecution::StaticClass()
		|| Effect->Executions[0].CalculationModifiers.Num() != 1) { return false; }
	const FGameplayEffectExecutionScopedModifierInfo& Modifier = Effect->Executions[0].CalculationModifiers[0];
	return Modifier.AggregatorType == EGameplayEffectScopedModifierAggregatorType::CapturedAttributeBacked
		&& Modifier.CapturedAttribute == FGameplayEffectAttributeCaptureDefinition(UDCCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
		&& Modifier.ModifierOp == EGameplayModOp::Additive
		&& Modifier.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller
		&& Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag == DCGameplayTags::SetByCaller_Damage;
}

FVector UDCGameplayAbility_EnemyRangedAttack::GetFireOrigin(const ADCGASEnemyCharacter* Enemy)
{
	const USkeletalMeshComponent* Mesh = Enemy->GetMesh();
	return Mesh && Mesh->DoesSocketExist(TEXT("Muzzle")) ? Mesh->GetSocketLocation(TEXT("Muzzle"))
		: Enemy->GetActorLocation() + FVector(0.0, 0.0, 60.0);
}

FVector UDCGameplayAbility_EnemyRangedAttack::GetAimPoint(const AActor* AttackTarget)
{
	return AttackTarget->GetActorLocation() + FVector(0.0, 0.0, 50.0);
}

bool UDCGameplayAbility_EnemyRangedAttack::IsSourceReady(const ADCGASEnemyCharacter* Enemy) const
{
	const UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	return ASC && Enemy->HasAuthority() && Enemy->IsGASReady() && !Enemy->IsActorBeingDestroyed()
		&& !Enemy->IsDeadOrDying() && Enemy->GetHealthComponent()->GetHealth() > 0.0f
		&& ASC->GetAvatarActor() == Enemy && ASC->GetOwnerActor() == Enemy
		&& !ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death)
		&& !ASC->HasMatchingGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
}

bool UDCGameplayAbility_EnemyRangedAttack::IsTargetReady(const ADCGASEnemyCharacter* Enemy, AActor* AttackTarget) const
{
	if (!IsSourceReady(Enemy) || !IsValid(AttackTarget) || AttackTarget == Enemy || AttackTarget->IsActorBeingDestroyed()
		|| AttackTarget->GetWorld() != Enemy->GetWorld()) { return false; }
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(AttackTarget);
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AttackTarget);
	const UDCTeamSubsystem* Teams = Enemy->GetWorld()->GetSubsystem<UDCTeamSubsystem>();
	return Health && Health->GetHealth() > 0.0f && !Health->IsDeadOrDying() && ASC
		&& !ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death)
		&& Teams && Teams->CanCauseDamage(Enemy, AttackTarget, false);
}

bool UDCGameplayAbility_EnemyRangedAttack::HasLineOfSight(const ADCGASEnemyCharacter* Enemy, AActor* AttackTarget, const FVector& Origin) const
{
	const FVector CurrentAim = GetAimPoint(AttackTarget);
	if (FVector::DistSquared(Origin, CurrentAim) > FMath::Square(AttackRange)) { return false; }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCEnemyAttackSight), false, Enemy);
	FHitResult Hit;
	const bool bBlocked = Enemy->GetWorld()->LineTraceSingleByChannel(Hit, Origin, CurrentAim, ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == AttackTarget;
}

bool UDCGameplayAbility_EnemyRangedAttack::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const ADCGASEnemyCharacter* Enemy = ActorInfo ? Cast<ADCGASEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	AActor* AttackTarget = Enemy ? Enemy->GetRequestedRangedAttackTarget() : nullptr;
	return HasValidConfiguration() && IsTargetReady(Enemy, AttackTarget)
		&& HasLineOfSight(Enemy, AttackTarget, GetFireOrigin(Enemy))
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UDCGameplayAbility_EnemyRangedAttack::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const uint32 Serial = ++ActivationSerial;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive() || ActivationSerial != Serial) { return; }
	ADCGASEnemyCharacter* Enemy = Cast<ADCGASEnemyCharacter>(ActorInfo->AvatarActor.Get());
	AActor* AttackTarget = Enemy ? Enemy->GetRequestedRangedAttackTarget() : nullptr;
	if (!HasValidConfiguration() || !IsTargetReady(Enemy, AttackTarget)
		|| !HasLineOfSight(Enemy, AttackTarget, GetFireOrigin(Enemy)) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!IsActive() || ActivationSerial != Serial) { return; }
	if (!IsTargetReady(Enemy, AttackTarget))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SourceEnemy = Enemy;
	LockedTarget = AttackTarget;
	TargetHealth = UDCLyraHealthComponent::FindHealthComponent(AttackTarget);
	AttackWorld = Enemy->GetWorld();
	LockedOrigin = GetFireOrigin(Enemy);
	LockedAimPoint = GetAimPoint(AttackTarget);
	Phase = EDCEnemyAttackPhase::Windup;
	Enemy->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleActorEndPlay);
	AttackTarget->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleActorEndPlay);
	TargetHealth->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleTargetDeath);
	AttackWorld->GetTimerManager().SetTimer(ContextTimer, this, &ThisClass::CheckContext, 0.05f, true);
	bIntentEmitted = true;
	BP_OnAttackIntent(LockedOrigin, LockedAimPoint, WindupTime);
	if (!IsActive() || ActivationSerial != Serial || Phase != EDCEnemyAttackPhase::Windup) { return; }
	WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, WindupTime);
	WindupTask->OnFinish.AddDynamic(this, &ThisClass::FinishWindup);
	WindupTask->ReadyForActivation();
}

void UDCGameplayAbility_EnemyRangedAttack::CheckContext()
{
	if (!IsActive() || bEndingAttack) { return; }
	ADCGASEnemyCharacter* Enemy = SourceEnemy.Get();
	if (!IsSourceReady(Enemy) || !CurrentActorInfo || CurrentActorInfo->AvatarActor.Get() != Enemy
		|| (Phase == EDCEnemyAttackPhase::Windup && (!IsTargetReady(Enemy, LockedTarget.Get())
			|| !HasLineOfSight(Enemy, LockedTarget.Get(), LockedOrigin)))) { AbortAttack(); }
}

void UDCGameplayAbility_EnemyRangedAttack::FinishWindup()
{
	if (!IsActive() || bEndingAttack || Phase != EDCEnemyAttackPhase::Windup) { return; }
	const uint32 Serial = ActivationSerial;
	CheckContext();
	if (!IsActive() || ActivationSerial != Serial || Phase != EDCEnemyAttackPhase::Windup) { return; }
	// Transition before damage/cosmetic callbacks: a lethal hit must still incur recovery.
	Phase = EDCEnemyAttackPhase::Recovery;
	ADCGASEnemyCharacter* Enemy = SourceEnemy.Get();
	const FVector TraceEnd = LockedOrigin + (LockedAimPoint - LockedOrigin).GetSafeNormal() * AttackRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCEnemyAttackShot), false, Enemy);
	FHitResult Hit;
	const bool bHit = Enemy->GetWorld()->LineTraceSingleByChannel(Hit, LockedOrigin, TraceEnd, ECC_Visibility, Params);
	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	// A clear ray is a miss, not guaranteed damage to the selected target.
	if (HitActor && HitActor == LockedTarget.Get() && IsTargetReady(Enemy, HitActor))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (!Spec.IsValid()) { AbortAttack(); return; }
		Spec.Data->SetSetByCallerMagnitude(DCGameplayTags::SetByCaller_Damage, DamageAmount);
		FGameplayEffectContextHandle Context = Spec.Data->GetContext();
		Context.AddOrigin(LockedOrigin);
		Context.AddHitResult(Hit, true);
		Enemy->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(),
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor));
	}
	if (!IsActive() || ActivationSerial != Serial || Phase != EDCEnemyAttackPhase::Recovery) { return; }
	BP_OnAttackShot(LockedOrigin, bHit ? Hit.ImpactPoint : TraceEnd, HitActor);
	if (!IsActive() || ActivationSerial != Serial || Phase != EDCEnemyAttackPhase::Recovery) { return; }
	RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoveryTime);
	RecoveryTask->OnFinish.AddDynamic(this, &ThisClass::FinishRecovery);
	RecoveryTask->ReadyForActivation();
}

void UDCGameplayAbility_EnemyRangedAttack::FinishRecovery()
{
	if (IsActive() && Phase == EDCEnemyAttackPhase::Recovery)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UDCGameplayAbility_EnemyRangedAttack::AbortAttack()
{
	// Invalid source/target must not fire even if cosmetic BP code changes cancelability.
	if (IsActive() && !bEndingAttack) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
}

void UDCGameplayAbility_EnemyRangedAttack::HandleActorEndPlay(AActor* EndedActor, EEndPlayReason::Type)
{
	if (EndedActor == SourceEnemy.Get() || Phase == EDCEnemyAttackPhase::Windup) { AbortAttack(); }
}

void UDCGameplayAbility_EnemyRangedAttack::HandleTargetDeath(AActor*)
{
	if (Phase == EDCEnemyAttackPhase::Windup) { AbortAttack(); }
}

void UDCGameplayAbility_EnemyRangedAttack::ReleaseWaitTask(TObjectPtr<UAbilityTask_WaitDelay>& Task)
{
	if (Task)
	{
		Task->OnFinish.RemoveAll(this);
		// Engine WaitDelay uses a private dummy timer handle. Only clear our own task's timers.
		if (UWorld* World = AttackWorld.Get()) { World->GetTimerManager().ClearAllTimersForObject(Task.Get()); }
		Task->EndTask();
		Task = nullptr;
	}
}

void UDCGameplayAbility_EnemyRangedAttack::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEndingAttack || !IsEndAbilityValid(Handle, ActorInfo)) { return; }
	bEndingAttack = true;
	if (UWorld* World = AttackWorld.Get()) { World->GetTimerManager().ClearTimer(ContextTimer); }
	ReleaseWaitTask(WindupTask);
	ReleaseWaitTask(RecoveryTask);
	if (ADCGASEnemyCharacter* Enemy = SourceEnemy.Get()) { Enemy->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleActorEndPlay); }
	if (AActor* AttackTarget = LockedTarget.Get()) { AttackTarget->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleActorEndPlay); }
	if (UDCLyraHealthComponent* Health = TargetHealth.Get()) { Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleTargetDeath); }
	SourceEnemy.Reset();
	LockedTarget.Reset();
	TargetHealth.Reset();
	AttackWorld.Reset();
	Phase = EDCEnemyAttackPhase::Idle;
	if (bIntentEmitted)
	{
		bIntentEmitted = false;
		BP_OnAttackEnded(bWasCancelled);
	}
	bEndingAttack = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UDCGameplayAbility_EnemyRangedAttack::BP_OnAttackIntent_Implementation(FVector, FVector, float) {}
void UDCGameplayAbility_EnemyRangedAttack::BP_OnAttackShot_Implementation(FVector, FVector, AActor*) {}
void UDCGameplayAbility_EnemyRangedAttack::BP_OnAttackEnded_Implementation(bool) {}
