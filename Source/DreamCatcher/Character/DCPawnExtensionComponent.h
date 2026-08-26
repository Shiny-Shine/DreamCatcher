#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/DCAbilitySet.h"
#include "Components/ActorComponent.h"
#include "DCPawnExtensionComponent.generated.h"

class UDCAbilitySystemComponent;
class UDCPawnData;

/**
 * PawnData와 PlayerState 소유 ASC를 현재 Pawn에 연결하는 컴포넌트.
 *
 * PlayerState:
 *   ASC의 Owner
 *
 * Character:
 *   ASC의 Avatar
 */
UCLASS(ClassGroup = (DreamCatcher), meta = (BlueprintSpawnableComponent))
class DREAMCATCHER_API UDCPawnExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDCPawnExtensionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 지정한 Actor에서 PawnExtensionComponent를 찾음.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Pawn")
	static UDCPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor);

	// 이 Pawn이 사용할 설정 데이터.
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Pawn")
	const UDCPawnData* GetPawnData() const
	{
		return PawnData;
	}

	/**
	 * 현재 Pawn과 연결된 ASC.
	 *
	 * ASC의 실제 소유자는 PlayerState일 수 있음.
	 */
	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Ability System")
	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}

	// PlayerState가 소유한 ASC에 현재 Pawn을 Avatar로 연결.
	void InitializeAbilitySystem(UDCAbilitySystemComponent* InAbilitySystemComponent, AActor* InOwnerActor);

	// 현재 Pawn과 ASC의 연결을 해제.
	void UninitializeAbilitySystem();

	/**
	 * ASC 초기화 완료 이벤트에 등록.
	 *
	 * 이미 초기화된 상태라면 등록 직후 한 번 호출.
	 */
	void OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

	// ASC 연결이 해제될 때 호출되는 이벤트에 등록.
	void OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

protected:
	virtual void OnRegister() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 이 Pawn을 구성할 데이터 에셋.
	 *
	 * 테스트 Character Blueprint에서
	 * DA_DC_PlayerPawn을 할당.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DreamCatcher|Pawn")
	TObjectPtr<const UDCPawnData> PawnData;

private:
	// PlayerState가 소유하는 ASC를 캐시.
	UPROPERTY(Transient)
	TObjectPtr<UDCAbilitySystemComponent> AbilitySystemComponent;

	/**
	 * PawnData로부터 부여된 Ability, Effect, Attribute의 Handle.
	 *
	 * UnPossess 또는 Pawn 파괴 시 이 Pawn이 부여한 항목만
	 * ASC에서 제거하기 위해 보관.
	 */
	UPROPERTY()
	TArray<FDCAbilitySet_GrantedHandles> PawnDataGrantedHandles;

	FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;
};
