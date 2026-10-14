#pragma once

#include "CoreMinimal.h"
#include "DreamCatcherPlayerController.h"
#include "DCLoadoutController.generated.h"

class UDCInventoryItemDefinition;
class UDCInventoryItemInstance;
class UDCInventoryManagerComponent;
class UDCLyraQuickBarComponent;
class UDCLyraWeaponUserInterface;
class UDCLyraWeaponStateComponent;
class UDCPawnData;
class UDCPawnExtensionComponent;

/** Production Inventory/QuickBar loadout owner. Gameplay remains in Lyra assets. */
UCLASS()
class DREAMCATCHER_API ADCRifleIntegrationController : public ADreamCatcherPlayerController
{
	GENERATED_BODY()
public:
	ADCRifleIntegrationController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Loadout")
	void EquipIntegrationRifle();

	UFUNCTION(BlueprintCallable, Category="Loadout")
	void ClearIntegrationRifle();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="Loadout")
	TSubclassOf<UDCInventoryItemDefinition> StartingRifle;

	/** Optional standalone reticle host; the production Experience supplies its own HUD. */
	UPROPERTY(EditDefaultsOnly, Category="Loadout|UI")
	TSubclassOf<UDCLyraWeaponUserInterface> WeaponReticleHostClass;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Loadout|UI")
	TObjectPtr<UDCLyraWeaponUserInterface> WeaponReticleHost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loadout")
	TObjectPtr<UDCInventoryManagerComponent> RifleInventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loadout")
	TObjectPtr<UDCLyraQuickBarComponent> RifleQuickBar;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loadout")
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

