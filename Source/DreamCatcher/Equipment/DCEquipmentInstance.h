#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DCEquipmentInstance.generated.h"

class AActor;
class APawn;
class UWorld;

struct FDCEquipmentActorToSpawn;

/**
 * 장착 시 생성되는 장비 런타임 객체.
 *
 * EquipmentManager가 Pawn을 Outer로 지정해서 생성.
 * 이후 WeaponInstance와 RangedWeaponInstance가 이 클래스를 상속.
 */
UCLASS(BlueprintType, Blueprintable)
class DREAMCATCHER_API UDCEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UDCEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UObject 자체에는 월드가 없으므로 소유 Pawn을 통해 구함.
	virtual UWorld* GetWorld() const override;

	// 이 장비를 장착한 Pawn.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	APawn* GetPawn() const;

	// 장비를 장착하게 만든 출처 객체. 이후 InventoryItemInstance 등을 연결할 수 있음.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	UObject* GetInstigator() const
	{
		return Instigator;
	}

	void SetInstigator(UObject* InInstigator);

	// 현재 이 Instance가 장착 상태인지 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	bool IsEquipped() const
	{
		return bEquipped;
	}

	// 이 장비가 생성한 유효한 Actor 목록을 반환.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Equipment")
	TArray<AActor*> GetSpawnedActors() const;

	// Definition의 설정으로 Actor들을 생성하고 부착. 실패하면 이번에 생성한 Actor들을 정리하고 false를 반환.
	virtual bool SpawnEquipmentActors(const TArray<FDCEquipmentActorToSpawn>& ActorsToSpawn);

	// 이 Instance가 생성한 Actor들만 제거.
	virtual void DestroyEquipmentActors();

	// 장착 절차가 끝난 뒤 EquipmentManager가 호출.
	virtual void OnEquipped();

	// 해제 절차 중 EquipmentManager가 호출.
	virtual void OnUnequipped();
	
	// EquipmentManager가 장착 중인 Instance에 호출하는 업데이트. 자동 Actor Tick이 아니며, 필요한 자식 클래스만 재정의.
	virtual void TickEquipment(float DeltaSeconds);

protected:
	// 무기별 Blueprint에서 장착 연출을 연결하는 이벤트.
	UFUNCTION(BlueprintImplementableEvent, Category = "DreamCatcher|Equipment", meta = (DisplayName = "On Equipped"))
	void K2_OnEquipped();

	// 무기별 Blueprint에서 해제 연출을 연결하는 이벤트.
	UFUNCTION(BlueprintImplementableEvent, Category = "DreamCatcher|Equipment", meta = (DisplayName = "On Unequipped"))
	void K2_OnUnequipped();

private:
	// 런타임 참조이므로 에셋 설정값으로 저장 X.
	UPROPERTY(Transient)
	TObjectPtr<UObject> Instigator;

	// 생성한 Actor를 추적하여 장비 해제 시 정확히 정리.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// 같은 장착·해제 이벤트가 중복 발생하는 것을 방지.
	UPROPERTY(Transient)
	bool bEquipped = false;
};
