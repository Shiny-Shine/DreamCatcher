#include "Equipment/DCEquipmentInstance.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DreamCatcher.h"
#include "Engine/World.h"
#include "Equipment/DCEquipmentDefinition.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

UDCEquipmentInstance::UDCEquipmentInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

APawn* UDCEquipmentInstance::GetPawn() const
{
	// EquipmentManager가 반드시 Pawn을 Outer로 지정해야 함.
	return Cast<APawn>(GetOuter());
}

UWorld* UDCEquipmentInstance::GetWorld() const
{
	// 클래스 기본 객체는 실제 플레이 월드에 속하지 않음.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	const APawn* OwningPawn = GetPawn();

	return IsValid(OwningPawn) ? OwningPawn->GetWorld() : nullptr;
}

void UDCEquipmentInstance::SetInstigator(UObject* InInstigator)
{
	Instigator = InInstigator;
}

TArray<AActor*> UDCEquipmentInstance::GetSpawnedActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(SpawnedActors.Num());

	for (AActor* SpawnedActor : SpawnedActors)
	{
		// 외부에서 먼저 파괴된 Actor는 반환 X.
		if (IsValid(SpawnedActor))
		{
			Result.Add(SpawnedActor);
		}
	}

	return Result;
}

bool UDCEquipmentInstance::SpawnEquipmentActors(const TArray<FDCEquipmentActorToSpawn>& ActorsToSpawn)
{
	APawn* OwningPawn = GetPawn();
	UWorld* World = GetWorld();

	if (!IsValid(OwningPawn) || !World)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: owning Pawn or World is invalid."), *GetNameSafe(this));

		return false;
	}

	// 장비 생성은 Authority에서 실행. 현재 Standalone 플레이에서는 로컬 월드가 Authority.
	if (!OwningPawn->HasAuthority())
	{
		return false;
	}

	// 같은 Instance에서 Actor가 중복 생성되는 것을 방지.
	if (bEquipped || !SpawnedActors.IsEmpty())
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("Equipment [%s]: actors were already spawned or equipped."),
		       *GetNameSafe(this));

		return false;
	}

	// Actor가 없는 장비도 허용. 예로 AbilitySet만 제공하는 장비가 있을 수 있음.
	if (ActorsToSpawn.IsEmpty())
	{
		return true;
	}

	USceneComponent* AttachTarget = OwningPawn->GetRootComponent();

	if (ACharacter* Character = Cast<ACharacter>(OwningPawn))
	{
		// Character에서는 Capsule이 아니라 Skeletal Mesh에 부착.
		AttachTarget = Character->GetMesh();
	}

	if (!IsValid(AttachTarget))
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: attach target is invalid."), *GetNameSafe(this));

		return false;
	}

	// 생성 전에 설정을 먼저 확인. 소켓 이름이 틀렸는데 메시 원점에 붙는 문제를 방지.
	for (const FDCEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		UClass* ActorClass = SpawnInfo.ActorToSpawn.Get();

		if (!ActorClass || ActorClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: invalid ActorToSpawn [%s]."),
			       *GetNameSafe(this), *GetNameSafe(ActorClass));

			return false;
		}

		if (!SpawnInfo.AttachSocket.IsNone() && !AttachTarget->DoesSocketExist(SpawnInfo.AttachSocket))
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: socket [%s] was not found on [%s]."),
			       *GetNameSafe(this), *SpawnInfo.AttachSocket.ToString(), *GetNameSafe(AttachTarget));

			return false;
		}
	}

	for (const FDCEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		const FTransform ParentTransform =
			SpawnInfo.AttachSocket.IsNone()
				? AttachTarget->GetComponentTransform()
				: AttachTarget->GetSocketTransform(SpawnInfo.AttachSocket, RTS_World);

		// 상대 Transform과 소켓의 월드 Transform을 조합.
		// 처음부터 손 근처에서 생성되어 월드 원점에 나타나는 것을 피함.
		const FTransform SpawnTransform = SpawnInfo.AttachTransform * ParentTransform;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = OwningPawn;
		SpawnParameters.Instigator = OwningPawn;

		// 장착용 외형 Actor는 충돌 때문에 생성이 거부되지 않게 함.
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* NewActor = World->SpawnActor<AActor>(SpawnInfo.ActorToSpawn.Get(), SpawnTransform, SpawnParameters);

		if (!IsValid(NewActor))
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: failed to spawn [%s]."),
			       *GetNameSafe(this), *GetNameSafe(SpawnInfo.ActorToSpawn.Get()));

			// 여러 Actor 중 하나가 실패하면 먼저 생성된 것들도 정리.
			DestroyEquipmentActors();
			return false;
		}

		// 부착 실패 시에도 정리 대상에 포함되도록 먼저 기록.
		SpawnedActors.Add(NewActor);

		const bool bAttached = NewActor->AttachToComponent(
			AttachTarget, FAttachmentTransformRules::KeepWorldTransform, SpawnInfo.AttachSocket);

		if (!bAttached)
		{
			UE_LOG(LogDreamCatcher, Error, TEXT("Equipment [%s]: failed to attach [%s]."),
			       *GetNameSafe(this), *GetNameSafe(NewActor));

			DestroyEquipmentActors();
			return false;
		}

		// 최종적으로 Definition에 지정한 소켓 기준 보정값을 적용.
		NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
	}

	return true;
}

void UDCEquipmentInstance::DestroyEquipmentActors()
{
	// Destroy 중 다른 이벤트가 호출되어도 같은 목록을 다시 처리하지 않도록 내부 목록을 먼저 비움.
	const TArray<TObjectPtr<AActor>> ActorsToDestroy = SpawnedActors;
	SpawnedActors.Reset();

	for (AActor* SpawnedActor : ActorsToDestroy)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}
}

void UDCEquipmentInstance::OnEquipped()
{
	if (bEquipped)
	{
		return;
	}

	bEquipped = true;

	// Blueprint에서 장착 연출을 실행할 수 있음.
	K2_OnEquipped();
}

void UDCEquipmentInstance::OnUnequipped()
{
	if (!bEquipped)
	{
		return;
	}

	bEquipped = false;

	// 이 이벤트 시점에는 Actor가 아직 존재할 수 있어. 실제 Actor 제거는 EquipmentManager가 별도로 호출.
	K2_OnUnequipped();
}

void UDCEquipmentInstance::TickEquipment(float DeltaSeconds)
{
	// 일반 장비는 매 프레임 처리할 내용이 없음.
	(void)DeltaSeconds;
}