#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/DCAbilitySet.h"
#include "Components/ActorComponent.h"
#include "DCEquipmentManagerComponent.generated.h"

class UDCAbilitySystemComponent;
class UDCEquipmentDefinition;
class UDCEquipmentInstance;
class UDCPawnExtensionComponent;
class UDCHealthComponent;
class UDCWeaponInstance;

// 장착된 장비 하나의 관리 기록.
// Instance와 부여한 GAS 항목을 같은 기록에 보관하여 장비 해제 시 해당 장비의 항목만 정리.
USTRUCT()
struct FDCEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UDCEquipmentDefinition> Definition;

	// UPROPERTY로 참조하여 장착 중인 UObject가 GC되지 않도록 함.
	UPROPERTY()
	TObjectPtr<UDCEquipmentInstance> Instance;

	// 해제 시 현재 Pawn에서 다시 찾지 않고 실제로 AbilitySet을 부여했던 ASC를 사용.
	UPROPERTY()
	TObjectPtr<UDCAbilitySystemComponent> GrantedAbilitySystem;

	UPROPERTY()
	FDCAbilitySet_GrantedHandles GrantedHandles;
};

// 장비 구성이 변경되면 HUD 등이 다시 조회할 수 있음.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCEquipmentChangedSignature);

// Pawn의 장비 장착·해제를 관리.
// 현재 단계는 Standalone 플레이 기준. 장비 Instance와 Actor의 네트워크 복제는 포함 X.
UCLASS(ClassGroup = (DreamCatcher), meta = (BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCEquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDCEquipmentManagerComponent();

	// 성공하면 장착된 Instance를 반환. 같은 Definition이 이미 장착되어 있으면 기존 Instance를 반환.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DreamCatcher|Equipment")
	UDCEquipmentInstance* EquipItem(TSubclassOf<UDCEquipmentDefinition> EquipmentDefinition,
	                                UObject* Instigator = nullptr);

	// 이 Manager가 관리하는 장비만 해제할 수 있음.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DreamCatcher|Equipment")
	bool UnequipItem(UDCEquipmentInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DreamCatcher|Equipment")
	void UnequipAll();

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	int32 GetEquipmentCount() const
	{
		return EquipmentEntries.Num();
	}

	// 이후 현재 WeaponInstance 또는 RangedWeaponInstance를 찾을 때 사용.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	UDCEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UDCEquipmentInstance> InstanceType) const;

	UPROPERTY(BlueprintAssignable, Category = "DreamCatcher|Equipment")
	FDCEquipmentChangedSignature OnEquipmentChanged;
	
	// 현재 장착 중인 무기를 반환. 무기가 없으면 nullptr이며, Blueprint에서는 Is Valid로 확인.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	UDCWeaponInstance* GetCurrentWeaponInstance() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 장착 중인 장비의 런타임 상태를 갱신.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void BeginPlay() override;

private:
	// 기록 하나가 부여한 GAS 항목과 생성 Actor를 정리.
	void ReleaseEntry(FDCEquipmentEntry& Entry);

	// 장착·해제 처리 도중 들어온 전체 정리 요청을 처리.
	void FinishEquipmentChange();

	UPROPERTY(Transient)
	TArray<FDCEquipmentEntry> EquipmentEntries;

	// Blueprint 이벤트에서 같은 작업이 중첩되는 것을 방지.
	bool bChangingEquipment = false;

	// 종료 중에는 새 장비를 장착하지 않음.
	bool bEndingPlay = false;

	// 처리 도중 EndPlay 등이 발생하면 현재 작업이 끝난 뒤 정리.
	bool bCleanupRequested = false;
	
	// ASC가 현재 Pawn에 연결되고 기본 AbilitySet 부여가 끝났을 때 호출.
	void HandleAbilitySystemInitialized();

	// ASC 연결이 끊어지기 전에 장비를 정리.
	void HandleAbilitySystemUninitializing();

	// 기존 HealthComponent의 사망 시작 이벤트를 받음.
	UFUNCTION()
	void HandleOwnerDeath(AActor* DeadActor);

	// 구독한 컴포넌트를 기억해 EndPlay에서 이벤트를 해제.
	TWeakObjectPtr<UDCPawnExtensionComponent> BoundPawnExtension;
	TWeakObjectPtr<UDCHealthComponent> BoundHealthComponent;

	// ASC 준비 전, 연결 해제 중, 사망 후에는 새 장비를 장착 X.
	bool bEquipmentEnabled = false;
};
