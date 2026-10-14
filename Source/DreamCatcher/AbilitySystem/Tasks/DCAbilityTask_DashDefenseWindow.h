// DreamCatcher extension. Movement, montage, and cooldown remain in the Lyra Dash BP.
#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "DCAbilityTask_DashDefenseWindow.generated.h"

class UDCHealthSet;
class UGameplayEffect;
struct FGameplayEffectSpec;
struct FGameplayEffectRemovalInfo;

// Same signature on all outputs, as required by the latent Blueprint task node.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCDashDefenseWindowEvent, const FGameplayEventData&, EventData);

/** Authority-only, bounded damage immunity owned by one committed Dash ability. */
UCLASS()
class DREAMCATCHER_API UDCAbilityTask_DashDefenseWindow : public UAbilityTask
{
	GENERATED_BODY()

public:
	UDCAbilityTask_DashDefenseWindow(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintAssignable)
	FDCDashDefenseWindowEvent OnOpened;
	UPROPERTY(BlueprintAssignable)
	FDCDashDefenseWindowEvent OnClosed;
	UPROPERTY(BlueprintAssignable)
	FDCDashDefenseWindowEvent OnDamageAvoided;

	/** Wire the actual Commit result on both local-authority and SendInfo server paths.
	 * Duration is the original RootMotionDuration (currently 0.25), not AbilityDuration.
	 * DefenseEffect must be a non-stacking, non-periodic HasDuration tag-only GE.
	 * Client calls end without granting immunity or emitting authority events.
	 */
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UDCAbilityTask_DashDefenseWindow* DashDefenseWindow(UGameplayAbility* OwningAbility,
		bool bCommitSucceeded, TSubclassOf<UGameplayEffect> DefenseEffect, float Duration);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Ability|Tasks")
	bool IsDefenseWindowActive() const;

	bool HasReportedDamageAvoided() const { return bReportedSuccess; }
	FActiveGameplayEffectHandle GetDefenseEffectHandle() const { return DefenseHandle; }

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool HasValidContext() const;
	void HandleDamageBlocked(const FGameplayEffectSpec& DamageSpec, float BlockedDamage);
	void HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);
	void HandleAbilityEnded(UGameplayAbility* EndedAbility);
	void HandleOutOfHealth(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec,
		float Magnitude, float OldHealth, float NewHealth);
	void HandleTimeout();
	UFUNCTION()
	void HandleAvatarEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DefenseEffectClass;
	TWeakObjectPtr<const UDCHealthSet> BoundHealthSet;
	TWeakObjectPtr<AActor> InitialAvatar;
	TWeakObjectPtr<AActor> InitialOwner;
	TWeakObjectPtr<UWorld> TimerWorld;
	FActiveGameplayEffectHandle DefenseHandle;
	FTimerHandle WindowTimer;
	FDelegateHandle DamageBlockedBinding;
	FDelegateHandle OutOfHealthBinding;
	FDelegateHandle EffectRemovedBinding;
	FDelegateHandle AbilityEndedBinding;
	float WindowDuration = 0.0f;
	bool bCommitted = false;
	bool bOpened = false;
	bool bClosing = false;
	bool bReportedSuccess = false;
};
