// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Weapon/DCWeaponTypes.h"
#include "DreamCatcherCharacter.generated.h"

class UAbilitySystemComponent;
class UDCCameraComponent;
class UDCAbilitySystemComponent;
class UDCLyraHealthComponent;
class UDCPawnExtensionComponent;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCGASAimModeChangedSignature, EDCAimMode, NewAimMode);

/** Player avatar; Hero, PlayerState ASC and Lyra equipment own gameplay. */
UCLASS()
class DREAMCATCHER_API ADreamCatcherCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ADreamCatcherCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Ability System")
	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const;
	UFUNCTION(BlueprintPure, Category="Components")
	UDCLyraHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UFUNCTION(BlueprintPure, Category="Weapon|Animation")
	EDCWeaponAnimationType GetWeaponAnimationType() const { return WeaponAnimationType; }
	UFUNCTION(BlueprintPure, Category="DreamCatcher|Aim")
	EDCAimMode GetGASAimMode() const { return CurrentGASAimMode; }
	UPROPERTY(BlueprintAssignable, Category="DreamCatcher|Aim")
	FDCGASAimModeChangedSignature OnGASAimModeChanged;
	UFUNCTION(BlueprintCallable, Category="DC|Character")
	void ToggleCrouch();
	float GetCurrentLookSensitivityMultiplier() const;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void UnPossessed() override;
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);
	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();
	UFUNCTION(BlueprintImplementableEvent, Category="Aim")
	void BP_OnAimModeChanged(EDCAimMode NewAimMode);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCCameraComponent> ModeCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCPawnExtensionComponent> PawnExtensionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCLyraHealthComponent> HealthComponent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
	EDCWeaponAnimationType WeaponAnimationType = EDCWeaponAnimationType::Rifle;
private:
	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();
	void InitializeCameraModeSystem();
	void InitializeGASAimStateListeners();
	void UninitializeGASAimStateListeners();
	void HandleGASAimStateTagChanged(const FGameplayTag ChangedTag, int32 NewCount);
	void RefreshGASAimMode(bool bForceBroadcast);
	TWeakObjectPtr<UDCAbilitySystemComponent> BoundAimAbilitySystemComponent;
	FDelegateHandle ShoulderAimTagDelegateHandle;
	FDelegateHandle ScopeAimTagDelegateHandle;
	EDCAimMode CurrentGASAimMode = EDCAimMode::Hip;
};
