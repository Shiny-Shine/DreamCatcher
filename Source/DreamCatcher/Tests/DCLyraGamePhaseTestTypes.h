// Unsaved phase fixtures only; do not use these classes in gameplay assets.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Phases/DCLyraGamePhaseAbility.h"
#include "DCLyraGamePhaseTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10RootPhaseFixture : public UDCLyraGamePhaseAbility
{
	GENERATED_BODY()
public:
	UDCR10RootPhaseFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10PostGamePhaseFixture : public UDCLyraGamePhaseAbility
{
	GENERATED_BODY()
public:
	UDCR10PostGamePhaseFixture();
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCR10PhaseObserverFixture : public UObject
{
	GENERATED_BODY()
public:
	void Started(const FGameplayTag& Tag) { ++Starts; }
	void Ended(const FGameplayTag& Tag) { ++Ends; }
	void AbilityEnded(const UDCLyraGamePhaseAbility* Phase) { if (Phase) { ++AbilityEnds; } }
	int32 Starts = 0;
	int32 Ends = 0;
	int32 AbilityEnds = 0;
};
