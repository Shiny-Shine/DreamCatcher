// DreamCatcher-only test harness. The production equipment hierarchy remains an original Lyra port.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/Lyra/DCLyraEquipmentInstance.h"
#include "Equipment/Lyra/DCLyraEquipmentManagerComponent.h"
#include "Equipment/Lyra/DCLyraGameplayAbility_FromEquipment.h"
#include "Equipment/Lyra/DCLyraQuickBarComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/DCInventoryItemDefinition.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Inventory/DCInventoryManagerComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "Weapon/Lyra/DCLyraWeaponInstance.h"

namespace DCEquipmentTests
{
// Prepared by Scripts/Editor/r5_equipment_automation.py after the user builds C++.
constexpr const TCHAR* ItemClassPath = TEXT("/Game/DreamCatcher/GAS/Test/R5/Automation/Equipment/ID_DC_R5_EquipmentSmoke.ID_DC_R5_EquipmentSmoke_C");
constexpr const TCHAR* AbilityClassPath = TEXT("/Game/DreamCatcher/GAS/Test/R5/Automation/Equipment/GA_DC_R5_EquipmentSmoke.GA_DC_R5_EquipmentSmoke_C");

template <typename T>
T* AddTestComponent(AActor* Owner, const TCHAR* Name)
{
	T* Component = NewObject<T>(Owner, FName(Name), RF_Transient);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

struct FScopedMessageListeners
{
	FGameplayMessageListenerHandle Slots;
	FGameplayMessageListenerHandle ActiveIndex;
	~FScopedMessageListeners()
	{
		Slots.Unregister();
		ActiveIndex.Unregister();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCEquipmentQuickBarLifecycleTest,
	"DreamCatcher.R5.Equipment.QuickBarLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCEquipmentQuickBarLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace DCEquipmentTests;
	// GameInstance setup touches process-wide engine delegates. Never run this in the user's open editor.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCR5IsolatedAutomation")))
	{
		AddError(TEXT("Run in a separate UnrealEditor-Cmd process with -DCR5IsolatedAutomation; do not use an interactive editor session."));
		return false;
	}
	UClass* ItemClass = LoadClass<UDCInventoryItemDefinition>(nullptr, ItemClassPath);
	UClass* AbilityClass = LoadClass<UDCLyraGameplayAbility_FromEquipment>(nullptr, AbilityClassPath);
	if (!TestNotNull(TEXT("Prepared item fixture (run r5_equipment_automation.py prepare first)"), ItemClass)
		|| !TestNotNull(TEXT("Prepared FromEquipment ability fixture"), AbilityClass))
	{
		return false;
	}

	// Engine-owned RAII world helper: real GameInstance/message subsystem, no saved map or production Pawn.
	// This is a standalone authority test, NOT PIE, replication, or PlayerState/PawnExtension integration.
	FTestWorldWrapper Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.CreateTestWorld(EWorldType::Game)))
	{
		Fixture.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerController* Controller = World->SpawnActor<APlayerController>(SpawnParameters);
	ACharacter* Pawn = World->SpawnActor<ACharacter>(SpawnParameters);
	if (!TestNotNull(TEXT("Isolated native Controller"), Controller)
		|| !TestNotNull(TEXT("Isolated native Character"), Pawn))
	{
		return false;
	}
	Controller->Possess(Pawn);
	UDCAbilitySystemComponent* ASC = AddTestComponent<UDCAbilitySystemComponent>(Pawn, TEXT("R5_ASC"));
	UDCLyraEquipmentManagerComponent* Equipment = AddTestComponent<UDCLyraEquipmentManagerComponent>(Pawn, TEXT("R5_Equipment"));
	UDCInventoryManagerComponent* Inventory = AddTestComponent<UDCInventoryManagerComponent>(Controller, TEXT("R5_Inventory"));
	UDCLyraQuickBarComponent* QuickBar = AddTestComponent<UDCLyraQuickBarComponent>(Controller, TEXT("R5_QuickBar"));
	ASC->InitAbilityActorInfo(Pawn, Pawn);
	if (!TestTrue(TEXT("BeginPlay in isolated world"), Fixture.BeginPlayInTestWorld())
		|| !TestTrue(TEXT("GameInstance message subsystem exists"), UGameplayMessageSubsystem::HasInstance(QuickBar)))
	{
		Fixture.ForwardErrorMessages(this);
		return false;
	}
	TestEqual(TEXT("Original default slot count"), QuickBar->GetSlots().Num(), 3);
	TestEqual(TEXT("No automatic equipment in the isolated Pawn"), Equipment->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass()).Num(), 0);
	TestEqual(TEXT("No initial active slot"), QuickBar->GetActiveSlotIndex(), INDEX_NONE);

	int32 SlotMessages = 0;
	int32 ActiveMessages = 0;
	FScopedMessageListeners Listeners;
	UGameplayMessageSubsystem& Messages = UGameplayMessageSubsystem::Get(QuickBar);
	Listeners.Slots = Messages.RegisterListener<FDCLyraQuickBarSlotsChangedMessage>(
		FGameplayTag::RequestGameplayTag(TEXT("Lyra.QuickBar.Message.SlotsChanged")),
		[this, Controller, &SlotMessages](FGameplayTag, const FDCLyraQuickBarSlotsChangedMessage& Message)
		{
			TestTrue(TEXT("Slots message owner is the Controller"), Message.Owner == Controller);
			TestEqual(TEXT("Slots message contains all three slots"), Message.Slots.Num(), 3);
			++SlotMessages;
		});
	Listeners.ActiveIndex = Messages.RegisterListener<FDCLyraQuickBarActiveIndexChangedMessage>(
		FGameplayTag::RequestGameplayTag(TEXT("Lyra.QuickBar.Message.ActiveIndexChanged")),
		[this, Controller, &ActiveMessages](FGameplayTag, const FDCLyraQuickBarActiveIndexChangedMessage& Message)
		{
			TestTrue(TEXT("Active index message owner is the Controller"), Message.Owner == Controller);
			TestTrue(TEXT("Active index message has a valid index"), Message.ActiveIndex >= 0 && Message.ActiveIndex < 3);
			++ActiveMessages;
		});

	UDCInventoryItemInstance* FirstItem = Inventory->AddItemDefinition(ItemClass, 1);
	UDCInventoryItemInstance* SecondItem = Inventory->AddItemDefinition(ItemClass, 1);
	if (!TestNotNull(TEXT("First inventory item"), FirstItem) || !TestNotNull(TEXT("Second inventory item"), SecondItem))
	{
		return false;
	}
	TestTrue(TEXT("Items are separate instances"), FirstItem != SecondItem);
	const FGameplayTag StatTag = FGameplayTag::RequestGameplayTag(TEXT("Test.R2.CostCharge"));
	TestEqual(TEXT("Initial item stat"), FirstItem->GetStatTagStackCount(StatTag), 7);
	QuickBar->AddItemToSlot(0, FirstItem);
	QuickBar->AddItemToSlot(2, SecondItem);
	QuickBar->AddItemToSlot(0, SecondItem); // Original refuses to overwrite an occupied slot.
	TestEqual(TEXT("Only two slot change messages"), SlotMessages, 2);
	TestEqual(TEXT("Empty middle slot"), QuickBar->GetNextFreeItemSlot(), 1);

	auto CheckEquipped = [this, Pawn, ASC, Equipment, AbilityClass](UDCInventoryItemInstance* ExpectedItem) -> UDCLyraEquipmentInstance*
	{
		const TArray<UDCLyraEquipmentInstance*> Instances = Equipment->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass());
		if (!TestEqual(TEXT("Exactly one equipped instance"), Instances.Num(), 1))
		{
			return nullptr;
		}
		UDCLyraEquipmentInstance* Instance = Instances[0];
		TestTrue(TEXT("Equipment outer is the Pawn"), Instance->GetPawn() == Pawn);
		TestTrue(TEXT("Equipment instigator is the actual inventory item"), Instance->GetInstigator() == ExpectedItem);
		TestNotNull(TEXT("Original-derived WeaponInstance"), Cast<UDCLyraWeaponInstance>(Instance));
		const TArray<AActor*> Actors = Instance->GetSpawnedActors();
		if (TestEqual(TEXT("One equipment Actor spawned"), Actors.Num(), 1) && TestNotNull(TEXT("Spawned Actor"), Actors[0]))
		{
			TestTrue(TEXT("Actor owner is Pawn"), Actors[0]->GetOwner() == Pawn);
			if (TestNotNull(TEXT("Spawned Actor root"), Actors[0]->GetRootComponent()))
			{
				TestTrue(TEXT("Original Character mesh attachment"), Actors[0]->GetRootComponent()->GetAttachParent() == Pawn->GetMesh());
				TestTrue(TEXT("Definition attachment transform"), Actors[0]->GetRootComponent()->GetRelativeTransform().GetTranslation().Equals(FVector(10.0, 20.0, 30.0)));
			}
		}
		TestEqual(TEXT("Exactly one granted Ability"), ASC->GetActivatableAbilities().Num(), 1);
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
		if (TestNotNull(TEXT("AbilitySet granted the expected class"), Spec))
		{
			TestTrue(TEXT("Ability SourceObject is Equipment"), Spec->SourceObject.Get() == Instance);
			UDCLyraGameplayAbility_FromEquipment* Ability = Cast<UDCLyraGameplayAbility_FromEquipment>(Spec->GetPrimaryInstance());
			if (TestNotNull(TEXT("Instanced FromEquipment Ability"), Ability))
			{
				TestTrue(TEXT("GetAssociatedEquipment resolves SourceObject"), Ability->GetAssociatedEquipment() == Instance);
				TestTrue(TEXT("GetAssociatedItem resolves Instigator"), Ability->GetAssociatedItem() == ExpectedItem);
			}
		}
		return Instance;
	};
	auto CheckActorsRemoved = [this](UDCLyraEquipmentInstance* Instance)
	{
		if (Instance)
		{
			// Lyra retains the SpawnedActors array; actor destruction, not an empty array, is the contract.
			for (AActor* Actor : Instance->GetSpawnedActors())
			{
				TestTrue(TEXT("Old equipment Actor destroyed"), !IsValid(Actor) || Actor->IsActorBeingDestroyed());
			}
		}
	};

	QuickBar->SetActiveSlotIndex(0);
	UDCLyraEquipmentInstance* FirstEquipment = CheckEquipped(FirstItem);
	QuickBar->SetActiveSlotIndex(0);
	QuickBar->SetActiveSlotIndex(-1);
	QuickBar->SetActiveSlotIndex(99);
	TestEqual(TEXT("Same or invalid slot does not broadcast/equip again"), ActiveMessages, 1);
	TestTrue(TEXT("Same slot keeps its instance"), CheckEquipped(FirstItem) == FirstEquipment);
	FirstItem->RemoveStatTagStack(StatTag, 1);

	QuickBar->CycleActiveSlotForward();
	TestEqual(TEXT("Forward skips empty slot"), QuickBar->GetActiveSlotIndex(), 2);
	UDCLyraEquipmentInstance* SecondEquipment = CheckEquipped(SecondItem);
	CheckActorsRemoved(FirstEquipment);
	TestEqual(TEXT("Other item retains initial stats"), SecondItem->GetStatTagStackCount(StatTag), 7);
	QuickBar->CycleActiveSlotBackward();
	TestEqual(TEXT("Backward skips empty slot"), QuickBar->GetActiveSlotIndex(), 0);
	UDCLyraEquipmentInstance* Reequipped = CheckEquipped(FirstItem);
	CheckActorsRemoved(SecondEquipment);
	TestTrue(TEXT("Reequip creates new equipment, not a new item"), Reequipped != FirstEquipment);
	TestEqual(TEXT("Item stat survives unequip/reequip"), FirstItem->GetStatTagStackCount(StatTag), 6);

	const int32 ActiveMessagesBeforeRemoval = ActiveMessages;
	TestTrue(TEXT("Remove active slot returns its item"), QuickBar->RemoveItemFromSlot(0) == FirstItem);
	CheckActorsRemoved(Reequipped);
	TestEqual(TEXT("No active slot after removal"), QuickBar->GetActiveSlotIndex(), INDEX_NONE);
	TestNull(TEXT("No active item after removal"), QuickBar->GetActiveSlotItem());
	TestEqual(TEXT("All equipment removed"), Equipment->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass()).Num(), 0);
	TestEqual(TEXT("Granted Ability revoked"), ASC->GetActivatableAbilities().Num(), 0);
	// Original RemoveItemFromSlot broadcasts SlotsChanged only, not ActiveIndexChanged.
	TestEqual(TEXT("Original removal message behavior preserved"), ActiveMessages, ActiveMessagesBeforeRemoval);
	TestEqual(TEXT("Slot removal does not delete inventory item"), Inventory->GetAllItems().Num(), 2);

	QuickBar->SetActiveSlotIndex(2);
	UDCLyraEquipmentInstance* FinalEquipment = CheckEquipped(SecondItem);
	Equipment->DestroyComponent(); // Exercises original UninitializeComponent before ASC teardown.
	CheckActorsRemoved(FinalEquipment);
	TestEqual(TEXT("Component teardown revokes equipment Ability"), ASC->GetActivatableAbilities().Num(), 0);
	Inventory->RemoveItemInstance(FirstItem);
	Inventory->RemoveItemInstance(SecondItem);
	TestEqual(TEXT("Inventory cleanup"), Inventory->GetAllItems().Num(), 0);

	AddInfo(TEXT("Scope: standalone transient game world; no production PawnData, PIE, client replication, visuals, or device feedback validation."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
