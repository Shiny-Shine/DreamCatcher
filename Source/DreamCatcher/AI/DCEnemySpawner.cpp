#include "AI/DCEnemySpawner.h"
#include "AI/DCGASEnemyCharacter.h"
#include "AbilitySystem/Abilities/DCLyraGameplayAbility_Death.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Lyra/DCLyraExperienceManagerComponent.h"
#include "GameModes/Lyra/DCLyraGameMode.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "TimerManager.h"
#include "DreamCatcher.h"
#include "AIController.h"

ADCEnemySpawner::ADCEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);
}

int32 ADCEnemySpawner::GetPendingLyraCleanupCount() const
{
	int32 Count = 0;
	for (const auto& Entry : LyraEnemies)
	{
		if (!LivingEnemies.Contains(Entry.Key)) { ++Count; }
	}
	return Count;
}

int32 ADCEnemySpawner::GetPlannedEnemyCount() const
{
	return GASEnemyClass || LyraBotControllerClass ? FMath::Max(0, SpawnCount) : 0;
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
	bEndingPlay = true;
	bIsActive = false;
	bWaitingForLyraExperience = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	CleanupOwnedLyraBots();
	CleanupOwnedGASNPCs();
	for (const TWeakObjectPtr<AActor>& Enemy : LivingEnemies) { UnbindEnemy(Enemy.Get()); }
	LivingEnemies.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADCEnemySpawner::ActivateSpawner()
{
	if (bIsActive || bIsCleared || bSpawnFailed || bEndingPlay)
	{
		return;
	}

	// GAS NPCs and finite Lyra bots are authority-owned.
	if ((GASEnemyClass || LyraBotControllerClass) && (!HasAuthority() || GetNetMode() == NM_Client)) { return; }
	if ((GASEnemyClass || LyraBotControllerClass) && (SpawnCount <= 0 || MaxAliveAtOnce <= 0 || !FMath::IsFinite(SpawnInterval) || SpawnInterval < 0.0f))
	{
		FailSpawner(TEXT("invalid GAS/Lyra spawn count, concurrency, or interval"));
		return;
	}
	bIsActive = true;
	if (LyraBotControllerClass)
	{
		AGameStateBase* GameState = GetWorld()->GetGameState();
		UDCLyraExperienceManagerComponent* Experience = GameState ? GameState->FindComponentByClass<UDCLyraExperienceManagerComponent>() : nullptr;
		if (!GetWorld()->GetAuthGameMode<ADCLyraGameMode>() || !Experience)
		{
			FailSpawner(TEXT("finite Lyra bots require the Lyra GameMode and Experience"));
			return;
		}
		bWaitingForLyraExperience = true;
		Experience->CallOrRegister_OnExperienceLoaded_LowPriority(
			FOnDCLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnLyraExperienceLoaded));
		return;
	}
	TrySpawnNext();
}

void ADCEnemySpawner::OnLyraExperienceLoaded(const UDCLyraExperienceDefinition*)
{
	if (bEndingPlay || !bIsActive || bSpawnFailed || !bWaitingForLyraExperience) { return; }
	bWaitingForLyraExperience = false;
	// Let every Experience callback (including team setup) finish before spawning.
	SpawnTimerHandle = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TrySpawnNext);
}

void ADCEnemySpawner::TrySpawnNext()
{
	if (!bIsActive || bIsCleared || bSpawnFailed || bEndingPlay || bSpawningEnemy || bWaitingForLyraExperience)
	{
		return;
	}

	// An unconfigured optional spawner retains its existing immediate-completion behavior.
	if (!GASEnemyClass && !LyraBotControllerClass)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("%s has no GAS or Lyra spawn class assigned. Completing spawner immediately."), *GetName());
		CompleteSpawner();
		return;
	}

	if (GetAliveCount() >= MaxAliveAtOnce)
	{
		return;
	}

	if (SpawnedCount >= SpawnCount)
	{
		if (GetAliveCount() == 0)
		{
			CompleteSpawner();
		}
		return;
	}

	SpawnEnemy();
	if (!bIsActive || bEndingPlay || bSpawnFailed) { return; }
	// A Death callback can run during the deferred actor's BeginPlay.
	if (SpawnedCount >= SpawnCount && GetAliveCount() == 0)
	{
		// Do not synchronously finish an Encounter while it is still registering spawners.
		SpawnTimerHandle = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TrySpawnNext);
		return;
	}

	// 아직 남은 적이 있고 동시에 더 유지할 수 있으면 다음 스폰을 예약.
	if (SpawnedCount < SpawnCount && GetAliveCount() < MaxAliveAtOnce)
	{
		ScheduleNextSpawn();
	}
}

void ADCEnemySpawner::SpawnEnemy()
{
	if (LyraBotControllerClass)
	{
		TGuardValue<bool> SpawnGuard(bSpawningEnemy, true);
		SpawnLyraEnemy();
		return;
	}
	if (!GASEnemyClass)
	{
		return;
	}

	TGuardValue<bool> SpawnGuard(bSpawningEnemy, true);
	const FTransform Transform = SpawnPoint->GetComponentTransform();
	UClass* SpawnClass = GASEnemyClass.Get();
	AActor* SpawnedEnemy = GetWorld()->SpawnActorDeferred<AActor>(SpawnClass, Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!SpawnedEnemy)
	{
		if (GASEnemyClass) { FailSpawner(TEXT("GAS SpawnActor failed")); }
		return;
	}
	// Bind/count before construction/BeginPlay so an immediate death cannot be missed.
	TrackEnemy(SpawnedEnemy);
	SpawnedEnemy->FinishSpawning(Transform);
	if (IsValid(SpawnedEnemy) && OwnedGASNPCs.Contains(SpawnedEnemy))
	{
		if (APawn* Pawn = Cast<APawn>(SpawnedEnemy))
		{
			if (AAIController* Controller = Cast<AAIController>(Pawn->GetController()))
			{
				OwnedGASNPCs[SpawnedEnemy] = Controller;
			}
		}
	}
	if (bEndingPlay || bSpawnFailed || !IsValid(SpawnedEnemy) || SpawnedEnemy->IsActorBeingDestroyed()) { return; }
	if (ADCGASEnemyCharacter* GASEnemy = Cast<ADCGASEnemyCharacter>(SpawnedEnemy))
	{
		if (!GASEnemy->IsGASReady() && !GASEnemy->IsDeadOrDying())
		{
			// A broken setup is not a kill/clear. Remove only the unusable actor we created.
			FailSpawner(TEXT("GAS enemy did not initialize"));
			LivingEnemies.Remove(TWeakObjectPtr<AActor>(SpawnedEnemy));
			UnbindEnemy(SpawnedEnemy);
			if (IsValid(SpawnedEnemy)) { SpawnedEnemy->Destroy(); }
		}
	}
}

void ADCEnemySpawner::SpawnLyraEnemy()
{
	ADCLyraGameMode* GameMode = GetWorld()->GetAuthGameMode<ADCLyraGameMode>();
	if (!HasAuthority() || !GameMode) { FailSpawner(TEXT("Lyra GameMode unavailable")); return; }

	// Original BotCreation ordering. The finite spawner owns this controller, not SpawnedBotList.
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;
	ADCLyraPlayerBotController* Controller = GetWorld()->SpawnActor<ADCLyraPlayerBotController>(
		LyraBotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (!Controller) { FailSpawner(TEXT("Lyra controller creation failed")); return; }
	OwnedLyraControllers.Add(Controller);
	Controller->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleLyraControllerDestroyed);
	if (!Controller->PlayerState)
	{
		ReleaseLyraController(Controller);
		FailSpawner(TEXT("Lyra controller has no PlayerState"));
		return;
	}
	Controller->PlayerState->SetPlayerName(FString::Printf(TEXT("%s_%d"), *GetName(), SpawnedCount + 1));
	GameMode->GenericPlayerInitialization(Controller);
	if (bEndingPlay || bSpawnFailed || !IsValid(Controller)) { ReleaseLyraController(Controller); return; }
	// Keep the original GameMode/PawnData/Possess path, but honor the finite spawn point.
	GameMode->RestartPlayerAtTransform(Controller, SpawnPoint->GetComponentTransform());
	APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	if (bEndingPlay || bSpawnFailed || !IsValid(Pawn) || Pawn->IsActorBeingDestroyed())
	{
		if (IsValid(Pawn)) { Pawn->Destroy(); }
		ReleaseLyraController(Controller); // also cancels a failed GameMode restart queued on this controller
		if (!bEndingPlay && !bSpawnFailed) { FailSpawner(TEXT("Lyra pawn creation/possession failed")); }
		return;
	}
	UDCPawnExtensionComponent* Extension = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	if (Extension) { Extension->CheckDefaultInitialization(); }
	if (bEndingPlay || bSpawnFailed || !IsValid(Controller) || !IsValid(Pawn)
		|| Pawn->IsActorBeingDestroyed() || Controller->GetPawn() != Pawn)
	{
		if (IsValid(Pawn) && !Pawn->IsActorBeingDestroyed()) { Pawn->Destroy(); }
		ReleaseLyraController(Controller);
		if (!bEndingPlay && !bSpawnFailed) { FailSpawner(TEXT("Lyra pawn removed during initialization")); }
		return;
	}
	UDCAbilitySystemComponent* ASC = Extension ? Extension->GetDCAbilitySystemComponent() : nullptr;
	bool bHasDeathAbility = false;
	if (ASC)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->IsA<UDCLyraGameplayAbility_Death>()) { bHasDeathAbility = true; break; }
		}
	}
	if (!Extension || !Health || (!Health->IsDeadOrDying() && (!ASC || !bHasDeathAbility || Health->GetMaxHealth() <= 0.0f)))
	{
		Pawn->Destroy();
		ReleaseLyraController(Controller);
		FailSpawner(TEXT("Lyra pawn health/ability initialization failed"));
		return;
	}
	TrackLyraEnemy(Pawn, Controller);
	// BeginPlay/Possess may have synchronously started death before delegates could be bound.
	if (Health->IsDeadOrDying()) { HandleLyraDeathStarted(Pawn); }
	if (Health->GetDeathState() == EDCLyraDeathState::DeathFinished) { HandleLyraDeathFinished(Pawn); }
}

void ADCEnemySpawner::TrackLyraEnemy(AActor* Enemy, ADCLyraPlayerBotController* Controller)
{
	check(Enemy && Controller && !LyraEnemies.Contains(Enemy));
	LyraEnemies.Add(Enemy, Controller);
	LivingEnemies.Add(Enemy);
	++SpawnedCount;
	UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Enemy);
	check(Health);
	Health->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleLyraDeathStarted);
	Health->OnDeathFinished.AddUniqueDynamic(this, &ThisClass::HandleLyraDeathFinished);
	Enemy->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleTrackedEnemyDestroyed);
}

void ADCEnemySpawner::HandleLyraDeathStarted(AActor* Enemy)
{
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Enemy);
	if (LyraEnemies.Contains(Enemy) && Health && Health->IsDeadOrDying()) { ConsumeEnemyDeath(Enemy); }
}

void ADCEnemySpawner::HandleLyraDeathFinished(AActor* Enemy)
{
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Enemy);
	if (!bEndingPlay && LyraEnemies.Contains(Enemy) && Health && Health->GetDeathState() == EDCLyraDeathState::DeathFinished)
	{
		FinishedLyraDeaths.Add(Enemy);
		// Do not destroy the Controller/ASC inside Death Ability's EndAbility, or before
		// K2_OnDeathFinished clears Inventory/QuickBar. The original Pawn destroys itself.
	}
}

void ADCEnemySpawner::ReleaseLyraController(ADCLyraPlayerBotController* Controller)
{
	if (!Controller || OwnedLyraControllers.Remove(Controller) == 0) { return; }
	Controller->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleLyraControllerDestroyed);
	GetWorldTimerManager().ClearAllTimersForObject(Controller);
	if (IsValid(Controller) && !Controller->IsActorBeingDestroyed()) { Controller->Destroy(); }
}

void ADCEnemySpawner::RemoveLyraEnemy(AActor* Enemy, bool bDestroyPawn)
{
	TWeakObjectPtr<ADCLyraPlayerBotController> Controller;
	if (!Enemy || !LyraEnemies.RemoveAndCopyValue(Enemy, Controller)) { return; }
	LivingEnemies.Remove(Enemy);
	FinishedLyraDeaths.Remove(Enemy);
	UnbindEnemy(Enemy);
	if (bDestroyPawn && IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) { Enemy->Destroy(); }
	ReleaseLyraController(Controller.Get());
}

void ADCEnemySpawner::CleanupOwnedLyraBots()
{
	TArray<TWeakObjectPtr<AActor>> Pawns;
	LyraEnemies.GetKeys(Pawns);
	for (const TWeakObjectPtr<AActor>& Pawn : Pawns)
	{
		if (AActor* Enemy = Pawn.Get()) { UnbindEnemy(Enemy); }
	}
	LyraEnemies.Reset();
	FinishedLyraDeaths.Reset();
	const TArray<TWeakObjectPtr<ADCLyraPlayerBotController>> Controllers = OwnedLyraControllers.Array();
	for (const TWeakObjectPtr<ADCLyraPlayerBotController>& Controller : Controllers)
	{
		if (ADCLyraPlayerBotController* Bot = Controller.Get())
		{
			Bot->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleLyraControllerDestroyed);
			GetWorldTimerManager().ClearAllTimersForObject(Bot);
		}
	}
	OwnedLyraControllers.Reset();
	if (GetWorld()->bIsTearingDown || (Pawns.IsEmpty() && Controllers.IsEmpty())) { return; }
	// A clear listener may destroy this spawner inside DeathStarted. Do not destroy
	// the owning ASC in that ability's stack. No callback retains the ended spawner.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([Pawns, Controllers]()
	{
		for (const TWeakObjectPtr<AActor>& Pawn : Pawns)
		{
			if (AActor* Enemy = Pawn.Get(); Enemy && !Enemy->IsActorBeingDestroyed()) { Enemy->Destroy(); }
		}
		for (const TWeakObjectPtr<ADCLyraPlayerBotController>& Controller : Controllers)
		{
			if (ADCLyraPlayerBotController* Bot = Controller.Get(); Bot && !Bot->IsActorBeingDestroyed()) { Bot->Destroy(); }
		}
	}));
}

void ADCEnemySpawner::HandleLyraControllerDestroyed(AActor* Actor)
{
	ADCLyraPlayerBotController* Controller = Cast<ADCLyraPlayerBotController>(Actor);
	if (!Controller || OwnedLyraControllers.Remove(Controller) == 0) { return; }
	TArray<TWeakObjectPtr<AActor>> Pawns;
	LyraEnemies.GetKeys(Pawns);
	for (const TWeakObjectPtr<AActor>& Pawn : Pawns)
	{
		if (LyraEnemies.FindRef(Pawn) == Controller)
		{
			RemoveLyraEnemy(Pawn.Get(), true);
		}
	}
	if (!bEndingPlay && !GetWorld()->bIsTearingDown) { FailSpawner(TEXT("owned Lyra controller removed before pawn cleanup")); }
}

void ADCEnemySpawner::ScheduleNextSpawn()
{
	if (!bIsActive || bIsCleared || bSpawnFailed || bEndingPlay || bSpawningEnemy
		|| SpawnedCount >= SpawnCount || GetAliveCount() >= MaxAliveAtOnce)
	{
		return;
	}

	const float Delay = SpawnInterval > 0.0f ? SpawnInterval : 0.01f;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ADCEnemySpawner::TrySpawnNext, Delay, false);
}

void ADCEnemySpawner::HandleGASEnemyDeath(ADCGASEnemyCharacter* DeadEnemy)
{
	if (DeadEnemy && DeadEnemy->IsDeadOrDying()) { ConsumeEnemyDeath(DeadEnemy); }
}

void ADCEnemySpawner::TrackEnemy(AActor* Enemy)
{
	check(Enemy);
	LivingEnemies.Add(TWeakObjectPtr<AActor>(Enemy));
	++SpawnedCount;
	if (ADCGASEnemyCharacter* GAS = Cast<ADCGASEnemyCharacter>(Enemy))
	{
		GAS->OnEnemyDeath.AddUniqueDynamic(this, &ThisClass::HandleGASEnemyDeath);
		if (bCleanupOwnedGASNPCs)
		{
			OwnedGASNPCs.Add(Enemy, TWeakObjectPtr<AController>());
			GAS->GetHealthComponent()->OnDeathFinished.AddUniqueDynamic(this, &ThisClass::HandleOwnedGASDeathFinished);
		}
	}
	Enemy->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleTrackedEnemyDestroyed);
}

void ADCEnemySpawner::UnbindEnemy(AActor* Enemy)
{
	if (!Enemy) { return; }
	if (UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Enemy))
	{
		Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleLyraDeathStarted);
		Health->OnDeathFinished.RemoveDynamic(this, &ThisClass::HandleLyraDeathFinished);
		Health->OnDeathFinished.RemoveDynamic(this, &ThisClass::HandleOwnedGASDeathFinished);
	}
	Enemy->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleTrackedEnemyDestroyed);
	if (ADCGASEnemyCharacter* GAS = Cast<ADCGASEnemyCharacter>(Enemy))
	{
		GAS->OnEnemyDeath.RemoveDynamic(this, &ThisClass::HandleGASEnemyDeath);
	}
}

void ADCEnemySpawner::ConsumeEnemyDeath(AActor* Enemy)
{
	if (bEndingPlay || !Enemy || LivingEnemies.Remove(TWeakObjectPtr<AActor>(Enemy)) == 0) { return; }
	// Presentation counter only: duplicate death, destruction and abort cleanup
	// never reach this successful living-enemy removal guard.
	if (DefeatedCount < MAX_int32) { ++DefeatedCount; }
	// Retain Lyra DeathFinished/OnDestroyed bindings after counting the kill.
	if (!LyraEnemies.Contains(Enemy) && !OwnedGASNPCs.Contains(Enemy)) { UnbindEnemy(Enemy); }
	if (!bIsActive || bSpawnFailed || bSpawningEnemy) { return; }
	if (SpawnedCount >= SpawnCount && GetAliveCount() == 0)
	{
		CompleteSpawner();
		return;
	}

	ScheduleNextSpawn();
}

void ADCEnemySpawner::HandleTrackedEnemyDestroyed(AActor* Enemy)
{
	if (Enemy && OwnedGASNPCs.Contains(Enemy))
	{
		const bool bUnexpected = LivingEnemies.Contains(Enemy) || !FinishedGASDeaths.Contains(Enemy);
		const TWeakObjectPtr<AController> Controller = OwnedGASNPCs.FindRef(Enemy);
		OwnedGASNPCs.Remove(Enemy);
		FinishedGASDeaths.Remove(Enemy);
		LivingEnemies.Remove(Enemy);
		UnbindEnemy(Enemy);
		if (!GetWorld()->bIsTearingDown)
		{
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([Controller]()
			{
				if (AController* C = Controller.Get(); C && !C->GetPawn() && !C->IsActorBeingDestroyed()) { C->Destroy(); }
			}));
			if (bUnexpected && !bEndingPlay && !bSpawnFailed) { FailSpawner(TEXT("owned GAS NPC removed before death finished")); }
		}
		return;
	}
	if (Enemy && LyraEnemies.Contains(Enemy))
	{
		const bool bUnexpected = LivingEnemies.Contains(Enemy) || !FinishedLyraDeaths.Contains(Enemy);
		RemoveLyraEnemy(Enemy, false);
		if (bUnexpected && !bEndingPlay && !GetWorld()->bIsTearingDown)
		{
			FailSpawner(TEXT("Lyra pawn removed before its death finished"));
		}
		return;
	}
	if (!Enemy || LivingEnemies.Remove(TWeakObjectPtr<AActor>(Enemy)) == 0) { return; }
	UnbindEnemy(Enemy);
	if (!bEndingPlay && !GetWorld()->bIsTearingDown)
	{
		FailSpawner(TEXT("tracked enemy destroyed without a death notification"));
	}
}

void ADCEnemySpawner::FailSpawner(const TCHAR* Reason)
{
	if (bEndingPlay || bSpawnFailed) { return; }
	bSpawnFailed = true;
	bIsActive = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	CleanupOwnedGASNPCs();
	UE_LOG(LogDreamCatcher, Error, TEXT("[R10-1] Spawner aborted: %s (%s); not reporting cleared."), *GetName(), Reason);
	OnSpawnerFailed.Broadcast(this);
}

void ADCEnemySpawner::CompleteSpawner()
{
	if (bIsCleared || bEndingPlay || bSpawnFailed || bSpawningEnemy)
	{
		return;
	}

	bIsCleared = true;
	bIsActive = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	OnSpawnerCleared.Broadcast(this);
}

void ADCEnemySpawner::HandleOwnedGASDeathFinished(AActor* Enemy)
{
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Enemy);
	if (OwnedGASNPCs.Contains(Enemy) && Health && Health->GetDeathState() == EDCLyraDeathState::DeathFinished)
	{
		FinishedGASDeaths.Add(Enemy);
	}
}

void ADCEnemySpawner::CleanupOwnedGASNPCs()
{
	const auto Owned = OwnedGASNPCs;
	for (const auto& Entry : Owned)
	{
		if (AActor* Enemy = Entry.Key.Get()) { UnbindEnemy(Enemy); }
		LivingEnemies.Remove(Entry.Key);
	}
	OwnedGASNPCs.Reset();
	FinishedGASDeaths.Reset();
	if (Owned.IsEmpty() || GetWorld()->bIsTearingDown) { return; }
	// Never destroy the ASC in a death/damage callback. Capture no ended spawner.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([Owned]()
	{
		for (const auto& Entry : Owned)
		{
			AActor* Enemy = Entry.Key.Get();
			AController* Controller = Entry.Value.Get();
			const bool bOwnsController = Controller && (!Controller->GetPawn() || Controller->GetPawn() == Enemy);
			if (Enemy && !Enemy->IsActorBeingDestroyed()) { Enemy->Destroy(); }
			if (bOwnsController && IsValid(Controller) && !Controller->IsActorBeingDestroyed()) { Controller->Destroy(); }
		}
	}));
}
