#include "DCRifleIntegrationActors.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Engine/World.h"
#include "Equipment/DCEquipmentDefinition.h"
#include "Equipment/DCEquipmentManagerComponent.h"
#include "Equipment/Lyra/DCLyraEquipmentInstance.h"
#include "Equipment/Lyra/DCLyraEquipmentManagerComponent.h"
#include "Equipment/Lyra/DCLyraQuickBarComponent.h"
#include "GameFramework/Pawn.h"
#include "GameModes/Lyra/DCLyraGameState.h"
#include "Inventory/DCInventoryItemDefinition.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Inventory/DCInventoryManagerComponent.h"
#include "Player/DCPlayerState.h"
#include "TimerManager.h"
#include "Weapon/Lyra/DCLyraWeaponStateComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCRifleIntegrationActors)

DEFINE_LOG_CATEGORY_STATIC(LogDCRifleIntegration, Log, All);

ADCRifleIntegrationController::ADCRifleIntegrationController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RifleInventory = CreateDefaultSubobject<UDCInventoryManagerComponent>(TEXT("RifleInventory"));
	RifleQuickBar = CreateDefaultSubobject<UDCLyraQuickBarComponent>(TEXT("RifleQuickBar"));
	RifleWeaponState = CreateDefaultSubobject<UDCLyraWeaponStateComponent>(TEXT("RifleWeaponState"));
}

void ADCRifleIntegrationController::OnPossess(APawn* InPawn)
{
	ClearIntegrationRifle();
	DetachPawnListener();
	Super::OnPossess(InPawn);
	if (HasAuthority())
	{
		BoundPawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(InPawn);
		if (UDCPawnExtensionComponent* Extension = BoundPawnExtension.Get())
		{
			Extension->OnAbilitySystemUninitializing_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::ClearIntegrationRifle));
		}
		EquipIntegrationRifle();
	}
}

void ADCRifleIntegrationController::EquipIntegrationRifle()
{
	if (!HasAuthority() || GrantedRifle || !GetWorld()) { return; }
	GetWorldTimerManager().ClearTimer(EquipTimer);
	if (!StartingRifle)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] StartingRifle is not assigned on the integration Controller."));
		return;
	}
	// Wait only for the existing PawnExtension/ASC pipeline; no replacement initialization.
	RemainingReadinessChecks = 100;
	GetWorldTimerManager().SetTimer(EquipTimer, this, &ThisClass::TryEquipWhenReady, 0.05f, true);
}

void ADCRifleIntegrationController::TryEquipWhenReady()
{
	APawn* ControlledPawn = GetPawn();
	UDCPawnExtensionComponent* Extension = BoundPawnExtension.Get();
	UDCAbilitySystemComponent* ASC = GetDCAbilitySystemComponent();
	UDCLyraEquipmentManagerComponent* Equipment = ControlledPawn ? ControlledPawn->FindComponentByClass<UDCLyraEquipmentManagerComponent>() : nullptr;
	if (!IsValid(ControlledPawn) || !ControlledPawn->HasActorBegunPlay() || !Extension || Extension->IsAbilitySystemUninitializing()
		|| !ASC || ASC->GetAvatarActor() != ControlledPawn || !Equipment || RifleQuickBar->GetSlots().IsEmpty()
		|| !Extension->HasReachedInitState(DCGameplayTags::InitState_GameplayReady))
	{
		if (--RemainingReadinessChecks <= 0)
		{
			GetWorldTimerManager().ClearTimer(EquipTimer);
			UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] Loadout readiness timed out. Pawn=%s Extension=%s ASC=%s Equipment=%s"),
				*GetNameSafe(ControlledPawn), *GetNameSafe(Extension), *GetNameSafe(ASC), *GetNameSafe(Equipment));
		}
		return;
	}
	GetWorldTimerManager().ClearTimer(EquipTimer);
	if (ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death)) { return; }
	const UDCPawnData* PawnData = Extension->GetPawnData();
	const UDCEquipmentManagerComponent* Legacy = ControlledPawn->FindComponentByClass<UDCEquipmentManagerComponent>();
	if (!PawnData || PawnData->DefaultWeaponDefinition || (Legacy && Legacy->GetEquipmentCount() != 0)
		|| !Equipment->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass()).IsEmpty()
		|| RifleQuickBar->GetSlots()[0] != nullptr)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] Refusing duplicate loadout. Use a separate PawnData with no legacy DefaultWeaponDefinition and an empty slot 0."));
		return;
	}
	GrantedRifle = RifleInventory->AddItemDefinition(StartingRifle, 1);
	if (!GrantedRifle)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] Inventory item grant failed."));
		return;
	}
	RifleQuickBar->AddItemToSlot(0, GrantedRifle);
	RifleQuickBar->SetActiveSlotIndex(0);
	const TArray<UDCLyraEquipmentInstance*> Instances = Equipment->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass());
	if (Instances.Num() != 1 || !Instances[0] || Instances[0]->GetInstigator() != GrantedRifle)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] Equipment/Inventory Instigator link failed; removing this test loadout."));
		ClearIntegrationRifle();
		return;
	}
	UE_LOG(LogDCRifleIntegration, Log, TEXT("[R5-R6] Rifle equipped. Pawn=%s Item=%s Equipment=%s; firing/reloading remain in imported abilities."),
		*GetNameSafe(ControlledPawn), *GetNameSafe(GrantedRifle), *GetNameSafe(Instances[0]));
}

void ADCRifleIntegrationController::ClearIntegrationRifle()
{
	if (GetWorld()) { GetWorldTimerManager().ClearTimer(EquipTimer); }
	if (!HasAuthority() || bClearingLoadout || !GrantedRifle) { return; }
	TGuardValue<bool> Clearing(bClearingLoadout, true);
	const TArray<UDCInventoryItemInstance*> Slots = RifleQuickBar->GetSlots();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index] == GrantedRifle) { RifleQuickBar->RemoveItemFromSlot(Index); }
	}
	RifleInventory->RemoveItemInstance(GrantedRifle);
	GrantedRifle = nullptr;
	UE_LOG(LogDCRifleIntegration, Log, TEXT("[R5-R6] Test rifle removed; equipment abilities/actors released through QuickBar."));
}

void ADCRifleIntegrationController::DetachPawnListener()
{
	if (UDCPawnExtensionComponent* Extension = BoundPawnExtension.Get()) { Extension->UnregisterAbilitySystemDelegates(this); }
	BoundPawnExtension.Reset();
}

void ADCRifleIntegrationController::OnUnPossess()
{
	// QuickBar locates the EquipmentManager through GetPawn(): remove before Super clears it.
	ClearIntegrationRifle();
	DetachPawnListener();
	Super::OnUnPossess();
}

void ADCRifleIntegrationController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearIntegrationRifle();
	DetachPawnListener();
	Super::EndPlay(EndPlayReason);
}

ADCRifleIntegrationGameMode::ADCRifleIntegrationGameMode()
{
	PlayerControllerClass = ADCRifleIntegrationController::StaticClass();
	PlayerStateClass = ADCPlayerState::StaticClass();
	GameStateClass = ADCLyraGameState::StaticClass();
}

void ADCRifleIntegrationGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	ADCPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ADCPlayerState>() : nullptr;
	if (!PS || !IntegrationPawnData || IntegrationPawnData->DefaultWeaponDefinition)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] Integration GameMode requires ADCPlayerState and its own PawnData without a legacy weapon."));
		return;
	}
	const UDCPawnData* Existing = PS->GetPawnData<UDCPawnData>();
	if (Existing && Existing != IntegrationPawnData)
	{
		UE_LOG(LogDCRifleIntegration, Error, TEXT("[R5-R6] PlayerState already owns different PawnData; not overwriting it."));
		return;
	}
	// Test-map-only teams: preserve the original TeamSubsystem damage rules.
	PS->SetGenericTeamId(FGenericTeamId(0));
	UE_LOG(LogDCRifleIntegration, Log, TEXT("[R6] Integration shooter team=%d"), PS->GetTeamId());
	if (!Existing) { PS->SetPawnData(IntegrationPawnData); }
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}
