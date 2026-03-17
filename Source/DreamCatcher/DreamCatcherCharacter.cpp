#include "DreamCatcherCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DCCombatComponent.h"
#include "Components/DCHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DreamCatcher.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

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
}

void ADreamCatcherCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay 시점에는 컴포넌트와 런타임 객체들이 모두 살아 있으므로
	// 이벤트 바인딩을 여기서 해두는 것이 안전.
	if (CombatComponent)
	{
		CombatComponent->OnPrimaryFireRequested.AddDynamic(this, &ADreamCatcherCharacter::HandlePrimaryFireRequested);
		CombatComponent->OnDodgeRequested.AddDynamic(this, &ADreamCatcherCharacter::HandleDodgeRequested);
		CombatComponent->OnUltimateRequested.AddDynamic(this, &ADreamCatcherCharacter::HandleUltimateRequested);
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
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
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
	if (CombatComponent)
	{
		CombatComponent->TryDodge();
	}
}

void ADreamCatcherCharacter::Ultimate()
{
	if (CombatComponent)
	{
		CombatComponent->TryUltimate();
	}
}

void ADreamCatcherCharacter::HandlePrimaryFireRequested()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 슈터에서는 "카메라 기준 조준점"과 "총구 위치"를 분리해서 생각하는 것이 중요.
	// 먼저 카메라에서 쏘는 가상의 조준 레이를 계산.
	FVector ViewLocation = FollowCamera->GetComponentLocation();
	FRotator ViewRotation = FollowCamera->GetComponentRotation();

	if (Controller)
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * FireTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerFireTrace), false, this);
	Params.AddIgnoredActor(this);

	World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params);

	// 크로스헤어가 가리키는 실제 목표 지점.
	const FVector AimPoint = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	// 실제 총알/이펙트는 총구에서 시작해야 하므로 총구 소켓 위치를 구함.
	FVector MuzzleLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f);
	if (USkeletalMeshComponent* CharacterMesh = GetMesh(); CharacterMesh && CharacterMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = CharacterMesh->GetSocketLocation(MuzzleSocketName);
	}

	// C++는 계산만 하고, 블루프린트가 여기서 탄 생성/VFX/사운드를 처리.
	BP_OnPrimaryFireRequested(MuzzleLocation, AimPoint);
}

void ADreamCatcherCharacter::HandleDodgeRequested()
{
	// 마지막 이동 입력 방향으로 회피, 입력이 없으면 정면 회피로 처리.
	FVector DodgeDirection = GetLastMovementInputVector().GetSafeNormal();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector();
	}

	// LaunchCharacter는 가장 빠르게 회피 프로토타입을 만들 수 있는 방법.
	LaunchCharacter((DodgeDirection * DodgeImpulse) + (FVector::UpVector * DodgeLiftImpulse), true, true);

	// 회피 몽타주, 잔상, 무적 프레임 표시 등은 블루프린트에서.
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

float ADreamCatcherCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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