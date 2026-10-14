#pragma once

#include "CoreMinimal.h"
#include "AI/DCGASTestTarget.h"
#include "GameplayAbilitySpecHandle.h"
#include "DCRifleIntegrationTarget.generated.h"

class UDCLyraGameplayAbility_Death;
class UDCLyraHealthComponent;

/** Integration-only target. Keeps the old R3 health-print target unchanged. */
UCLASS(Blueprintable)
class DREAMCATCHER_API ADCRifleIntegrationTarget : public ADCGASTestTarget
{
	GENERATED_BODY()
public:
	ADCRifleIntegrationTarget();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="R6 Integration")
	TObjectPtr<UDCLyraHealthComponent> IntegrationHealthComponent;

	/** Assign the original-derived GA_Hero_Death, not the old DC death implementation. */
	UPROPERTY(EditDefaultsOnly, Category="R6 Integration")
	TSubclassOf<UDCLyraGameplayAbility_Death> DeathAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="R6 Integration", meta=(ClampMin="0", ClampMax="254"))
	uint8 IntegrationTeamId = 1;

private:
	UFUNCTION()
	void HandleIntegrationDeathStarted(AActor* OwningActor);
	UFUNCTION()
	void HandleIntegrationDeathFinished(AActor* OwningActor);

	FGameplayAbilitySpecHandle GrantedDeathAbility;
	bool bHealthInitialized = false;
};
