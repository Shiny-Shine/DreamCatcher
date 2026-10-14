// Transient regression fixtures only; never select these in gameplay assets.
#pragma once

#include "Character/DCHeroComponent.h"
#include "Weapon/Lyra/DCLyraGameplayAbility_RangedWeapon.h"
#include "DCMergeReviewTestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCReviewHeroFixture : public UDCHeroComponent
{
	GENERATED_BODY()
public:
	void InitializeForTest(UInputComponent* Input, const TArray<FInputMappingContextAndPriority>& Mappings)
	{
		DefaultInputMappings = Mappings;
		bReadyToBindInputs = false;
		InitializePlayerInput(Input);
	}
};

UCLASS(NotBlueprintable, Transient, HideDropDown)
class UDCReviewRangedWeaponFixture : public UDCLyraGameplayAbility_RangedWeapon
{
	GENERATED_BODY()
public:
	void TraceForTest(UDCLyraRangedWeaponInstance* Weapon, const FGameplayAbilityActorInfo* ActorInfo,
		const TArray<ECollisionChannel>& Channels, TArray<FHitResult>& OutHits);
	int32 GetTraceCountForTest() const { return NextTraceChannel; }

protected:
	virtual ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const override;

private:
	TArray<ECollisionChannel> TraceChannels;
	mutable int32 NextTraceChannel = 0;
};
