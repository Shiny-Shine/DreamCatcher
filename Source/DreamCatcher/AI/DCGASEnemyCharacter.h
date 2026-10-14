#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/DCAbilitySet.h"
#include "GameFramework/Character.h"
#include "Teams/DCTeamAgentInterface.h"
#include "DCGASEnemyCharacter.generated.h"

class UDCAbilitySystemComponent;
class UDCAbilityTagRelationshipMapping;
class UDCCombatSet;
class UDCHealthSet;
class UDCLyraGameplayAbility_Death;
class UDCGameplayAbility_EnemyRangedAttack;
class UDCLyraHealthComponent;
class ADCGASEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCGASEnemyDeathSignature, ADCGASEnemyCharacter*, DeadEnemy);

/** Single-life NPC adapter around the ported Lyra GAS/health/death classes.
 * R10-1 stays passive by default. R10-2 attack requests are opt-in; no legacy health store.
 */
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCGASEnemyCharacter : public ACharacter,
	public IAbilitySystemInterface, public IDCTeamAgentInterface
{
	GENERATED_BODY()
public:
	ADCGASEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FOnDCTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Enemy|GAS")
	UDCLyraHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UFUNCTION(BlueprintPure, Category="Enemy|GAS")
	bool IsGASReady() const { return bGASReady && !bEndingPlay; }
	UFUNCTION(BlueprintPure, Category="Enemy|GAS")
	bool IsDeadOrDying() const;
	/** Server request; requires exactly one selected ability already granted by AbilitySets. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Enemy|Attack")
	bool TryStartRangedAttack(AActor* AttackTarget);
	AActor* GetRequestedRangedAttackTarget() const { return RequestedAttackTarget.Get(); }

	/** Authority-only, once at DeathStarted, not again at FinishDeath/Destroy. */
	UPROPERTY(BlueprintAssignable, Category="Enemy|Events")
	FDCGASEnemyDeathSignature OnEnemyDeath;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCLyraHealthComponent> HealthComponent;
	UPROPERTY()
	TObjectPtr<UDCHealthSet> HealthSet;
	UPROPERTY()
	TObjectPtr<UDCCombatSet> CombatSet;

	/** Required concrete original-derived Death BP; its Duration owns the death delay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|GAS")
	TSubclassOf<UDCLyraGameplayAbility_Death> DeathAbilityClass;
	/** Optional extra grants. Do not grant a second Death ability or Health/Combat set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|GAS")
	TArray<TObjectPtr<const UDCAbilitySet>> AbilitySets;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|GAS")
	TObjectPtr<UDCAbilityTagRelationshipMapping> TagRelationshipMapping;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|GAS", meta=(ClampMin="1.0"))
	float InitialHealth = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_TeamId, Category="Enemy|Team", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamId = 1;

	/** Null/false preserves the accepted R10-1 passive enemy. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack")
	TSubclassOf<UDCGameplayAbility_EnemyRangedAttack> RangedAttackAbilityClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack")
	bool bAutoRangedAttack = false;

	UFUNCTION(BlueprintImplementableEvent, Category="Enemy")
	void BP_OnDamaged(float HealthNormalized);
	UFUNCTION(BlueprintImplementableEvent, Category="Enemy")
	void BP_OnDeath();

private:
	void UninitializeGAS();
	void FailInitialization(const TCHAR* Reason);
	void StopMovementAndCollision();
	void DestroyAfterDeath();
	void UpdateRangedAttack();
	UFUNCTION()
	void HandleHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* DamageInstigator);
	UFUNCTION()
	void HandleDeathStarted(AActor* OwningActor);
	UFUNCTION()
	void HandleDeathFinished(AActor* OwningActor);
	UFUNCTION()
	void OnRep_TeamId(uint8 OldTeamId);

	UPROPERTY()
	FOnDCTeamIndexChangedDelegate OnTeamChanged;
	FGameplayAbilitySpecHandle GrantedDeathAbility;
	FDCAbilitySet_GrantedHandles GrantedHandles;
	FTimerHandle DeathCleanupTimer;
	FTimerHandle AttackRequestTimer;
	TWeakObjectPtr<AActor> RequestedAttackTarget;
	bool bInitializationAttempted = false;
	bool bGASReady = false;
	bool bHealthInitialized = false;
	bool bDeathReported = false;
	bool bDeathFinishScheduled = false;
	bool bEndingPlay = false;
};
