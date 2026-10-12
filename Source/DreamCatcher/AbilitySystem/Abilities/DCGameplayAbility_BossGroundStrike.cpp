#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "AI/Boss/DCBossTelegraph.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Executions/DCLyraDamageExecution.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffect.h"
#include "Teams/DCTeamSubsystem.h"

UDCGameplayAbility_BossGroundStrike::UDCGameplayAbility_BossGroundStrike(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
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

bool UDCGameplayAbility_BossGroundStrike::ValidConfiguration() const
{
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f || !FMath::IsFinite(Radius) || Radius < 1.f
		|| !FMath::IsFinite(AttackRange) || AttackRange < 1.f || !FMath::IsFinite(VerticalTolerance) || VerticalTolerance < 1.f
		|| !FMath::IsFinite(WindupTime) || WindupTime < .1f || !FMath::IsFinite(RecoveryTime) || RecoveryTime < .01f) { return false; }
	// Same original DamageExecution + additive SetByCaller contract as the accepted R10 shot.
	const UGameplayEffect* GE = DamageEffectClass.GetDefaultObject();
	if (!GE || GE->DurationPolicy != EGameplayEffectDurationType::Instant || !GE->Modifiers.IsEmpty()
		|| GE->Executions.Num() != 1 || GE->Executions[0].CalculationClass != UDCLyraDamageExecution::StaticClass()
		|| GE->Executions[0].CalculationModifiers.Num() != 1) { return false; }
	const auto& M = GE->Executions[0].CalculationModifiers[0];
	return M.AggregatorType == EGameplayEffectScopedModifierAggregatorType::CapturedAttributeBacked
		&& M.CapturedAttribute == FGameplayEffectAttributeCaptureDefinition(UDCCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
		&& M.ModifierOp == EGameplayModOp::Additive
		&& M.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller
		&& M.ModifierMagnitude.GetSetByCallerFloat().DataTag == DCGameplayTags::SetByCaller_Damage;
}

bool UDCGameplayAbility_BossGroundStrike::ValidContext(const ADCBossCharacter* Source, AActor* Target) const
{
	if (!IsValid(Source) || !Source->CanRunBossCombat() || !Source->GetBossData()
		|| Source->GetBossPhaseComponent()->GetBossRunState() != EDCBossRunState::Fighting
		|| !IsValid(Target) || Target == Source || Target->IsActorBeingDestroyed() || Target->GetWorld() != Source->GetWorld()) { return false; }
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Target);
	const UDCTeamSubsystem* Teams = Source->GetWorld()->GetSubsystem<UDCTeamSubsystem>();
	const UAbilitySystemComponent* ASC = Source->GetAbilitySystemComponent();
	return Health && Health->GetHealth() > 0.f && !Health->IsDeadOrDying()
		&& UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) && Teams && Teams->CanCauseDamage(Source, Target, false)
		&& ASC && !ASC->HasMatchingGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
}

bool UDCGameplayAbility_BossGroundStrike::FindGround(const ADCBossCharacter* Source, AActor* Target, FVector& OutGround) const
{
	if (!ValidContext(Source, Target) || FVector::DistSquared(Source->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(AttackRange)) { return false; }
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCBossGroundLock), false, Source);
	Params.AddIgnoredActor(Target);
	const FVector Start = Target->GetActorLocation();
	if (!Source->GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 2200), ECC_Visibility, Params)
		|| Hit.bStartPenetrating || Hit.ImpactNormal.Z < .7f) { return false; }
	OutGround = Hit.ImpactPoint;
	return true;
}

bool UDCGameplayAbility_BossGroundStrike::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* RelevantTags) const
{
	const ADCBossCharacter* Source = ActorInfo ? Cast<ADCBossCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const FGameplayAbilitySpec* Other = Source ? Source->GetAbilitySystemComponent()->FindAbilitySpecFromClass(Source->GetLineAbilityClass()) : nullptr;
	FVector Ground;
	return ValidConfiguration() && Source && (!Other || !Other->IsActive()) && FindGround(Source, Source->GetRequestedGroundTarget(), Ground)
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, RelevantTags);
}

void UDCGameplayAbility_BossGroundStrike::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
	const uint32 ThisSerial = ++Serial;
	bEnding = false;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, Event);
	if (!IsActive() || Serial != ThisSerial) { return; }
	Boss = Cast<ADCBossCharacter>(ActorInfo->AvatarActor.Get());
	LockedTarget = Boss.IsValid() ? Boss->GetRequestedGroundTarget() : nullptr;
	if (!Boss.IsValid() || !FindGround(Boss.Get(), LockedTarget.Get(), LockedGround) || !CommitAbility(Handle, ActorInfo, ActivationInfo)) { Abort(); return; }
	if (!IsActive() || Serial != ThisSerial) { return; }
	AttackWorld = Boss->GetWorld();
	Phase = EDCEnemyAttackPhase::Windup;
	Warning = ADCBossTelegraph::Circle(Boss.Get(), LockedGround, Radius, Boss->GetBossData()->WarningMaterial, WindupTime + .5f);
	AttackWorld->GetTimerManager().SetTimer(ContextTimer, this, &ThisClass::CheckContext, .05f, true);
	WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, WindupTime);
	WindupTask->OnFinish.AddDynamic(this, &ThisClass::Detonate);
	WindupTask->ReadyForActivation();
}

void UDCGameplayAbility_BossGroundStrike::CheckContext()
{
	if (!IsActive() || bEnding) { return; }
	if (!Boss.IsValid() || !Boss->CanRunBossCombat()
		|| Boss->GetBossPhaseComponent()->GetBossRunState() != EDCBossRunState::Fighting
		|| !CurrentActorInfo || CurrentActorInfo->AvatarActor.Get() != Boss.Get()
		|| (Phase == EDCEnemyAttackPhase::Windup && !ValidContext(Boss.Get(), LockedTarget.Get()))) { Abort(); }
}

bool UDCGameplayAbility_BossGroundStrike::IsInsideStrike(AActor* Target) const
{
	if (!ValidContext(Boss.Get(), Target)) { return false; }
	FVector Feet = Target->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Target)) { Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); }
	// Actor center in XY; feet in Z. The circle is never widened by the capsule radius.
	return FVector::DistSquared2D(Feet, LockedGround) <= FMath::Square(Radius)
		&& FMath::Abs(Feet.Z - LockedGround.Z) <= VerticalTolerance;
}

bool UDCGameplayAbility_BossGroundStrike::HasBlastLineOfSight(AActor* Target, FHitResult& Hit) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCBossBlastOcclusion), false, Boss.Get());
	const bool bBlocked = AttackWorld->LineTraceSingleByChannel(Hit, LockedGround + FVector(0, 0, 20),
		Target->GetActorLocation(), ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == Target;
}

void UDCGameplayAbility_BossGroundStrike::Detonate()
{
	if (!IsActive() || bEnding || Phase != EDCEnemyAttackPhase::Windup) { return; }
	const uint32 ThisSerial = Serial;
	CheckContext();
	if (!IsActive() || Serial != ThisSerial || Phase != EDCEnemyAttackPhase::Windup) { return; }
	Phase = EDCEnemyAttackPhase::Recovery; // Reentrant damage callbacks cannot detonate twice.
	if (Warning.IsValid()) { Warning->Destroy(); }
	Warning.Reset();
	ADCBossTelegraph::Circle(Boss.Get(), LockedGround, Radius, Boss->GetBossData()->ImpactMaterial, .2f);
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCBossBlast), false, Boss.Get());
	// A box broad phase includes capsule centers above the warning plane. Narrow phase below is exact.
	AttackWorld->OverlapMultiByObjectType(Overlaps, LockedGround + FVector(0, 0, VerticalTolerance), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(FVector(Radius, Radius, VerticalTolerance + 200.f)), Params);
	TSet<TWeakObjectPtr<AActor>> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Seen.Contains(Target)) { continue; }
		Seen.Add(Target);
		FHitResult Hit;
		if (!IsInsideStrike(Target) || !HasBlastLineOfSight(Target, Hit)) { continue; }
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (!Spec.IsValid()) { Abort(); return; }
		Spec.Data->SetSetByCallerMagnitude(DCGameplayTags::SetByCaller_Damage, DamageAmount);
		FGameplayEffectContextHandle Context = Spec.Data->GetContext();
		Context.AddOrigin(LockedGround);
		if (Hit.GetActor() == Target) { Context.AddHitResult(Hit, true); }
		Boss->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(),
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target));
		if (!IsActive() || Serial != ThisSerial || !Boss.IsValid() || !Boss->CanRunBossCombat()) { return; }
	}
	RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoveryTime);
	RecoveryTask->OnFinish.AddDynamic(this, &ThisClass::FinishRecovery);
	RecoveryTask->ReadyForActivation();
}

void UDCGameplayAbility_BossGroundStrike::FinishRecovery()
{
	if (IsActive() && Phase == EDCEnemyAttackPhase::Recovery) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
}

void UDCGameplayAbility_BossGroundStrike::Abort()
{
	if (IsActive() && !bEnding) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
}

void UDCGameplayAbility_BossGroundStrike::ReleaseTask(TObjectPtr<UAbilityTask_WaitDelay>& Task)
{
	if (!Task) { return; }
	Task->OnFinish.RemoveAll(this);
	if (AttackWorld.IsValid()) { AttackWorld->GetTimerManager().ClearAllTimersForObject(Task.Get()); }
	Task->EndTask();
	Task = nullptr;
}

void UDCGameplayAbility_BossGroundStrike::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEnding || !IsEndAbilityValid(Handle, ActorInfo)) { return; }
	bEnding = true;
	if (AttackWorld.IsValid()) { AttackWorld->GetTimerManager().ClearTimer(ContextTimer); }
	ReleaseTask(WindupTask);
	ReleaseTask(RecoveryTask);
	if (Warning.IsValid()) { Warning->Destroy(); }
	Warning.Reset();
	Boss.Reset(); LockedTarget.Reset(); AttackWorld.Reset();
	Phase = EDCEnemyAttackPhase::Idle;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}
