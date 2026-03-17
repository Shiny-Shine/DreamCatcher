#include "AI/DCEnemyCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/DCHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADCEnemyCharacter::ADCEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UDCHealthComponent>(TEXT("HealthComponent"));

	// 기본 AIController 사용. 첫 슬라이스에서는 별도 AIController 클래스를 만들지 않음.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

void ADCEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	CurrentTarget = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ADCEnemyCharacter::HandleDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &ADCEnemyCharacter::HandleHealthChanged);
	}

	// 이동 경로 요청은 너무 자주 하지 않고 짧은 간격 타이머로 갱신.
	GetWorldTimerManager().SetTimer(MovementTimerHandle, this, &ADCEnemyCharacter::UpdateMovement, 0.25f, true);
}

void ADCEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead || !CurrentTarget.IsValid())
	{
		return;
	}

	// 항상 플레이어를 바라보게 해서 전투 가독성 향상.
	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		const FRotator FacingRotation = ToTarget.Rotation();
		SetActorRotation(FRotator(0.0f, FacingRotation.Yaw, 0.0f));
	}

	if (ToTarget.SizeSquared() <= FMath::Square(AttackRange))
	{
		TryFireAtTarget();
	}
}

void ADCEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MovementTimerHandle);
	Super::EndPlay(EndPlayReason);
}

float ADCEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HealthComponent)
	{
		return 0.0f;
	}

	return HealthComponent->ApplyDamage(DamageAmount, DamageCauser);
}

void ADCEnemyCharacter::UpdateMovement()
{
	if (bIsDead || !CurrentTarget.IsValid())
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceToTarget > MoveAcceptanceRadius)
	{
		AIController->MoveToActor(CurrentTarget.Get(), MoveAcceptanceRadius);
	}
	else
	{
		AIController->StopMovement();
	}
}

void ADCEnemyCharacter::TryFireAtTarget()
{
	if (!CurrentTarget.IsValid() || bIsDead)
	{
		return;
	}

	if (GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}

	NextAttackTime = GetWorld()->GetTimeSeconds() + AttackInterval;

	FVector FireStart = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	if (USkeletalMeshComponent* CharacterMesh = GetMesh(); CharacterMesh && CharacterMesh->DoesSocketExist(MuzzleSocketName))
	{
		FireStart = CharacterMesh->GetSocketLocation(MuzzleSocketName);
	}

	const FVector TargetPoint = CurrentTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyFireTrace), false, this);
	Params.AddIgnoredActor(this);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, FireStart, TargetPoint, ECC_Visibility, Params);
	const FVector FireEnd = bBlocked ? Hit.ImpactPoint : TargetPoint;
	AActor* HitActor = bBlocked ? Hit.GetActor() : CurrentTarget.Get();

	// 막히지 않았거나, 실제로 플레이어를 맞췄을 때만 데미지를 줌.
	if (!bBlocked || HitActor == CurrentTarget.Get())
	{
		UGameplayStatics::ApplyDamage(CurrentTarget.Get(), AttackDamage, GetController(), this, UDamageType::StaticClass());
	}

	BP_OnAttackShot(FireStart, FireEnd, HitActor);
}

void ADCEnemyCharacter::HandleDeath(AActor* DeadActor)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	GetWorldTimerManager().ClearTimer(MovementTimerHandle);

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BP_OnDeath();
	OnEnemyDeath.Broadcast(this);
	SetLifeSpan(DestroyDelay);
}

void ADCEnemyCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	BP_OnDamaged(CurrentHealth / FMath::Max(1.0f, MaxHealth));
}