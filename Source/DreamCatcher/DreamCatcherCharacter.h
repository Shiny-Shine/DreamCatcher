#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/DCCombatComponent.h"
#include "Weapon/DCWeaponTypes.h"
#include "DreamCatcherCharacter.generated.h"

class UCameraComponent;
class UDCHealthComponent;
class USceneComponent;
class UStaticMeshComponent;
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

// 조준 상태 정의를 위한 카메라 프로필 구조체
USTRUCT(BlueprintType)
struct FDCAimCameraProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Camera", meta=(ClampMin="5.0", ClampMax="170.0"))
	float FieldOfView = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Camera", meta=(ClampMin="0.0", Units="cm"))
	float TargetArmLength = 325.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Camera")
	FVector SocketOffset = FVector(0.0f, 55.0f, 65.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Camera", meta=(ClampMin="0.01"))
	float LookSensitivityMultiplier = 1.0f;
};

UCLASS()
class DREAMCATCHER_API ADreamCatcherCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Unreal의 기본 데미지 파이프라인과 연결.
	ADreamCatcherCharacter();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category="Components")
	UDCHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UDCCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	// 임시 무기 모델을 표시하는 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 총알과 사격 연출이 시작되는 총구 위치.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> MuzzlePoint;
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	EDCWeaponAnimationType GetWeaponAnimationType() const
	{
		return WeaponAnimationType;
	}

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void UnPossessed() override;

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

	// 이동 입력.
	// IMC_Player 안의 WASD 입력과 연결.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	// 점프 입력.
	// C++ 빌드 후 BP_DreamCatcherChracter에서 IA_Jump를 할당.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> UltimateAction;


	// 캐릭터 오른손에 무기를 부착할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FName WeaponSocketName = TEXT("Weapon_R");

	// 카메라에서 조준할 최대 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="1000.0", Units="cm"))
	float FireTraceDistance = 10000.0f;

	// 구르기를 시작할 때 적용할 수평 속도.
	// LaunchCharacter를 사용하지 않고 CharacterMovement의 수평 Velocity를
	// 직접 설정하므로 캐릭터가 Falling 상태로 바뀌지 않음.
	// 단위는 cm/s.
	// 초기 테스트값 900은 약간 빠른 구르기용 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Dodge", meta=(ClampMin="0.0", Units="cm/s"))
	float DodgeSpeed = 900.0f;

	// C++는 "언제 발사해야 하는지"까지만 결정하고,
	// 실제 총알/VFX/사운드는 블루프린트에서 처리.
	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnPrimaryFireResolved(const FVector& MuzzleLocation, const FVector& FireEnd, AActor* HitActor, float AppliedDamage);

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnDodgeRequested(const FVector& DodgeDirection);

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_OnUltimateRequested();

	UFUNCTION(BlueprintImplementableEvent, Category="Character")
	void BP_OnDeath();

	// 피격 연출은 블루프린트에서 처리.
	UFUNCTION(BlueprintImplementableEvent, Category="Character")
	void BP_OnDamaged(float CurrentHealth, float MaxHealth, AActor* DamageCauser);

	// 조준 상태 정의를 위한 카메라 프로필
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim|Input", meta=(ClampMin="0.05", Units="s"))
	float AimHoldThreshold = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim|Camera")
	FDCAimCameraProfile HipCameraProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim|Camera")
	FDCAimCameraProfile ShoulderCameraProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim|Camera")
	FDCAimCameraProfile ScopeCameraProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim|Camera", meta=(ClampMin="0.1"))
	float AimCameraBlendSpeed = 12.0f;

	UFUNCTION(BlueprintImplementableEvent, Category="Aim")
	void BP_OnAimModeChanged(EDCAimMode NewAimMode);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	EDCWeaponAnimationType WeaponAnimationType =
		EDCWeaponAnimationType::Rifle;
	
	
	// 카메라 반동
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Recoil",
	meta=(ClampMin="0.0"))
	float RecoilKickInterpSpeed = 90.0f;	// 반동을 카메라에 얼마나 빠르게 적용할지

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Recoil",
		meta=(ClampMin="0.0", Units="deg"))
	float MaxAccumulatedRecoilPitch = 10.0f;	// 적용되지 않은 수직 반동의 최대치

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Recoil",
		meta=(ClampMin="0.0", Units="deg"))
	float MaxAccumulatedRecoilYaw = 1.0f;	// 적용되지 않은 좌우 반동의 최대치

private:
	// Enhanced Input에서 호출되는 실제 입력 함수들.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// Space Bar를 누른 순간 호출됩니다.
	void StartJump();

	// Space Bar를 떼거나 입력이 취소될 때 호출됩니다.
	void StopJump();

	void StartPrimaryFire();
	void StopPrimaryFire();
	void Dodge();
	void Ultimate();

	// 조준 상태 정의를 위한 카메라 프로필
	void AimPressed();
	void AimReleased();
	void AimCanceled();
	void ActivateShoulderAim();
	void CancelAimInputAndState();

	const FDCAimCameraProfile& GetAimCameraProfile(EDCAimMode AimMode) const;

	UFUNCTION()
	void HandleAimModeChanged(EDCAimMode NewAimMode);

	bool bAimInputPressed = false;
	bool bShoulderAimActivated = false;
	float CurrentLookSensitivityMultiplier = 1.0f;

	FTimerHandle AimHoldTimerHandle;

	// 컴포넌트 이벤트를 받아 Character 레벨 동작으로 바꾸는 함수들.
	UFUNCTION()
	void HandleShotFired(float ShotSpreadDegrees, float PitchKickDegrees, float YawKickDegrees);
	
	UFUNCTION()
	void HandleDodgeRequested();

	UFUNCTION()
	void HandleUltimateRequested();

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	void UpdateCameraRecoil(float DeltaSeconds);

	// 아직 카메라 회전에 적용되지 않은 반동량.
	// X = Pitch, Y = Yaw
	FVector2D PendingRecoil = FVector2D::ZeroVector;
};
