#pragma once

#include "CommonAnimatedSwitcher.h"
#include "DCMissionTransitionSwitcher.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FDCMissionSwitcherMidpoint, int64, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FDCMissionSwitcherFinished, int64);

/** Thin passive adapter around the original CommonUI FadeOnly/CubicInOut. */
UCLASS()
class DREAMCATCHER_API UDCMissionTransitionSwitcher : public UCommonAnimatedSwitcher
{
	GENERATED_BODY()
public:
	UDCMissionTransitionSwitcher(const FObjectInitializer& Initializer = FObjectInitializer::Get());
	void SnapToPage(int32 Index, int64 Serial);
	bool FadeToPage(int32 Index, float Duration, int64 Serial);
	int32 GetDisplayedPageIndex() const;
	FDCMissionSwitcherMidpoint OnMissionMidpoint;
	FDCMissionSwitcherFinished OnMissionFinished;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
	virtual void HandleSlateActiveIndexChanged(int32 ActiveIndex) override;
	virtual void HandleSlateIsTransitioningChanged(bool bIsTransitioning) override;
private:
	void PerformSnap(int32 Index);
	int64 AnimationSerial = 0;
	uint64 CancelGeneration = 0;
	int32 TargetPage = INDEX_NONE;
	bool bInFlight = false;
	bool bSuppressCallbacks = false;
	bool bInsideSlateCallback = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCMissionHUDTestAccess;
#endif
};
