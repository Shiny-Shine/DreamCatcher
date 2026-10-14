#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "Teams/DCTeamAgentInterface.h"
#include "DCGASTestTarget.generated.h"

class UAbilitySystemComponent;
class UDCAbilitySystemComponent;
class UDCHealthSet;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

// 사격과 GAS 데미지 연결을 검증하는 표적. AI와 사망 Ability는 사용 X.
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCGASTestTarget
	: public AActor
	, public IAbilitySystemInterface
	, public IDCTeamAgentInterface
{
	GENERATED_BODY()

public:
	ADCGASTestTarget();

	virtual UAbilitySystemComponent*
	GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "DreamCatcher|Test")
	float GetCurrentHealth() const;

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnDCTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;

	// 테스트용: 원본 TeamSubsystem을 통해 공격자와 표적의 팀을 설정합니다.
	// -1은 NoTeam, 0~254는 유효한 팀 ID입니다. 현재 Standalone 검증에 사용합니다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DreamCatcher|Test")
	bool ConfigureTestTeams(AActor* SourceActor, int32 SourceTeamId, int32 TargetTeamId);

	// Standalone test: trace from SourceActor to this target and use its equipped weapon as AbilitySource.
	// Returns the actual health loss. Teams are configured separately with ConfigureTestTeams.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DreamCatcher|Test")
	float ApplyWeaponSourceTestDamage(AActor* SourceActor, float SetByCallerDamage = 25.0f);

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UDCHealthSet> HealthSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DreamCatcher|Test", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "DreamCatcher|Test")
	void BP_OnHealthChanged(float CurrentHealth, float MaxHealth);

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	FDelegateHandle HealthChangedHandle;

	FGenericTeamId TestTeamID = FGenericTeamId::NoTeam;

	UPROPERTY()
	FOnDCTeamIndexChangedDelegate OnTeamChangedDelegate;
};
