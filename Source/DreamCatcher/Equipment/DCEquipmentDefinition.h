#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DCEquipmentDefinition.generated.h"

class AActor;
class UDCAbilitySet;
class UDCEquipmentInstance;

// 장비를 장착할 때 생성할 Actor 하나의 설정.
// 하나의 장비가 여러 Actor를 생성할 수도 있으므로 Definition에서는 이 구조체를 배열로 보관.
USTRUCT(BlueprintType)
struct DREAMCATCHER_API FDCEquipmentActorToSpawn
{
	GENERATED_BODY()

	// 생성할 Actor Blueprint. 라이플에서는 지난 단계의 B_DC_Rifle을 지정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<AActor> ActorToSpawn;

	// 캐릭터 메시에서 Actor를 부착할 소켓. None이면 메시 컴포넌트 자체에 부착합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName AttachSocket = NAME_None;

	// 부착 소켓을 기준으로 적용할 위치, 회전, 크기. 월드 좌표가 아니라 소켓 기준 상대 Transform.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FTransform AttachTransform = FTransform::Identity;
};

// 장비 하나의 공유 설정.
// Blueprint 자식 클래스의 Class Defaults에서 값을 지정. 현재 Heat나 장착 여부 같은 런타임 상태는 저장 X.
UCLASS(Abstract, BlueprintType, Blueprintable, Const)
class DREAMCATCHER_API UDCEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UDCEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 장착 시 생성할 런타임 장비 객체의 클래스. 이후 라이플 전용 RangedWeaponInstance로 변경.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Equipment")
	TSubclassOf<UDCEquipmentInstance> InstanceType;

	// 장착할 동안 ASC에 부여할 AbilitySet. 실제 부여와 회수는 다음 단계의 EquipmentManager가 담당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Equipment")
	TArray<TObjectPtr<const UDCAbilitySet>> AbilitySetsToGrant;

	// 장착할 때 생성하고 부착할 Actor 목록.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Equipment",
		meta = (TitleProperty = "ActorToSpawn"))
	TArray<FDCEquipmentActorToSpawn> ActorsToSpawn;
};
