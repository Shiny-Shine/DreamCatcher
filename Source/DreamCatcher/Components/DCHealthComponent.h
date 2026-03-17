#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCHealthComponent.generated.h"

// 체력 변화 시 UI, 피격 이펙트 등이 반응할 수 있도록 이벤트를 노출.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

// 죽음 처리도 외부(Character, HUD, Director)가 구독할 수 있게 분리.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCDeathSignature, AActor*, DeadActor);

/*
 * 이 컴포넌트는 "체력 규칙"만 담당.
 * 실제 피격 애니메이션, 사망 연출, UI 갱신은 이 이벤트를 받은 바깥쪽에서 처리.
 */
UCLASS(ClassGroup=(DreamCatcher), meta=(BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDCHealthComponent();

	// 블루프린트에서도 체력 변화 이벤트를 바인딩할 수 있게 공개.
	UPROPERTY(BlueprintAssignable, Category="Health")
	FDCHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FDCDeathSignature OnDeath;

	// 최대 체력을 바꾸는 함수.
	// 레벨업, 장비 변경, 업그레이드 같은 시스템이 나중에 이 함수를 호출.
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth = true);

	// 가장 단순한 형태의 데미지 적용 함수.
	// 현재는 로컬 규칙만 처리하고, 데미지 타입/방어력/속성 상성은 나중에 확장.
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDamage(float Damage, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category="Health")
	float Heal(float Amount);

	// 체력을 최대치로 초기화.
	// 재시작, 재스폰, 스테이지 시작 시 사용.
	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetToFull();

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

protected:
	// BeginPlay는 "게임이 실제로 시작된 뒤" 호출.
	// 에디터에서 조정된 MaxHealth 값이 여기서 최종 반영됨.
	virtual void BeginPlay() override;

	// EditAnywhere:
	// 블루프린트 기본값이나 Details 패널에서 디자이너가 조정할 수 있음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	// VisibleAnywhere:
	// 런타임 상태를 보기는 쉽지만 함부로 수정하지 않게 하기 위함.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 100.0f;
};