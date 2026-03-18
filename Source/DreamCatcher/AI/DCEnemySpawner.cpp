#include "AI/DCEnemySpawner.h"
#include "AI/DCEnemyCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DreamCatcher.h"

ADCEnemySpawner::ADCEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);
}

void ADCEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnBeginPlay)
	{
		ActivateSpawner();
	}
}

void ADCEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ADCEnemySpawner::ActivateSpawner()
{
	if (bIsActive || bIsCleared)
	{
		return;
	}

	bIsActive = true;
	TrySpawnNext();
}

void ADCEnemySpawner::TrySpawnNext()
{
	if (!bIsActive || bIsCleared)
	{
		return;
	}

	// EnemyClass 누락 시 인카운터가 멈추지 않게 즉시 종료 처리.
	if (!EnemyClass)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("%s has no EnemyClass assigned. Completing spawner immediately."), *GetName());
		CompleteSpawner();
		return;
	}

	if (AliveCount >= MaxAliveAtOnce)
	{
		return;
	}

	if (SpawnedCount >= SpawnCount)
	{
		if (AliveCount == 0)
		{
			CompleteSpawner();
		}
		return;
	}

	SpawnEnemy();

	// 아직 남은 적이 있고 동시에 더 유지할 수 있으면 다음 스폰을 예약.
	if (SpawnedCount < SpawnCount && AliveCount < MaxAliveAtOnce)
	{
		ScheduleNextSpawn();
	}
}

void ADCEnemySpawner::SpawnEnemy()
{
	if (!EnemyClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ADCEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<ADCEnemyCharacter>(EnemyClass, SpawnPoint->GetComponentTransform(), SpawnParams);
	if (!SpawnedEnemy)
	{
		return;
	}

	SpawnedEnemy->OnEnemyDeath.AddDynamic(this, &ADCEnemySpawner::HandleSpawnedEnemyDeath);

	++SpawnedCount;
	++AliveCount;
}

void ADCEnemySpawner::ScheduleNextSpawn()
{
	if (!bIsActive || bIsCleared || SpawnedCount >= SpawnCount || AliveCount >= MaxAliveAtOnce)
	{
		return;
	}

	const float Delay = SpawnInterval > 0.0f ? SpawnInterval : 0.01f;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ADCEnemySpawner::TrySpawnNext, Delay, false);
}

void ADCEnemySpawner::HandleSpawnedEnemyDeath(ADCEnemyCharacter* DeadEnemy)
{
	if (DeadEnemy)
	{
		DeadEnemy->OnEnemyDeath.RemoveDynamic(this, &ADCEnemySpawner::HandleSpawnedEnemyDeath);
	}

	AliveCount = FMath::Max(0, AliveCount - 1);

	if (SpawnedCount >= SpawnCount && AliveCount == 0)
	{
		CompleteSpawner();
		return;
	}

	ScheduleNextSpawn();
}

void ADCEnemySpawner::CompleteSpawner()
{
	if (bIsCleared)
	{
		return;
	}

	bIsCleared = true;
	OnSpawnerCleared.Broadcast(this);
}