// Copyright Epic Games, Inc. All Rights Reserved.

#include "DreamCatcherCharacter.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Camera/DCCameraComponent.h"
#include "Camera/DCCameraMode.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Character/DCHeroComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Character/DCCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DreamCatcher.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ADreamCatcherCharacter::ADreamCatcherCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDCCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
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

	// CameraMode Stack을 사용할 새 카메라.
	// CameraMode가 직접 World Transform을 계산하므로 SpringArm 끝이 아니라 RootComponent에 부착.
	ModeCamera = CreateDefaultSubobject<UDCCameraComponent>(TEXT("ModeCamera"));

	ModeCamera->SetupAttachment(RootComponent);

	// CameraMode가 Controller 회전을 읽어 직접 View를 계산하므로
	// CameraComponent 자체가 PawnControlRotation을 중복 적용하지 않게 함.
	ModeCamera->bUsePawnControlRotation = false;

	// Activate when the Experience supplies PawnData and Hero binds the camera delegate.
	ModeCamera->SetAutoActivate(false);

	// PawnExtension owns ASC initialization; health observes the same lifetime.
	PawnExtensionComponent = CreateDefaultSubobject<UDCPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	// Ported from LyraCharacter: react to the extension's ASC lifetime rather than initialize it here.
	PawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtensionComponent->OnAbilitySystemUninitialized_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	HealthComponent = CreateDefaultSubobject<UDCLyraHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);
	// Keep ACharacter's native tick; the removed override only added legacy recoil/camera work.
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* ADreamCatcherCharacter::GetAbilitySystemComponent() const
{
	return GetDCAbilitySystemComponent();
}

UDCAbilitySystemComponent* ADreamCatcherCharacter::GetDCAbilitySystemComponent() const
{
	return PawnExtensionComponent ? PawnExtensionComponent->GetDCAbilitySystemComponent() : nullptr;
}

void ADreamCatcherCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtensionComponent->HandleControllerChanged();
}

void ADreamCatcherCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	PawnExtensionComponent->HandleControllerChanged();
}

void ADreamCatcherCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtensionComponent->HandlePlayerStateReplicated();
}

void ADreamCatcherCharacter::OnAbilitySystemInitialized()
{
	UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent();
	check(ASC);

	// The PlayerState ASC can outlive its previous pawn's crouching state.
	if (ASC->GetAvatarActor() == this)
	{
		ASC->SetLooseGameplayTagCount(DCGameplayTags::Status_Crouching, IsCrouched() ? 1 : 0);
	}

	HealthComponent->InitializeWithAbilitySystem(ASC);

	InitializeGASAimStateListeners();
	InitializeCameraModeSystem();
}

void ADreamCatcherCharacter::OnAbilitySystemUninitialized()
{
	if (UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent())
	{
		// PawnExtension clears the avatar before this callback. Do not touch a replacement avatar's tags.
		const AActor* Avatar = ASC->GetAvatarActor();
		if (!Avatar || Avatar == this)
		{
			ASC->SetLooseGameplayTagCount(DCGameplayTags::Status_Crouching, 0);
		}
	}

	HealthComponent->UninitializeFromAbilitySystem();

	UninitializeGASAimStateListeners();
}

void ADreamCatcherCharacter::InitializeGASAimStateListeners()
{
	UDCAbilitySystemComponent* AbilitySystemComponent = GetDCAbilitySystemComponent();

	if (!AbilitySystemComponent)
	{
		return;
	}

	if (BoundAimAbilitySystemComponent.Get() == AbilitySystemComponent)
	{
		RefreshGASAimMode(true);
		return;
	}

	UninitializeGASAimStateListeners();

	BoundAimAbilitySystemComponent = AbilitySystemComponent;

	ShoulderAimTagDelegateHandle =
		AbilitySystemComponent->
		RegisterGameplayTagEvent(DCGameplayTags::State_Aim_Shoulder, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGASAimStateTagChanged);

	ScopeAimTagDelegateHandle =
		AbilitySystemComponent->
		RegisterGameplayTagEvent(DCGameplayTags::State_Aim_Scope, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGASAimStateTagChanged);

	RefreshGASAimMode(true);
}

void ADreamCatcherCharacter::UninitializeGASAimStateListeners()
{
	UDCAbilitySystemComponent* AbilitySystemComponent = BoundAimAbilitySystemComponent.Get();

	if (AbilitySystemComponent)
	{
		if (ShoulderAimTagDelegateHandle.IsValid())
		{
			AbilitySystemComponent->UnregisterGameplayTagEvent(
				ShoulderAimTagDelegateHandle,
				DCGameplayTags::State_Aim_Shoulder,
				EGameplayTagEventType::NewOrRemoved
			);
		}

		if (ScopeAimTagDelegateHandle.IsValid())
		{
			AbilitySystemComponent->UnregisterGameplayTagEvent(
				ScopeAimTagDelegateHandle,
				DCGameplayTags::State_Aim_Scope,
				EGameplayTagEventType::NewOrRemoved
			);
		}
	}

	ShoulderAimTagDelegateHandle = FDelegateHandle();

	ScopeAimTagDelegateHandle = FDelegateHandle();

	BoundAimAbilitySystemComponent.Reset();

	CurrentGASAimMode = EDCAimMode::Hip;
}

void ADreamCatcherCharacter::HandleGASAimStateTagChanged(const FGameplayTag ChangedTag, int32 NewCount)
{
	(void)ChangedTag;
	(void)NewCount;

	RefreshGASAimMode(false);
}

void ADreamCatcherCharacter::RefreshGASAimMode(bool bForceBroadcast)
{
	const UDCAbilitySystemComponent* AbilitySystemComponent = BoundAimAbilitySystemComponent.Get();

	EDCAimMode NewAimMode = EDCAimMode::Hip;

	if (AbilitySystemComponent)
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope))
		{
			NewAimMode = EDCAimMode::Scope;
		}
		else if (AbilitySystemComponent->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Shoulder))
		{
			NewAimMode = EDCAimMode::Shoulder;
		}
	}

	if (!bForceBroadcast && CurrentGASAimMode == NewAimMode)
	{
		return;
	}

	CurrentGASAimMode = NewAimMode;

	OnGASAimModeChanged.Broadcast(CurrentGASAimMode);

	BP_OnAimModeChanged(CurrentGASAimMode);
}

void ADreamCatcherCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	if (IsPlayerControlled() && HealthComponent)
	{
		// Reuse Lyra's fall-death effect so armor/immunity cannot bypass death,
		// and the existing death ability and stage-failure flow run normally.
		if (HasAuthority())
		{
			HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
		}
		return;
	}

	// Inspector bots share this character base; preserve their engine behavior.
	Super::FellOutOfWorld(DamageType);
}

void ADreamCatcherCharacter::OnDeathFinished(AActor*)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
	UE_LOG(LogDreamCatcher, Log, TEXT("[R3-1] Death finished: [%s]. Avatar cleanup queued."), *GetNameSafe(this));
}

void ADreamCatcherCharacter::DisableMovementAndCollision()
{
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UDCCharacterMovementComponent* DCMoveComp = CastChecked<UDCCharacterMovementComponent>(GetCharacterMovement());
	DCMoveComp->StopMovementImmediately();
	DCMoveComp->DisableMovement();
}

void ADreamCatcherCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}

void ADreamCatcherCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	// A replacement pawn may already own the PlayerState ASC. Never clear that pawn's actor info.
	if (UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent())
	{
		if (ASC->GetAvatarActor() == this)
		{
			PawnExtensionComponent->UninitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}

void ADreamCatcherCharacter::ToggleCrouch()
{
	const UDCCharacterMovementComponent* DCMoveComp =
		CastChecked<UDCCharacterMovementComponent>(GetCharacterMovement());

	if (IsCrouched() || DCMoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (DCMoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void ADreamCatcherCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UDCAbilitySystemComponent* DCASC = GetDCAbilitySystemComponent())
	{
		DCASC->SetLooseGameplayTagCount(DCGameplayTags::Status_Crouching, 1);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void ADreamCatcherCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UDCAbilitySystemComponent* DCASC = GetDCAbilitySystemComponent())
	{
		DCASC->SetLooseGameplayTagCount(DCGameplayTags::Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool ADreamCatcherCharacter::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}

void ADreamCatcherCharacter::InitializeCameraModeSystem()
{
	const UDCPawnData* PawnData = PawnExtensionComponent->GetPawnData();
	if (PawnData && PawnData->DefaultCameraMode)
	{
		// Hero owns the original camera selection delegate.
		ModeCamera->Activate(true);
	}
	else
	{
		ModeCamera->Deactivate();
	}
}

void ADreamCatcherCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (PawnExtensionComponent->GetPawnData())
	{
		ensureMsgf(UDCHeroComponent::FindHeroComponent(this), TEXT("PawnData requires a Hero component."));
	}
	InitializeCameraModeSystem();
}

void ADreamCatcherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PawnExtensionComponent->SetupPlayerInputComponent();
}

void ADreamCatcherCharacter::OnDeathStarted(AActor*)
{
	if (UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent())
	{
		ASC->CancelAimInputAndState();
	}
	DisableMovementAndCollision();
}

void ADreamCatcherCharacter::UnPossessed()
{
	if (UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent())
	{
		ASC->CancelAimInputAndState();
	}
	Super::UnPossessed();
	PawnExtensionComponent->HandleControllerChanged();
}

float ADreamCatcherCharacter::GetCurrentLookSensitivityMultiplier() const
{
	return ModeCamera->IsActive() ? ModeCamera->GetCurrentLookSensitivityMultiplier() : 1.0f;
}
