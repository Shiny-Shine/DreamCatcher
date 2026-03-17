// Copyright Epic Games, Inc. All Rights Reserved.

#include "DreamCatcherGameMode.h"
#include "DreamCatcherCharacter.h"
#include "Components/DCHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADreamCatcherGameMode::ADreamCatcherGameMode()
{
}

void ADreamCatcherGameMode::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay 시점에 플레이어가 아직 준비되지 않았을 수 있으므로 별도 바인딩 함수를 둠.
	BindPlayerDeath();
}

void ADreamCatcherGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BindPlayerTimerHandle);
	GetWorldTimerManager().ClearTimer(RestartLevelTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ADreamCatcherGameMode::BindPlayerDeath()
{
	if (bPlayerDeathBound)
	{
		return;
	}

	ADreamCatcherCharacter* PlayerCharacter = Cast<ADreamCatcherCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter || !PlayerCharacter->GetHealthComponent())
	{
		GetWorldTimerManager().SetTimer(BindPlayerTimerHandle, this, &ADreamCatcherGameMode::BindPlayerDeath, 0.1f, false);
		return;
	}

	PlayerCharacter->GetHealthComponent()->OnDeath.RemoveDynamic(this, &ADreamCatcherGameMode::HandlePlayerDeath);
	PlayerCharacter->GetHealthComponent()->OnDeath.AddDynamic(this, &ADreamCatcherGameMode::HandlePlayerDeath);
	bPlayerDeathBound = true;
}

void ADreamCatcherGameMode::HandlePlayerDeath(AActor* DeadActor)
{
	if (bRestartQueued)
	{
		return;
	}

	bRestartQueued = true;
	GetWorldTimerManager().SetTimer(RestartLevelTimerHandle, this, &ADreamCatcherGameMode::RestartCurrentLevel, RestartLevelDelay, false);
}

void ADreamCatcherGameMode::RestartCurrentLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}