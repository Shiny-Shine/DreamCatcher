#pragma once

#include "Engine/DataAsset.h"
#include "DCBossData.generated.h"

class UMaterialInterface;

/** R12 tuning, not final BIG BROTHER design. Attack geometry lives on the granted GAs. */
UCLASS(BlueprintType)
class DREAMCATCHER_API UDCBossData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss")
	FText DisplayName = NSLOCTEXT("DCBoss", "PrototypeName", "보스 프로토타입");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1"))
	float MaxHealth = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.01", ClampMax="0.99"))
	float PhaseTwoHealthRatio = .5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float OpeningDelay = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float TransitionDuration = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float PhaseOneGap = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1", Units="s"))
	float PhaseTwoGap = .35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Presentation")
	TObjectPtr<UMaterialInterface> WarningMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Presentation")
	TObjectPtr<UMaterialInterface> ImpactMaterial;

	bool IsValidConfiguration() const;
};
