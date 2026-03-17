#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCEnemyCharacter.generated.h"

class UDCHealthComponent;
class ADCEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCEnemyDeathSignature, ADCEnemyCharacter*, DeadEnemy);

UCLASS()
class DREAMCATCHER_API ADCEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADCEnemyCharacter();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FDCEnemyDeathSignature OnEnemyDeath;

	UFUNCTION(BlueprintPure, Category="Enemy")
	UDCHealthComponent* GetHealthComponent() const { return HealthComponent; }

	// 플레이어와 동일하게 엔진 기본 데미지 파이프라인을 받습니다.
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCHealthComponent> HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0", Units="cm/s"))
	float MoveSpeed = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0", Units="cm"))
	float AttackRange = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0", Units="cm"))
	float MoveAcceptanceRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0", Units="s"))
	float AttackInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0"))
	float AttackDamage = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.0", Units="s"))
	float DestroyDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	FName MuzzleSocketName = TEXT("Muzzle");

	UFUNCTION(BlueprintImplementableEvent, Category="Enemy")
	void BP_OnAttackShot(const FVector& FireStart, const FVector& FireEnd, AActor* HitActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Enemy")
	void BP_OnDamaged(float HealthNormalized);

	UFUNCTION(BlueprintImplementableEvent, Category="Enemy")
	void BP_OnDeath();

private:
	void UpdateMovement();
	void TryFireAtTarget();

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	TWeakObjectPtr<AActor> CurrentTarget;
	FTimerHandle MovementTimerHandle;
	float NextAttackTime = 0.0f;
	bool bIsDead = false;
};