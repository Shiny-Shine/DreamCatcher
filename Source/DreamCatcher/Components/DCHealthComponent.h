#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCHealthComponent.generated.h"

class UDCAbilitySystemComponent;
class UDCHealthSet;
class UGameplayEffect;

struct FGameplayEffectSpec;
struct FGameplayTag;

// 기존 HUD와 Blueprint 호환성을 유지하는 이벤트.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCDeathSignature, AActor*, DeadActor);

// 사망 처리 단계 구분.
// Health가 0인 것과 실제 사망 처리가 시작된 것은 서로 다른 상태로 관리.
UENUM(BlueprintType)
enum class EDCDeathState : uint8
{
	NotDead UMETA(DisplayName = "Not Dead"),
	DeathStarted UMETA(DisplayName = "Death Started"),
	DeathFinished UMETA(DisplayName = "Death Finished")
};

/**
 * GAS HealthSet을 관찰하고 기존 Character/HUD 인터페이스로 중계.
 *
 * 플레이어:
 *   UDCHealthSet이 실제 체력 데이터 원본입니다.
 *
 * 아직 GAS로 전환하지 않은 적:
 *   기존 CurrentHealth와 MaxHealth를 임시로 사용.
 */
UCLASS(ClassGroup = (DreamCatcher), meta = (BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDCHealthComponent();

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FDCHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FDCDeathSignature OnDeath;

	// 사망 처리가 완전히 끝났을 때 발생.
	// 현재 OnDeath는 DeathStarted 시점에 발생해 기존 Character와 GameMode 호환성을 유지.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FDCDeathSignature OnDeathFinished;

	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Health")
	void StartDeath();

	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Health")
	void FinishDeath();

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Health")
	EDCDeathState GetDeathState() const
	{
		return DeathState;
	}

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Health")
	bool IsDeadOrDying() const
	{
		return DeathState != EDCDeathState::NotDead;
	}

	// 이 컴포넌트를 지정한 ASC의 HealthSet과 연결.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Health")
	void InitializeWithAbilitySystem(UDCAbilitySystemComponent* InAbilitySystemComponent);

	// 현재 ASC와 AttributeSet 연결을 해제.
	UFUNCTION(BlueprintCallable, Category = "DreamCatcher|Health")
	void UninitializeFromAbilitySystem();

	// Legacy 호환 함수.
	// GAS 연결 상태에서는 Attribute를 변경하고, 연결되지 않은 기존 적은 내부 Legacy 값을 변경.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth = true);

	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float Damage, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Health")
	float Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetToFull();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Health")
	bool IsUsingAbilitySystem() const
	{
		return AbilitySystemComponent != nullptr && HealthSet != nullptr;
	}

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ASC가 없는 기존 Actor가 사용하는 임시 최대 체력. 플레이어가 GAS에 연결되면 이 값은 캐시 역할만 함.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	// ASC가 없는 기존 Actor가 사용하는 임시 현재 체력. 플레이어가 GAS에 연결되면 HealthSet 값을 반영하는 캐시.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	// GAS Actor에게 데미지를 적용할 때 사용하는 GameplayEffect. BP_DC_Player_GAS_Test에서 GE_DC_Damage를 할당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Health|Gameplay Effects")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

	// GAS Actor를 회복할 때 사용하는 GameplayEffect. BP_DC_Player_GAS_Test에서 GE_DC_Heal을 할당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Health|Gameplay Effects")
	TSubclassOf<UGameplayEffect> HealGameplayEffectClass;

private:
	// PawnExtension의 ASC 초기화 이벤트를 받음.
	void HandleAbilitySystemInitialized();

	// PawnExtension의 ASC 해제 이벤트를 받음.
	void HandleAbilitySystemUninitialized();

	void HandleHealthAttributeChanged(
		AActor* EffectInstigator,
		AActor* EffectCauser,
		const FGameplayEffectSpec* EffectSpec,
		float EffectMagnitude,
		float OldValue,
		float NewValue
	);

	void HandleMaxHealthAttributeChanged(
		AActor* EffectInstigator,
		AActor* EffectCauser,
		const FGameplayEffectSpec* EffectSpec,
		float EffectMagnitude,
		float OldValue,
		float NewValue
	);

	void HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec,
	                       float EffectMagnitude, float OldValue, float NewValue);

	// 지정한 GameplayEffect Spec을 만들고 SetByCaller 값을 넣어 현재 ASC에 적용.
	bool ApplySetByCallerGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffectClass,
	                                    const FGameplayTag& SetByCallerTag, float Magnitude, AActor* EffectCauser);

	// 현재 데이터 원본에서 값을 다시 읽어 HUD에 전달.
	void BroadcastCurrentHealth();

	// 죽음 이벤트가 한 번만 발생하도록 처리.
	void BroadcastDeathIfNeeded();
	
	void ClearGameplayTags();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Health",
		meta = (AllowPrivateAccess = "true"))
	EDCDeathState DeathState = EDCDeathState::NotDead;

	UPROPERTY(Transient)
	TObjectPtr<UDCAbilitySystemComponent>
	AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<const UDCHealthSet>
	HealthSet;

	bool bDeathBroadcasted = false;
};
