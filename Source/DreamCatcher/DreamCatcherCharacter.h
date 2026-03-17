#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DreamCatcherCharacter.generated.h"

class UCameraComponent;
class UDCCombatComponent;
class UDCHealthComponent;
class UInputAction;
class USpringArmComponent;
struct FInputActionValue;

/*
 * Character는 플레이어 몸체.
 * 이동, 카메라, 메시, 애니메이션 기준점이 여기에 붙는다.
 *
 * 이 클래스의 역할:
 * - 입력을 받는다
 * - 체력/전투 컴포넌트의 이벤트를 연결한다
 * - 블루프린트에 연출 훅을 넘긴다
 */
UCLASS()
class DREAMCATCHER_API ADreamCatcherCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Unreal의 기본 데미지 파이프라인과 연결.
	ADreamCatcherCharacter();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category="Components")
	UDCHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UDCCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// CameraBoom:
	// 캐릭터 뒤에서 카메라를 따라오게 하는 스프링 암.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// 실제 플레이 카메라.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	// 체력 규칙을 담당하는 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCHealthComponent> HealthComponent;

	// 발사/회피/궁극기 규칙을 담당하는 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCCombatComponent> CombatComponent;

	// 아래 InputAction들은 블루프린트 기본값에서 연결.
	// Unity의 Input Action Asset 참조와 유사.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> UltimateAction;

	// 총구 소켓 이름.
	// 블루프린트 메시에서 이 소켓을 만들어 두면 여기서 가져다 쓰는 구조.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 카메라에서 조준할 최대 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="1000.0", Units="cm"))
	float FireTraceDistance = 10000.0f;

	// 회피 시 앞으로 밀어주는 힘.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float DodgeImpulse = 900.0f;

	// 회피 시 살짝 띄워서 벽/바닥 마찰에 덜 걸리게 함.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float DodgeLiftImpulse = 120.0f;

	// C++는 "언제 발사해야 하는지"까지만 결정하고,
	// 실제 총알/VFX/사운드는 블루프린트에서 처리.
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnPrimaryFireRequested(const FVector& MuzzleLocation, const FVector& AimPoint);

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnDodgeRequested(const FVector& DodgeDirection);

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnUltimateRequested();

	UFUNCTION(BlueprintImplementableEvent, Category="Character")
	void BP_OnDeath();
	
	// 피격 연출은 블루프린트에서 처리.
	UFUNCTION(BlueprintImplementableEvent, Category="Character")
	void BP_OnDamaged(float CurrentHealth, float MaxHealth, AActor* DamageCauser);

private:
	// Enhanced Input에서 호출되는 실제 입력 함수들.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartPrimaryFire();
	void StopPrimaryFire();
	void Dodge();
	void Ultimate();

	// 컴포넌트 이벤트를 받아 Character 레벨 동작으로 바꾸는 함수들.
	UFUNCTION()
	void HandlePrimaryFireRequested();

	UFUNCTION()
	void HandleDodgeRequested();

	UFUNCTION()
	void HandleUltimateRequested();

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);
};