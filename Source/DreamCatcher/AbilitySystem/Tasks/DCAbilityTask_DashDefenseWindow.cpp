#include "DCAbilityTask_DashDefenseWindow.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "DCLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAbilityTask_DashDefenseWindow)

UDCAbilityTask_DashDefenseWindow::UDCAbilityTask_DashDefenseWindow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// A short-lived safety watch for direct ASC/Avatar replacement without an ability end.
	bTickingTask = true;
}

UDCAbilityTask_DashDefenseWindow* UDCAbilityTask_DashDefenseWindow::DashDefenseWindow(
	UGameplayAbility* OwningAbility, bool bCommitSucceeded, TSubclassOf<UGameplayEffect> DefenseEffect, float Duration)
{
	if (!OwningAbility)
	{
		return nullptr;
	}
	UDCAbilityTask_DashDefenseWindow* Task = NewAbilityTask<UDCAbilityTask_DashDefenseWindow>(OwningAbility);
	Task->bCommitted = bCommitSucceeded;
	Task->DefenseEffectClass = DefenseEffect;
	Task->WindowDuration = Duration;
	return Task;
}

bool UDCAbilityTask_DashDefenseWindow::HasValidContext() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	const UDCHealthSet* Health = BoundHealthSet.Get();
	return ASC && ASC->IsOwnerActorAuthoritative() && Ability && Ability->IsActive()
		&& InitialOwner.IsValid() && InitialAvatar.IsValid()
		&& InitialOwner->HasAuthority() && InitialAvatar->HasAuthority()
		&& ASC->GetOwnerActor() == InitialOwner.Get() && ASC->GetAvatarActor_Direct() == InitialAvatar.Get()
		&& Health && Health->GetHealth() > 0.0f
		&& !ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death);
}

bool UDCAbilityTask_DashDefenseWindow::IsDefenseWindowActive() const
{
	if (!bOpened || bClosing || IsFinished() || !HasValidContext())
	{
		return false;
	}
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(DefenseHandle);
	const UWorld* World = TimerWorld.Get();
	return Effect && !Effect->bIsInhibited && World && Effect->GetTimeRemaining(World->GetTimeSeconds()) > 0.0f;
}

void UDCAbilityTask_DashDefenseWindow::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!bCommitted || !Ability || !Ability->IsActive() || !ASC || !ASC->IsOwnerActorAuthoritative()
		|| !DefenseEffectClass || !FMath::IsFinite(WindowDuration) || WindowDuration <= 0.0f)
	{
		EndTask();
		return;
	}
	InitialOwner = ASC->GetOwnerActor();
	InitialAvatar = ASC->GetAvatarActor_Direct();
	BoundHealthSet = ASC->GetSet<UDCHealthSet>();
	TimerWorld = ASC->GetWorld();
	if (!HasValidContext() || !TimerWorld.IsValid())
	{
		EndTask();
		return;
	}

	FGameplayEffectSpecHandle Spec = Ability->MakeOutgoingGameplayEffectSpec(DefenseEffectClass, Ability->GetAbilityLevel());
	FGameplayTagContainer GrantedTags;
	if (Spec.IsValid())
	{
		Spec.Data->GetAllGrantedTags(GrantedTags);
	}
	const UGameplayEffect* Definition = Spec.IsValid() ? Spec.Data->Def.Get() : nullptr;
	if (!Definition || Definition->DurationPolicy != EGameplayEffectDurationType::HasDuration
		|| Definition->GetStackingType() != EGameplayEffectStackingType::None || Spec.Data->GetPeriod() != 0.0f
		|| !Definition->Modifiers.IsEmpty() || !Definition->Executions.IsEmpty()
		|| GrantedTags.Num() != 1 || !GrantedTags.HasTagExact(DCGameplayTags::Gameplay_DamageImmunity))
	{
		UE_LOG(LogDCAbilitySystem, Warning, TEXT("DashDefense refused: expected a tag-only, non-stacking, finite immunity GE."));
		EndTask();
		return;
	}

	// Do not restart/extend an overlapping window from a duplicate graph execution.
	for (FActiveGameplayEffectHandle Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const FActiveGameplayEffect* Existing = ASC->GetActiveGameplayEffect(Handle);
		if (Existing && Existing->Spec.Def.Get() == Definition && Existing->Spec.GetContext().GetSourceObject() == Ability.Get())
		{
			EndTask();
			return;
		}
	}
	Spec.Data->SetDuration(WindowDuration, true);
	Spec.Data->GetContext().AddSourceObject(Ability.Get());
	AbilityEndedBinding = Ability->OnGameplayAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	InitialAvatar->OnEndPlay.AddDynamic(this, &ThisClass::HandleAvatarEndPlay);
	OutOfHealthBinding = BoundHealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	const FActiveGameplayEffectHandle Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	// Applying a GE can run external delegates which end the ability or destroy its avatar.
	if (IsFinished() || bClosing || !HasValidContext() || !ASC->GetActiveGameplayEffect(Applied))
	{
		if (Applied.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Applied);
		}
		if (!IsFinished())
		{
			EndTask();
		}
		return;
	}
	DefenseHandle = Applied;
	FOnActiveGameplayEffectRemoved_Info* Removed = ASC->OnGameplayEffectRemoved_InfoDelegate(DefenseHandle);
	if (!Removed)
	{
		EndTask();
		return;
	}
	EffectRemovedBinding = Removed->AddUObject(this, &ThisClass::HandleEffectRemoved);
	DamageBlockedBinding = BoundHealthSet->OnDamageBlocked.AddUObject(this, &ThisClass::HandleDamageBlocked);
	bOpened = true;
	TimerWorld->GetTimerManager().SetTimer(WindowTimer, this, &ThisClass::HandleTimeout, WindowDuration, false);
	UE_LOG(LogDCAbilitySystem, Verbose, TEXT("DashDefense opened on %s for %.3fs"), *GetNameSafe(InitialAvatar.Get()), WindowDuration);
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData Event;
		Event.Target = InitialAvatar.Get();
		OnOpened.Broadcast(Event);
	}
}

void UDCAbilityTask_DashDefenseWindow::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (!IsDefenseWindowActive())
	{
		EndTask();
	}
}

void UDCAbilityTask_DashDefenseWindow::HandleDamageBlocked(const FGameplayEffectSpec& DamageSpec, float BlockedDamage)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (bReportedSuccess || !FMath::IsFinite(BlockedDamage) || BlockedDamage <= 0.0f || !IsDefenseWindowActive()
		|| DamageSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct))
	{
		return;
	}
	// This task's GE contributes exactly one immunity count. Other immunity or cheats
	// make causal attribution ambiguous: still block damage, but do not award a dodge.
	if (ASC->GetGameplayTagCount(DCGameplayTags::Gameplay_DamageImmunity) != 1
		|| ASC->HasMatchingGameplayTag(DCGameplayTags::Cheat_GodMode)
		|| ASC->HasMatchingGameplayTag(DCGameplayTags::Cheat_UnlimitedHealth))
	{
		return;
	}
	bReportedSuccess = true; // Set before delegates to make reentrant damage/event calls safe.
	FGameplayEventData Event;
	Event.EventTag = DCGameplayTags::GameplayEvent_Dodge_Success;
	Event.Instigator = DamageSpec.GetContext().GetOriginalInstigator();
	Event.Target = InitialAvatar.Get();
	Event.EventMagnitude = BlockedDamage;
	Event.ContextHandle = DamageSpec.GetContext();
	if (const FGameplayTagContainer* SourceTags = DamageSpec.CapturedSourceTags.GetAggregatedTags())
	{
		Event.InstigatorTags = *SourceTags;
	}
	if (const FGameplayTagContainer* TargetTags = DamageSpec.CapturedTargetTags.GetAggregatedTags())
	{
		Event.TargetTags = *TargetTags;
	}
	UE_LOG(LogDCAbilitySystem, Verbose, TEXT("DashDefense blocked %.3f damage on %s"), BlockedDamage, *GetNameSafe(InitialAvatar.Get()));
	ASC->HandleGameplayEvent(Event.EventTag, &Event);
	if (!IsFinished() && ShouldBroadcastAbilityTaskDelegates())
	{
		OnDamageAvoided.Broadcast(Event);
	}
}

void UDCAbilityTask_DashDefenseWindow::HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo) { EndTask(); }
void UDCAbilityTask_DashDefenseWindow::HandleAbilityEnded(UGameplayAbility* EndedAbility) { EndTask(); }
void UDCAbilityTask_DashDefenseWindow::HandleOutOfHealth(AActor*, AActor*, const FGameplayEffectSpec*, float, float, float) { EndTask(); }
void UDCAbilityTask_DashDefenseWindow::HandleTimeout() { EndTask(); }
void UDCAbilityTask_DashDefenseWindow::HandleAvatarEndPlay(AActor*, EEndPlayReason::Type) { EndTask(); }

void UDCAbilityTask_DashDefenseWindow::OnDestroy(bool bInOwnerFinished)
{
	if (bClosing)
	{
		return;
	}
	bClosing = true;
	if (UWorld* World = TimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(WindowTimer);
	}
	if (Ability)
	{
		Ability->OnGameplayAbilityEnded.Remove(AbilityEndedBinding);
	}
	if (InitialAvatar.IsValid())
	{
		InitialAvatar->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleAvatarEndPlay);
	}
	if (const UDCHealthSet* Health = BoundHealthSet.Get())
	{
		Health->OnDamageBlocked.Remove(DamageBlockedBinding);
		Health->OnOutOfHealth.Remove(OutOfHealthBinding);
	}
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (FOnActiveGameplayEffectRemoved_Info* Removed = ASC->OnGameplayEffectRemoved_InfoDelegate(DefenseHandle))
		{
			Removed->Remove(EffectRemovedBinding);
		}
		if (DefenseHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(DefenseHandle);
		}
	}
	DefenseHandle.Invalidate();
	if (bOpened)
	{
		UE_LOG(LogDCAbilitySystem, Verbose, TEXT("DashDefense closed on %s"), *GetNameSafe(InitialAvatar.Get()));
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			FGameplayEventData Event;
			Event.Target = InitialAvatar.Get();
			OnClosed.Broadcast(Event);
		}
	}
	bOpened = false;
	Super::OnDestroy(bInOwnerFinished);
}
