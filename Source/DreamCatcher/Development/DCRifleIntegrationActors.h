#pragma once

#include "CoreMinimal.h"
#include "DreamCatcherGameMode.h"
#include "DreamCatcherPlayerController.h"
#include "DCRifleIntegrationActors.generated.h"

class UDCInventoryItemDefinition;
class UDCInventoryItemInstance;
class UDCInventoryManagerComponent;
class UDCLyraQuickBarComponent;
class UDCLyraWeaponUserInterface;
class UDCLyraWeaponStateComponent;
class UDCPawnData;
class UDCPawnExtensionComponent;

/** Opt-in R5/R6/R7 test controller. Gameplay and weapon widgets remain in imported Lyra assets. */
UCLASS()
class DREAMCATCHER_API ADCRifleIntegrationController : public ADreamCatcherPlayerController
{
	GENERATED_BODY()
public:
	ADCRifleIntegrationController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="R5 Integration")
	void EquipIntegrationRifle();

	UFUNCTION(BlueprintCallable, Category="R5 Integration")
	void ClearIntegrationRifle();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="R5 Integration")
	TSubclassOf<UDCInventoryItemDefinition> StartingRifle;

	/** Independent R7 display only; does not replace the project's HUD or UIExtension policy. */
	UPROPERTY(EditDefaultsOnly, Category="R7 Integration")
	TSubclassOf<UDCLyraWeaponUserInterface> WeaponReticleHostClass;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="R7 Integration")
	TObjectPtr<UDCLyraWeaponUserInterface> WeaponReticleHost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="R5 Integration")
	TObjectPtr<UDCInventoryManagerComponent> RifleInventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="R5 Integration")
	TObjectPtr<UDCLyraQuickBarComponent> RifleQuickBar;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="R5 Integration")
	TObjectPtr<UDCLyraWeaponStateComponent> RifleWeaponState;

private:
	void TryEquipWhenReady();
	void DetachPawnListener();
	void UpdateWeaponReticleHost();
	void RemoveWeaponReticleHost();
	TWeakObjectPtr<APawn> WeaponReticlePawn;
	bool bWeaponReticleHostCreationFailed = false;
	bool bWeaponReticleHostEndingPlay = false;
	UPROPERTY(Transient) TObjectPtr<UDCInventoryItemInstance> GrantedRifle;
	TWeakObjectPtr<UDCPawnExtensionComponent> BoundPawnExtension;
	FTimerHandle EquipTimer;
	int32 RemainingReadinessChecks = 0;
	bool bClearingLoadout = false;
};

/** Chooses PawnData before the existing GameMode's deferred Pawn spawn, for this test map only. */
UCLASS()
class DREAMCATCHER_API ADCRifleIntegrationGameMode : public ADreamCatcherGameMode
{
	GENERATED_BODY()
public:
	ADCRifleIntegrationGameMode();
protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	UPROPERTY(EditDefaultsOnly, Category="R5 Integration")
	TObjectPtr<UDCPawnData> IntegrationPawnData;
};
