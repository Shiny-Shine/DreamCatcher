#include "DreamCatcherCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DCCombatComponent.h"
#include "Components/DCHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DreamCatcher.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

ADreamCatcherCharacter::ADreamCatcherCharacter()
{
	// Constructor는 기본 부품 조립 단계.
	// 런타임 월드 참조나 다른 액터 찾기는 일반적으로 여기서 안함.

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// 3인칭 슈터 기준으로 캐릭터가 컨트롤러 Yaw를 따르도록 설정.
	// 즉, 카메라 보는 방향과 캐릭터 방향이 같이 감.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 자동 회전하지 않도록 끔.
	// 슈터에서는 "이동 방향"보다 "조준 방향"이 더 중요.
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// 카메라 암을 생성.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 325.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 65.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 18.0f;

	// 실제 카메라를 암 끝에 붙임.
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 체력, 전투를 컴포넌트로 분리해
	// 나중에 적 캐릭터나 보스에도 재사용하기 쉽게 만ㄷ므.
	HealthComponent = CreateDefaultSubobject<UDCHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UDCCombatComponent>(TEXT("CombatComponent"));
	
	// 임시 무기 모델을 표시할 컴포넌트.
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));

	// 캐릭터 Skeletal Mesh의 Weapon_R 소켓에 부착.
	WeaponMesh->SetupAttachment(GetMesh(), WeaponSocketName);

	// 임시 무기가 플레이어의 사격 LineTrace나 이동 충돌을 방해하지 않게 함.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);

	// 총구 위치를 나타내는 빈 Scene Component.
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));

	MuzzlePoint->SetupAttachment(WeaponMesh);

	// 카메라 기본값 설정
	PrimaryActorTick.bCanEverTick = true;

	HipCameraProfile.FieldOfView = 90.0f;
	HipCameraProfile.TargetArmLength = 325.0f;
	HipCameraProfile.SocketOffset = FVector(0.0f, 55.0f, 65.0f);
	HipCameraProfile.LookSensitivityMultiplier = 1.0f;

	ShoulderCameraProfile.FieldOfView = 72.0f;
	ShoulderCameraProfile.TargetArmLength = 235.0f;
	ShoulderCameraProfile.SocketOffset = FVector(0.0f, 80.0f, 65.0f);
	ShoulderCameraProfile.LookSensitivityMultiplier = 0.7f;

	ScopeCameraProfile.FieldOfView = 40.0f;
	ScopeCameraProfile.TargetArmLength = 210.0f;
	ScopeCameraProfile.SocketOffset = FVector(0.0f, 70.0f, 65.0f);
	ScopeCameraProfile.LookSensitivityMultiplier = 0.35f;

	FollowCamera->SetFieldOfView(HipCameraProfile.FieldOfView);
}

void ADreamCatcherCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay 시점에는 컴포넌트와 런타임 객체들이 모두 살아 있으므로
	// 이벤트 바인딩을 여기서 해두는 것이 안전.
	if (CombatComponent)
	{
		CombatComponent->OnDodgeRequested.AddDynamic(this, &ADreamCatcherCharacter::HandleDodgeRequested);
		CombatComponent->OnUltimateRequested.AddDynamic(this, &ADreamCatcherCharacter::HandleUltimateRequested);
		
		CombatComponent->OnShotFired.AddDynamic(this, &ADreamCatcherCharacter::HandleShotFired);

		// 카메라 기본값 설정
		CombatComponent->OnAimModeChanged.AddDynamic(this, &ADreamCatcherCharacter::HandleAimModeChanged);
		
		HandleAimModeChanged(CombatComponent->GetAimMode());
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ADreamCatcherCharacter::HandleDeath);
	}
}

void ADreamCatcherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Character가 플레이어에게 빙의(Possess)된 뒤 호출되어 입력을 연결.
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("Enhanced Input component not found on %s"), *GetName());
		return;
	}

	// 입력 에셋 누락은 흔히 일어나니 바인딩 전에 체크.
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADreamCatcherCharacter::Move);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("MoveAction is not assigned on %s"), *GetName());
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADreamCatcherCharacter::Look);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("LookAction is not assigned on %s"), *GetName());
	}

	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ADreamCatcherCharacter::StartPrimaryFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &ADreamCatcherCharacter::StopPrimaryFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Canceled, this, &ADreamCatcherCharacter::StopPrimaryFire);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("FireAction is not assigned on %s"), *GetName());
	}

	if (JumpAction)
	{
		// 스페이스바를 누른순간 한 번 호출.
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ADreamCatcherCharacter::StartJump);

		// 스페이스바를 뗐을 때 호출.
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADreamCatcherCharacter::StopJump);

		// 입력 컨텍스트가 제거되는 등 입력이 중간에 취소될 때 호출.
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ADreamCatcherCharacter::StopJump);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("JumpAction is not assigned on %s"), *GetName());
	}

	if (DodgeAction)
	{
		EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &ADreamCatcherCharacter::Dodge);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("DodgeAction is not assigned on %s"), *GetName());
	}

	if (UltimateAction)
	{
		EnhancedInput->BindAction(UltimateAction, ETriggerEvent::Started, this, &ADreamCatcherCharacter::Ultimate);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("UltimateAction is not assigned on %s"), *GetName());
	}

	//조준 상태별 입력 바인딩
	if (AimAction)
	{
		EnhancedInput->BindAction(
			AimAction,
			ETriggerEvent::Started,
			this,
			&ADreamCatcherCharacter::AimPressed
		);

		EnhancedInput->BindAction(
			AimAction,
			ETriggerEvent::Completed,
			this,
			&ADreamCatcherCharacter::AimReleased
		);

		EnhancedInput->BindAction(
			AimAction,
			ETriggerEvent::Canceled,
			this,
			&ADreamCatcherCharacter::AimCanceled
		);
	}
	else
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("AimAction is not assigned on %s"), *GetName());
	}
}

void ADreamCatcherCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	// 컨트롤러의 Yaw 기준으로 이동 방향을 계산.
	// 카메라가 보는 방향 기준 WASD 이동.
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Input.X);
}

void ADreamCatcherCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	// 마우스/패드 입력을 컨트롤러 회전에 반영.
	AddControllerYawInput(Input.X * CurrentLookSensitivityMultiplier);
	AddControllerPitchInput(Input.Y * CurrentLookSensitivityMultiplier);
}

void ADreamCatcherCharacter::StartJump()
{
	// 기본제공 Jump()를 사용.
	Jump();
}

void ADreamCatcherCharacter::StopJump()
{
	StopJumping();
}

void ADreamCatcherCharacter::StartPrimaryFire()
{
	if (CombatComponent)
	{
		CombatComponent->StartPrimaryFire();
	}
}

void ADreamCatcherCharacter::StopPrimaryFire()
{
	if (CombatComponent)
	{
		CombatComponent->StopPrimaryFire();
	}
}

void ADreamCatcherCharacter::Dodge()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	// 점프하거나 낙하 중이라면 여기서 종료되므로 공중 구르기는 발생하지 않음.
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return;
	}

	// CombatComponent는 구르기 쿨다운을 확인합니다.
	if (CombatComponent && CombatComponent->TryDodge())
	{
		// 구르기가 실제로 성공한 경우 조준 상태를 해제합니다.
		CancelAimInputAndState();
	}
}

void ADreamCatcherCharacter::Ultimate()
{
	if (CombatComponent && CombatComponent->TryUltimate())
	{
		CancelAimInputAndState();
	}
}

void ADreamCatcherCharacter::HandleShotFired(float ShotSpreadDegrees, float PitchKickDegrees, float YawKickDegrees)
{
	// 카메라 반동 누적
	PendingRecoil.X = FMath::Clamp(PendingRecoil.X + PitchKickDegrees, 0.0f, MaxAccumulatedRecoilPitch);

	PendingRecoil.Y = FMath::Clamp(PendingRecoil.Y + YawKickDegrees, -MaxAccumulatedRecoilYaw, MaxAccumulatedRecoilYaw);

	UWorld* World = GetWorld();

	if (!World || !FollowCamera || !CombatComponent)
	{
		return;
	}
	
	// 1단계: 카메라에서 LineTrace
	// 3인칭 게임에서는 카메라와 총구 위치가 달라 화면 중앙의 크로스헤어가 가리키는 위치를 구함.
	FVector ViewLocation = FollowCamera->GetComponentLocation();
	FRotator ViewRotation = FollowCamera->GetComponentRotation();

	if (Controller)
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	// 탄착 퍼짐 계산
	const float SpreadRadians = FMath::DegreesToRadians(ShotSpreadDegrees);

	const FVector CameraShotDirection =
		SpreadRadians > KINDA_SMALL_NUMBER
			? FMath::VRandCone(ViewRotation.Vector(), SpreadRadians)
			: ViewRotation.Vector();

	const FVector CameraTraceEnd = ViewLocation + CameraShotDirection * FireTraceDistance;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PlayerFireTrace), false, this);

	// 자신의 캡슐, Skeletal Mesh, WeaponMesh를 맞지 않도록
	// 플레이어 Actor 전체를 Trace 대상에서 제외.
	TraceParams.AddIgnoredActor(this);

	FHitResult CameraHit;

	const bool bCameraBlocked = World->LineTraceSingleByChannel(CameraHit, ViewLocation, CameraTraceEnd, ECC_Visibility,TraceParams);

	// 카메라 Trace가 무언가를 맞혔다면 그 위치를 조준점으로 사용.
	// 맞히지 못했다면 최대 사거리 끝을 조준점으로 사용.
	const FVector AimPoint = bCameraBlocked ? CameraHit.ImpactPoint : CameraTraceEnd;
	
	// 2단계: 실제 총구 위치 확인
	FVector MuzzleLocation = GetActorLocation() + GetActorForwardVector() * 100.0f;

	if (MuzzlePoint)
	{
		MuzzleLocation = MuzzlePoint->GetComponentLocation();
	}

	/*
	 * 3단계: 총구에서 조준점까지 LineTrace
	 *
	 * 카메라 Trace만으로 데미지를 주면 총이 벽 뒤에 있는데도 카메라만 벽 너머를 보고
	 * 적을 공격하는 문제가 생길 수 있어 최종 판정은 반드시 총구에서 다시 확인.
	 */
	const FVector ToAimPoint = AimPoint - MuzzleLocation;

	if (ToAimPoint.IsNearlyZero())
	{
		return;
	}

	const FVector ShotDirection = ToAimPoint.GetSafeNormal();

	// 조준점보다 100cm 뒤까지 검사.
	// 표면 경계에서 부동소수점 오차로 명중이 누락되는 것을 줄여줌.
	const FVector MuzzleTraceEnd = AimPoint + ShotDirection * 100.0f;

	FHitResult MuzzleHit;

	const bool bMuzzleBlocked = World->LineTraceSingleByChannel(MuzzleHit, MuzzleLocation, MuzzleTraceEnd, ECC_Visibility, TraceParams);

	const FVector FireEnd = bMuzzleBlocked ? MuzzleHit.ImpactPoint : AimPoint;

	AActor* HitActor = bMuzzleBlocked ? MuzzleHit.GetActor() : nullptr;
	
	// 4단계: 데미지 적용
	float AppliedDamage = 0.0f;

	if (HitActor)
	{
		const float RequestedDamage = CombatComponent->GetPrimaryFireDamage();

		if (RequestedDamage > 0.0f)
		{
			AppliedDamage = UGameplayStatics::ApplyPointDamage(HitActor, RequestedDamage, ShotDirection, MuzzleHit, GetController(), this, UDamageType::StaticClass());
		}
	}

	/*
	 * 5단계: 테스트용 LineTrace 표시
	 *
	 * 초록색: 데미지 적용 성공
	 * 빨간색: 무언가를 맞혔지만 데미지는 적용되지 않음
	 * 흰색: 아무것도 맞히지 못함
	 */
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FColor DebugColor = FColor::White;

	if (bMuzzleBlocked)
	{
		DebugColor = AppliedDamage > 0.0f ? FColor::Green : FColor::Red;
	}

	DrawDebugLine(World, MuzzleLocation, FireEnd, DebugColor, false, 0.75f, 0, 1.5f);

	DrawDebugPoint(World, FireEnd, 10.0f, DebugColor, false, 0.75f);
#endif

	/*
	 * 6단계: Blueprint에 결과 전달
	 *
	 * 여기서는 사운드만 연결하고,
	 * 이후 총구 화염, 트레이서, 피격 VFX 등을 같은 이벤트에 연결.
	 */
	BP_OnPrimaryFireResolved(MuzzleLocation, FireEnd, HitActor,AppliedDamage);
}

void ADreamCatcherCharacter::HandleDodgeRequested()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!Movement)
	{
		return;
	}

	// 이번 프레임에 아직 CharacterMovement가 소비하지 않은 이동 입력을 먼저 확인.
	FVector DodgeDirection = GetPendingMovementInputVector().GetSafeNormal2D();

	// 현재 프레임의 입력을 이미 소비한 경우를 대비하여 직전 프레임의 이동 입력도 확인.
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetLastMovementInputVector().GetSafeNormal2D();
	}

	// Shift만 누른 경우에는 캐릭터가 바라보는 정면으로 구름.
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	// 기존 걷기 속도나 관성 제거.
	Movement->StopMovementImmediately();

	// 구르기 방향으로 수평 속도를 직접 설정.
	Movement->Velocity = DodgeDirection * DodgeSpeed;

	// 실제 구르기 애니메이션, 사운드, VFX는 BP_DreamCatcherChracter에서 구현.
	BP_OnDodgeRequested(DodgeDirection);
}

void ADreamCatcherCharacter::HandleUltimateRequested()
{
	// 궁극기 연출은 보통 캐릭터별로 크게 달라지므로 블루프린트 훅으로 넘김.
	BP_OnUltimateRequested();
}

void ADreamCatcherCharacter::HandleDeath(AActor* /*DeadActor*/)
{
	// 죽는 순간 자동 연사를 끊어줌.
	StopPrimaryFire();
	CancelAimInputAndState();

	// 더 이상 움직이지 못하게 막음.
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 입력도 끊어서 죽은 뒤 조작이 남지 않게 함.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// 실제 사망 애니메이션, dissolve, 컷신, 카메라 연출은 BP에서 처리.
	BP_OnDeath();
}

float ADreamCatcherCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                         AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HealthComponent)
	{
		return 0.0f;
	}

	const float AppliedDamage = HealthComponent->ApplyDamage(DamageAmount, DamageCauser);
	if (AppliedDamage > 0.0f)
	{
		BP_OnDamaged(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaxHealth(), DamageCauser);
	}

	return AppliedDamage;
}

// 조준 상태별 입력 함수
void ADreamCatcherCharacter::AimPressed()
{
	if (!CombatComponent || !CombatComponent->IsAimAllowed())
	{
		return;
	}

	const EDCAimMode CurrentAimMode =
		CombatComponent->GetAimMode();

	// Scope 상태에서는 우클릭을 누르는 즉시 Hip으로 돌아감.
	if (CurrentAimMode == EDCAimMode::Scope)
	{
		GetWorldTimerManager().ClearTimer(AimHoldTimerHandle);

		bAimInputPressed = false;
		bShoulderAimActivated = false;

		CombatComponent->ClearAimState();
		return;
	}

	// 짧은 클릭과 긴 입력을 구분하는 판정은 Hip 상태에서만.
	if (CurrentAimMode != EDCAimMode::Hip)
	{
		return;
	}

	bAimInputPressed = true;
	bShoulderAimActivated = false;

	if (AimHoldThreshold <= 0.0f)
	{
		ActivateShoulderAim();
		return;
	}

	// 설정된 시간 동안 우클릭을 유지하면 Shoulder로 전환.
	GetWorldTimerManager().SetTimer(
		AimHoldTimerHandle,
		this,
		&ADreamCatcherCharacter::ActivateShoulderAim,
		AimHoldThreshold,
		false
	);
}

void ADreamCatcherCharacter::AimReleased()
{
	// Scope 상태에서 우클릭을 누른 경우에는 AimPressed()에서
	// 이미 bAimInputPressed를 false로 만들었으므로 여기서 뭐 하지 않음.
	if (!bAimInputPressed)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AimHoldTimerHandle);

	const bool bWasShoulderAim = bShoulderAimActivated;

	bAimInputPressed = false;
	bShoulderAimActivated = false;

	if (!CombatComponent)
	{
		return;
	}

	if (bWasShoulderAim)
	{
		// Shoulder는 우클릭을 떼면 항상 Hip으로.
		CombatComponent->EndShoulderAim();
	}
	else if (CombatComponent->GetAimMode() == EDCAimMode::Hip)
	{
		// Shoulder 진입 전에 버튼을 뗐다면 짧은 클릭이므로 Scope로 전환합니다.
		CombatComponent->ToggleScopeAim();
	}
}

void ADreamCatcherCharacter::AimCanceled()
{
	CancelAimInputAndState();
}

void ADreamCatcherCharacter::ActivateShoulderAim()
{
	if (!bAimInputPressed || !CombatComponent)
	{
		return;
	}

	bShoulderAimActivated = true;
	CombatComponent->BeginShoulderAim();
}

void ADreamCatcherCharacter::CancelAimInputAndState()
{
	GetWorldTimerManager().ClearTimer(AimHoldTimerHandle);

	bAimInputPressed = false;
	bShoulderAimActivated = false;

	if (CombatComponent)
	{
		CombatComponent->ClearAimState();
	}
}

// 카메라와 감도 보간
const FDCAimCameraProfile& ADreamCatcherCharacter::GetAimCameraProfile(EDCAimMode AimMode) const
{
	switch (AimMode)
	{
	case EDCAimMode::Shoulder:
		return ShoulderCameraProfile;

	case EDCAimMode::Scope:
		return ScopeCameraProfile;

	case EDCAimMode::Hip:
	default:
		return HipCameraProfile;
	}
}

void ADreamCatcherCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const EDCAimMode AimMode = CombatComponent
		                           ? CombatComponent->GetAimMode()
		                           : EDCAimMode::Hip;

	const FDCAimCameraProfile& Profile = GetAimCameraProfile(AimMode);

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		Profile.TargetArmLength,
		DeltaSeconds,
		AimCameraBlendSpeed
	);

	CameraBoom->SocketOffset = FMath::VInterpTo(
		CameraBoom->SocketOffset,
		Profile.SocketOffset,
		DeltaSeconds,
		AimCameraBlendSpeed
	);

	const float NewFieldOfView = FMath::FInterpTo(
		FollowCamera->FieldOfView,
		Profile.FieldOfView,
		DeltaSeconds,
		AimCameraBlendSpeed
	);

	FollowCamera->SetFieldOfView(NewFieldOfView);

	CurrentLookSensitivityMultiplier = FMath::FInterpTo(
		CurrentLookSensitivityMultiplier,
		Profile.LookSensitivityMultiplier,
		DeltaSeconds,
		AimCameraBlendSpeed
	);
	
	UpdateCameraRecoil(DeltaSeconds);
}

void ADreamCatcherCharacter::UpdateCameraRecoil(float DeltaSeconds)
{
	if (!Controller)
	{
		PendingRecoil = FVector2d::ZeroVector;
		return;
	}
	
	if (PendingRecoil.IsNearlyZero())
	{
		return;
	}

	// 남아 있는 반동량의 일부만 이번 프레임에 적용, PendingRecoil은 아직 소비하지 않은 회전 입력.
	const float ApplyAlpha = 1.0f - FMath::Exp(
		-RecoilKickInterpSpeed * DeltaSeconds
	);

	const FVector2D AppliedRecoil =
		PendingRecoil * ApplyAlpha;

	PendingRecoil -= AppliedRecoil;

	// 극히 작은 값이 무한히 남지 않도록 정리합니다.
	if (PendingRecoil.SizeSquared() < 0.000001f)
	{
		PendingRecoil = FVector2D::ZeroVector;
	}

	FRotator NewControlRotation =
		Controller->GetControlRotation();

	NewControlRotation.Pitch += AppliedRecoil.X;
	NewControlRotation.Yaw += AppliedRecoil.Y;

	Controller->SetControlRotation(NewControlRotation);
}

void ADreamCatcherCharacter::HandleAimModeChanged(EDCAimMode NewAimMode)
{
	BP_OnAimModeChanged(NewAimMode);
}

void ADreamCatcherCharacter::UnPossessed()
{
	CancelAimInputAndState();
	Super::UnPossessed();
}
