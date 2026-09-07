#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "DCGASTestTarget.generated.h"

class UAbilitySystemComponent;
class UDCAbilitySystemComponent;
class UDCHealthSet;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

// 사격과 GAS 데미지 연결을 검증하는 표적. AI와 사망 Ability는 사용 X.
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCGASTestTarget : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADCGASTestTarget();

	virtual UAbilitySystemComponent*
	GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Test")
	float GetCurrentHealth() const;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UDCHealthSet> HealthSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Test", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "DreamCatcher|Test")
	void BP_OnHealthChanged(float CurrentHealth, float MaxHealth);

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	FDelegateHandle HealthChangedHandle;
};
