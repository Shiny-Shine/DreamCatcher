#pragma once

#include "AI/DCGASEnemyCharacter.h"
#include "DCBossCharacter.generated.h"

class UDCBossData;
class UDCBossPhaseComponent;
class UDCGameplayAbility_BossGroundStrike;

/** Single-life, stationary boss. Retains the accepted NPC GAS/Health/Death adapter. */
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCBossCharacter : public ADCGASEnemyCharacter
{
	GENERATED_BODY()
public:
	ADCBossCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetRangedAttackOrigin() const override;
	UFUNCTION(BlueprintPure, Category="Boss")
	UDCBossPhaseComponent* GetBossPhaseComponent() const { return BossPhase; }
	UFUNCTION(BlueprintPure, Category="Boss")
	const UDCBossData* GetBossData() const { return BossData; }
	TSubclassOf<UDCGameplayAbility_EnemyRangedAttack> GetLineAbilityClass() const { return RangedAttackAbilityClass; }
	TSubclassOf<UDCGameplayAbility_BossGroundStrike> GetGroundAbilityClass() const { return GroundStrikeAbilityClass; }
	AActor* GetRequestedGroundTarget() const { return RequestedGroundTarget.Get(); }
	bool TryStartGroundStrike(AActor* Target);
	bool CanRunBossCombat() const;
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss")
	TObjectPtr<const UDCBossData> BossData;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss")
	TSubclassOf<UDCGameplayAbility_BossGroundStrike> GroundStrikeAbilityClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UDCBossPhaseComponent> BossPhase;
	/** BigBrother opt-in. Yaw follows the living stage player even between attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Presentation")
	bool bAlwaysFacePlayer = false;
	/** Leave disabled for existing socket-based NPC/boss attack origins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Attack")
	bool bUseCustomAttackOrigin = false;
	/** Actor-local offset; BigBrother uses its rotation axis above the underground pivot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Attack", meta=(EditCondition="bUseCustomAttackOrigin", Units="cm", MakeEditWidget=true))
	FVector AttackOriginOffset = FVector(0.0, 0.0, 60.0);
private:
	TWeakObjectPtr<AActor> RequestedGroundTarget;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCBigBrotherTestAccess;
#endif
};
