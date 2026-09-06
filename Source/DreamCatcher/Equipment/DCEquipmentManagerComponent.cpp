#include "Equipment/DCEquipmentManagerComponent.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "DreamCatcher.h"
#include "Equipment/DCEquipmentDefinition.h"
#include "Equipment/DCEquipmentInstance.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Components/DCHealthComponent.h"
#include "Weapon/DCWeaponInstance.h"

UDCEquipmentManagerComponent::UDCEquipmentManagerComponent()
{
	// 장착된 무기의 Heat 회복과 상태별 퍼짐 갱신에 사용.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDCEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Pawn = Cast<APawn>(GetOwner());

	// 현재 단계에서는 Standalone/Authority에서만 장비를 생성하고 부여.
	if (!IsValid(Pawn) || !Pawn->HasAuthority())
	{
		return;
	}

	UDCPawnExtensionComponent* PawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(Pawn);

	UDCHealthComponent* HealthComponent = Pawn->FindComponentByClass<UDCHealthComponent>();

	if (!PawnExtension || !HealthComponent)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("EquipmentManager [%s] requires PawnExtension and HealthComponent."),
		       *GetNameSafe(this));

		return;
	}

	BoundPawnExtension = PawnExtension;
	BoundHealthComponent = HealthComponent;

	// 준비 완료 콜백이 즉시 실행될 수 있으므로 정리 이벤트부터 등록.
	PawnExtension->OnAbilitySystemUninitializing_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUninitializing));

	HealthComponent->OnDeath.AddUniqueDynamic(this, &ThisClass::HandleOwnerDeath);

	// ASC가 이미 준비되었다면 즉시 실행하고, 아니라면 준비 완료까지 대기.
	PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemInitialized));
}

void UDCEquipmentManagerComponent::HandleAbilitySystemInitialized()
{
	// 같은 연결에서 준비 완료 알림을 다시 받아도 기본 무기를 중복 장착 X.
	if (bEndingPlay || bEquipmentEnabled)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	UDCPawnExtensionComponent* PawnExtension = BoundPawnExtension.Get();

	if (!IsValid(Pawn) || !Pawn->HasAuthority() || !PawnExtension || PawnExtension->IsAbilitySystemUninitializing())
	{
		return;
	}

	UDCAbilitySystemComponent* ASC = PawnExtension->GetDCAbilitySystemComponent();

	if (!IsValid(ASC) || ASC->GetAvatarActor() != Pawn || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	const UDCHealthSet* HealthSet = ASC->GetSet<UDCHealthSet>();
	const UDCHealthComponent* HealthComponent = BoundHealthComponent.Get();

	// HealthComponent의 초기화 이벤트 순서에 의존하지 않도록 실제 Attribute도 확인.
	if (!HealthSet || HealthSet->GetHealth() <= 0.0f || ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dead) ||
		(HealthComponent && HealthComponent->IsDeadOrDying()))
	{
		return;
	}

	// 여기부터 수동 장착과 기본 무기 장착을 허용.
	bEquipmentEnabled = true;

	const UDCPawnData* PawnData = PawnExtension->GetPawnData();

	// 기본 무기를 지정하지 않은 Pawn은 빈손으로 시작할 수 있음.
	if (!PawnData || !PawnData->DefaultWeaponDefinition)
	{
		return;
	}

	const UDCEquipmentDefinition* Definition =
		PawnData->DefaultWeaponDefinition->GetDefaultObject<UDCEquipmentDefinition>();

	UClass* InstanceClass = Definition->InstanceType.Get();

	// DefaultWeaponDefinition에는 일반 장비가 아니라 무기 계열을 지정.
	if (!InstanceClass || !InstanceClass->IsChildOf(UDCWeaponInstance::StaticClass()))
	{
		UE_LOG(LogDreamCatcher, Error,
		       TEXT("PawnData [%s]: DefaultWeaponDefinition must use a WeaponInstance subclass."),
		       *GetNameSafe(PawnData));

		return;
	}

	// Inventory가 아직 없으므로 Instigator는 nullptr.
	UDCEquipmentInstance* Instance = EquipItem(PawnData->DefaultWeaponDefinition, nullptr);

	if (!Instance)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("EquipmentManager [%s]: failed to equip default weapon."),
		       *GetNameSafe(this));
	}
}

void UDCEquipmentManagerComponent::HandleAbilitySystemUninitializing()
{
	// 먼저 새 장착을 막아 해제 이벤트에서 장비가 다시 생기지 않도록 함.
	bEquipmentEnabled = false;

	UnequipAll();
}

void UDCEquipmentManagerComponent::HandleOwnerDeath(AActor* DeadActor)
{
	if (DeadActor != GetOwner())
	{
		return;
	}

	// 사망 시에는 Pawn이 아직 남아 있어도 무기와 무기 Ability를 회수.
	bEquipmentEnabled = false;

	UnequipAll();
}

UDCWeaponInstance* UDCEquipmentManagerComponent::GetCurrentWeaponInstance() const
{
	// 별도 CurrentWeapon 변수를 만들지 않고 기존 장착 목록을 원본으로 사용.
	return Cast<UDCWeaponInstance>(GetFirstInstanceOfType(UDCWeaponInstance::StaticClass()));
}

UDCEquipmentInstance* UDCEquipmentManagerComponent::
EquipItem(TSubclassOf<UDCEquipmentDefinition> EquipmentDefinition, UObject* Instigator)
{
	APawn* Pawn = Cast<APawn>(GetOwner());

	if (!bEquipmentEnabled || bEndingPlay || bChangingEquipment || !IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || !
		Pawn->HasAuthority())
	{
		return nullptr;
	}

	// 연결 해제 이벤트의 구독 순서와 무관하게 새 장착을 막음.
	const UDCPawnExtensionComponent* PawnExtension = BoundPawnExtension.Get();

	if (!PawnExtension || PawnExtension->IsAbilitySystemUninitializing())
	{
		return nullptr;
	}

	UClass* DefinitionClass = EquipmentDefinition.Get();

	if (!DefinitionClass || DefinitionClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("EquipmentManager [%s]: invalid EquipmentDefinition."), *GetNameSafe(this));

		return nullptr;
	}

	// Character의 IAbilitySystemInterface를 통해 PlayerState가 소유한 ASC까지 찾음.
	UDCAbilitySystemComponent* ASC = Cast<UDCAbilitySystemComponent>(
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));

	if (!IsValid(ASC) || ASC->GetAvatarActor() != Pawn || !ASC->IsOwnerActorAuthoritative())
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("EquipmentManager [%s]: Pawn ASC is not ready."), *GetNameSafe(this));

		return nullptr;
	}

	if (ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dead))
	{
		return nullptr;
	}

	// 사망 태그가 설정되기 직전의 체력 0 상태에서도 장착을 막음.
	const UDCHealthSet* HealthSet = ASC->GetSet<UDCHealthSet>();

	if (!HealthSet || HealthSet->GetHealth() <= 0.0f)
	{
		return nullptr;
	}

	if (const UDCHealthComponent* HealthComponent = BoundHealthComponent.Get())
	{
		if (HealthComponent->IsDeadOrDying())
		{
			return nullptr;
		}
	}

	// 같은 장비의 중복 장착 요청은 기존 Instance를 반환.
	for (const FDCEquipmentEntry& Entry : EquipmentEntries)
	{
		if (Entry.Definition == EquipmentDefinition && IsValid(Entry.Instance.Get()))
		{
			return Entry.Instance.Get();
		}
	}

	const UDCEquipmentDefinition* Definition = DefinitionClass->GetDefaultObject<UDCEquipmentDefinition>();

	UClass* InstanceClass = Definition->InstanceType.Get();

	if (!InstanceClass || InstanceClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: invalid InstanceType."), *GetNameSafe(DefinitionClass));

		return nullptr;
	}

	// 현재 단계에서는 무기를 하나만 장착. 일반 장비까지 하나로 제한하지는 않음.
	if (InstanceClass->IsChildOf(UDCWeaponInstance::StaticClass()) && GetCurrentWeaponInstance())
	{
		UE_LOG(LogDreamCatcher, Warning,
		       TEXT("EquipmentManager [%s]: unequip the current weapon before equipping another weapon."),
		       *GetNameSafe(this)
		);

		return nullptr;
	}

	// 빈 배열은 허용하지만, 배열 안의 None 항목은 설정 오류로 처리.
	for (const UDCAbilitySet* AbilitySet : Definition->AbilitySetsToGrant)
	{
		if (!IsValid(AbilitySet))
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: AbilitySetsToGrant contains None."),
			       *GetNameSafe(DefinitionClass));

			return nullptr;
		}
	}

	bChangingEquipment = true;

	// 함수 중간에서 return하더라도 변경 상태를 반드시 정리.
	ON_SCOPE_EXIT
	{
		FinishEquipmentChange();
	};

	const int32 NewIndex = EquipmentEntries.AddDefaulted();

	FDCEquipmentEntry& NewEntry = EquipmentEntries[NewIndex];
	NewEntry.Definition = EquipmentDefinition;
	NewEntry.GrantedAbilitySystem = ASC;

	// Instance의 GetPawn()이 동작하도록 Outer는 Manager가 아닌 Pawn.
	NewEntry.Instance = NewObject<UDCEquipmentInstance>(Pawn, InstanceClass);

	UDCEquipmentInstance* NewInstance = NewEntry.Instance.Get();
	NewInstance->SetInstigator(Instigator);

	// Actor 생성에 실패하면 AbilitySet은 부여 X.
	if (!NewInstance->SpawnEquipmentActors(Definition->ActorsToSpawn))
	{
		NewInstance->DestroyEquipmentActors();
		NewInstance->SetInstigator(nullptr);
		EquipmentEntries.RemoveAt(NewIndex);

		return nullptr;
	}

	// Actor의 BeginPlay 등에서 종료 요청이 들어왔다면 더 진행하지 않고 함수 종료 시 전체 정리를 수행.
	if (!bEquipmentEnabled || bEndingPlay || bCleanupRequested)
	{
		return nullptr;
	}

	for (const UDCAbilitySet* AbilitySet : Definition->AbilitySetsToGrant)
	{
		// SourceObject는 EquipmentInstance.
		// 이후 사격 Ability는 자신을 부여한 장비를 SourceObject를 통해 찾을 수 있음.
		AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, NewInstance);

		if (!bEquipmentEnabled || bEndingPlay || bCleanupRequested)
		{
			return nullptr;
		}
	}

	// Actor와 GAS 항목이 준비된 뒤 장착 이벤트를 호출.
	NewInstance->OnEquipped();

	if (!bEquipmentEnabled || bEndingPlay || bCleanupRequested)
	{
		return nullptr;
	}

	UE_LOG(LogDreamCatcher, Log, TEXT("Equipment [%s] equipped on [%s]. Count: %d"),
	       *GetNameSafe(DefinitionClass), *GetNameSafe(Pawn), EquipmentEntries.Num());

	OnEquipmentChanged.Broadcast();

	return (!bEquipmentEnabled || bEndingPlay || bCleanupRequested) ? nullptr : NewInstance;
}

bool UDCEquipmentManagerComponent::UnequipItem(UDCEquipmentInstance* ItemInstance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bChangingEquipment || !ItemInstance)
	{
		return false;
	}

	const int32 Index = EquipmentEntries.IndexOfByPredicate([ItemInstance](const FDCEquipmentEntry& Entry)
	{
		return Entry.Instance.Get() == ItemInstance;
	});

	// 다른 Manager의 Instance 또는 이미 해제한 Instance는 처리하지 않음.
	if (Index == INDEX_NONE)
	{
		return false;
	}

	bChangingEquipment = true;

	ON_SCOPE_EXIT
	{
		FinishEquipmentChange();
	};

	// 콜백에서 다시 조회해도 이미 해제된 장비로 보이도록 관리 목록에서 먼저 제거.
	FDCEquipmentEntry RemovedEntry = EquipmentEntries[Index];
	EquipmentEntries.RemoveAt(Index);

	ReleaseEntry(RemovedEntry);

	UE_LOG(LogDreamCatcher, Log, TEXT("Equipment unequipped on [%s]. Count: %d"),
	       *GetNameSafe(GetOwner()), EquipmentEntries.Num());

	OnEquipmentChanged.Broadcast();

	return true;
}

void UDCEquipmentManagerComponent::UnequipAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || EquipmentEntries.IsEmpty())
	{
		return;
	}

	if (bChangingEquipment)
	{
		// 장착 이벤트 처리 도중 전체 해제가 요청되면 현재 배열을 즉시 변경하지 않고 작업 종료 시 처리.
		bCleanupRequested = true;
		return;
	}

	bChangingEquipment = true;

	ON_SCOPE_EXIT
	{
		FinishEquipmentChange();
	};

	while (!EquipmentEntries.IsEmpty())
	{
		const int32 LastIndex = EquipmentEntries.Num() - 1;

		FDCEquipmentEntry RemovedEntry = EquipmentEntries[LastIndex];
		EquipmentEntries.RemoveAt(LastIndex);

		ReleaseEntry(RemovedEntry);
	}

	OnEquipmentChanged.Broadcast();
}

void UDCEquipmentManagerComponent::ReleaseEntry(FDCEquipmentEntry& Entry)
{
	// 관리 배열에서 제거한 뒤에도 정리 함수가 끝날 때까지 Instance와 ASC가 유지되도록 강한 참조를 잡음.
	TStrongObjectPtr<UDCEquipmentInstance> Instance(Entry.Instance.Get());

	TStrongObjectPtr<UDCAbilitySystemComponent> ASC(Entry.GrantedAbilitySystem.Get());

	// 무기 Actor가 존재하는 동안 실행 중인 장비 Ability부터 정리.
	if (IsValid(ASC.Get()))
	{
		Entry.GrantedHandles.TakeFromAbilitySystem(ASC.Get());
	}

	if (IsValid(Instance.Get()))
	{
		Instance->OnUnequipped();
		Instance->DestroyEquipmentActors();
		Instance->SetInstigator(nullptr);
	}

	Entry.Instance = nullptr;
	Entry.GrantedAbilitySystem = nullptr;
}

UDCEquipmentInstance* UDCEquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<UDCEquipmentInstance> InstanceType) const
{
	if (!InstanceType)
	{
		return nullptr;
	}

	for (const FDCEquipmentEntry& Entry : EquipmentEntries)
	{
		UDCEquipmentInstance* Instance = Entry.Instance.Get();

		if (IsValid(Instance) && Instance->IsEquipped() && Instance->IsA(InstanceType.Get()))
		{
			return Instance;
		}
	}

	return nullptr;
}

void UDCEquipmentManagerComponent::FinishEquipmentChange()
{
	bChangingEquipment = false;

	if (bCleanupRequested)
	{
		bCleanupRequested = false;
		UnequipAll();
	}
}

void UDCEquipmentManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	bEquipmentEnabled = false;

	// 종료된 컴포넌트가 이후 ASC 이벤트를 받지 않도록 구독을 해제.
	if (UDCPawnExtensionComponent* PawnExtension = BoundPawnExtension.Get())
	{
		PawnExtension->UnregisterAbilitySystemDelegates(this);
	}

	if (UDCHealthComponent* HealthComponent = BoundHealthComponent.Get())
	{
		HealthComponent->OnDeath.RemoveDynamic(this, &ThisClass::HandleOwnerDeath);
	}

	// PawnExtension이 먼저 정리했어도 빈 목록에 대한 호출은 안전.
	UnequipAll();

	BoundPawnExtension.Reset();
	BoundHealthComponent.Reset();

	Super::EndPlay(EndPlayReason);
}

void UDCEquipmentManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEquipmentEnabled || bEndingPlay || bChangingEquipment)
	{
		return;
	}

	for (const FDCEquipmentEntry& Entry : EquipmentEntries)
	{
		UDCEquipmentInstance* Instance = Entry.Instance.Get();

		if (IsValid(Instance) && Instance->IsEquipped())
		{
			// 계산용 업데이트. 이번 구현에서는 이 호출 안에서 장착 목록을 변경 X.
			Instance->TickEquipment(DeltaTime);
		}
	}
}
