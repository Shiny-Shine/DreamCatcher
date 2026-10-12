// Unsaved lifecycle fixtures. Production assets must use the original migrated Pawn/Controller.
#pragma once

#include "AI/DCEnemySpawner.h"
#include "GameFramework/Pawn.h"
#include "Tests/DCLyraAIAssemblyTestTypes.h"
#include "DCLyraFiniteSpawnerTestTypes.generated.h"

class UDCLyraHealthComponent;

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10FiniteSpawnerFixture : public ADCEnemySpawner
{
	GENERATED_BODY()
public:
	void ConfigureLyra();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10FinitePawnFixture : public APawn
{
	GENERATED_BODY()
public:
	ADCR10FinitePawnFixture();
	UPROPERTY()
	TObjectPtr<UDCLyraHealthComponent> Health;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION()
	void DeathFinished(AActor* Actor);
	void DestroyAfterDeath();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR10FiniteFailureGameModeFixture : public ADCR10AssemblyGameModeFixture
{
	GENERATED_BODY()
public:
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& Transform) override;
};
