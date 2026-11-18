#pragma once

#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "DCTraversalCombatComponent.generated.h"

class ACharacter;
class AController;
class APawn;
class UAnimInstance;
class UAnimMontage;
class UDCAbilitySystemComponent;
class UDCLyraEquipmentInstance;
class UDCLyraHealthComponent;
class UDCPawnExtensionComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCTraversalCleanupRequested);

struct FDCTraversalWeaponAttachment
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<USceneComponent> Root;
	TWeakObjectPtr<USceneComponent> Parent;
	FName Socket;
	FTransform RelativeTransform;
};

/** Opt-in standalone GASP coordinator and editable back-mount point. */
UCLASS(ClassGroup=(DreamCatcher), meta=(BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCTraversalCombatComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDCTraversalCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="DC|Traversal")
	static UDCTraversalCombatComponent* FindTraversalCombatComponent(const AActor* Actor);

	// Entry hooks pass through for sample actors without a standalone coordinator.
	UFUNCTION(BlueprintPure, Category="DC|Traversal")
	static bool CanRequestTraversalOrJump(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="DC|Traversal")
	static bool TryBeginTraversalForActor(AActor* Actor, UAnimMontage* Montage, UPrimitiveComponent* Obstacle);

	// Call after the original CMC montage/start-state chain. False routes to CMC cleanup.
	UFUNCTION(BlueprintCallable, Category="DC|Traversal")
	static bool ConfirmTraversalForActor(AActor* Actor, UAnimMontage* Montage);

	UFUNCTION(BlueprintPure, Category="DC|Traversal")
	static TEnumAsByte<EMovementMode> ResolveTraversalEndMovementMode(AActor* Actor, TEnumAsByte<EMovementMode> DesiredMode);

	// Skip the original SetMovementMode call if another system owns the current mode/submode.
	UFUNCTION(BlueprintPure, Category="DC|Traversal")
	static bool ShouldRestoreTraversalMovementMode(AActor* Actor);

	// Call after the original CMC movement/collision cleanup, before its existing delay.
	UFUNCTION(BlueprintCallable, Category="DC|Traversal")
	static void NotifyTraversalMovementCleanup(AActor* Actor);

	UFUNCTION(BlueprintPure, Category="DC|Traversal")
	static bool IsTraversalCombatActive(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="DC|Traversal")
	void AbortTraversal();

	// Bind the GASP player's original/shared CMC cleanup event; it must not start a new action.
	UPROPERTY(BlueprintAssignable, Category="DC|Traversal")
	FDCTraversalCleanupRequested OnTraversalCleanupRequested;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend struct FDCTraversalCombatTestAccessor;
	bool IsStandaloneGame() const;
	bool CanRequest() const;
	bool IsOwnerAlive() const;
	bool IsOwnerDead() const;
	UDCAbilitySystemComponent* FindOwnerASC() const;
	bool BeginTraversal(UAnimMontage* Montage, UPrimitiveComponent* Obstacle);
	bool ConfirmTraversal(UAnimMontage* Montage);
	void FinishTraversal(bool bAbort);
	void CleanupMovement();
	void RestoreObstacleIgnore();
	bool CaptureWeapon();
	bool HolsterCapturedWeapon();
	void RestoreWeapon();
	bool IsCapturedWeaponStillEquipped() const;
	void StopTrackedMontage();

	UFUNCTION()
	void HandleHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator);
	UFUNCTION()
	void HandleDeathStarted(AActor* Actor);
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);
	void HandleAbilitySystemUninitializing();

	TWeakObjectPtr<UDCLyraHealthComponent> BoundHealth;
	TWeakObjectPtr<UDCPawnExtensionComponent> BoundPawnExtension;
	TWeakObjectPtr<UDCAbilitySystemComponent> ActiveASC;
	TWeakObjectPtr<UAnimInstance> ActiveAnimInstance;
	TWeakObjectPtr<UAnimMontage> ActiveMontage;
	TWeakObjectPtr<UAnimInstance> RetiredAnimInstance;
	TWeakObjectPtr<UDCLyraEquipmentInstance> ActiveWeapon;
	TWeakObjectPtr<UPrimitiveComponent> TraversalObstacle;
	TArray<FDCTraversalWeaponAttachment> WeaponAttachments;
	FGameplayTagContainer BlockedAbilityTypes;
	FActiveGameplayEffectHandle TraversalStateEffect;
	int32 MontageInstanceID = INDEX_NONE;
	int32 RetiredMontageInstanceID = INDEX_NONE;
	uint64 PreparedFrame = 0;
	uint64 ScopeSerial = 0;
	uint64 AdmissionAfterFrame = 0;
	bool bTraversalActive = false;
	bool bFinishing = false;
	bool bEndingPlay = false;
	bool bOwnsAbilityBlocks = false;
	bool bMovementClaimed = false;
	bool bMovementCleanedUp = false;
	bool bObstacleWasIgnored = false;
};
