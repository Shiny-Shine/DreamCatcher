#include "UI/Stage/DCMissionTransitionSwitcher.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SSpacer.h"

UDCMissionTransitionSwitcher::UDCMissionTransitionSwitcher(const FObjectInitializer& Initializer) : Super(Initializer)
{
	TransitionType = ECommonSwitcherTransition::FadeOnly;
	TransitionCurveType = ETransitionCurve::CubicInOut;
	TransitionDuration = .4f;
	TransitionFallbackStrategy = ECommonSwitcherTransitionFallbackStrategy::None;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UDCMissionTransitionSwitcher::GetDisplayedPageIndex() const
{
	return MyAnimatedSwitcher.IsValid() ? MyAnimatedSwitcher->GetActiveWidgetIndex() : GetActiveWidgetIndex();
}

void UDCMissionTransitionSwitcher::PerformSnap(int32 Index)
{
	TGuardValue<bool> Suppress(bSuppressCallbacks, true);
	bSetOnce = false;
	SetDisableTransitionAnimation(true);
	Super::SetActiveWidgetIndex(Index);
	SetDisableTransitionAnimation(false);
	// Instant SCommon transitions stop their active timer on a later tick. Its
	// guard must be released now, without treating that late callback as a finish.
	if (MyInputGuard.IsValid()) { Super::HandleSlateIsTransitioningChanged(false); }
}

void UDCMissionTransitionSwitcher::SnapToPage(int32 Index, int64 Serial)
{
	AnimationSerial = Serial; TargetPage = Index; bInFlight = false;
	const uint64 Generation = ++CancelGeneration;
	if (bInsideSlateCallback)
	{
		if (MyInputGuard.IsValid()) { MyInputGuard->SetVisibility(EVisibility::Collapsed); }
		// SCommon resumes its sequence after the midpoint listener returns. Snap
		// only after that stack unwinds; newer transitions invalidate this request.
		const TWeakObjectPtr<UDCMissionTransitionSwitcher> Weak(this);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Index, Serial, Generation](float)
		{
			if (UDCMissionTransitionSwitcher* Self = Weak.Get(); Self && Self->AnimationSerial == Serial
				&& Self->CancelGeneration == Generation && !Self->bInFlight)
			{ Self->PerformSnap(Index); }
			return false;
		}));
		return;
	}
	PerformSnap(Index);
}

bool UDCMissionTransitionSwitcher::FadeToPage(int32 Index, float Duration, int64 Serial)
{
	++CancelGeneration;
	if (!MyAnimatedSwitcher.IsValid() || Index == GetDisplayedPageIndex() || !FMath::IsFinite(Duration) || Duration <= 0) { return false; }
	AnimationSerial = Serial; TargetPage = Index; bInFlight = true;
	TransitionDuration = Duration;
	MyAnimatedSwitcher->SetTransition(Duration, ETransitionCurve::CubicInOut, ECommonSwitcherTransition::FadeOnly);
	SetDisableTransitionAnimation(false);
	Super::SetActiveWidgetIndex(Index);
	return true;
}

void UDCMissionTransitionSwitcher::HandleSlateActiveIndexChanged(int32 ActiveIndex)
{
	TGuardValue<bool> Callback(bInsideSlateCallback, true);
	Super::HandleSlateActiveIndexChanged(ActiveIndex);
	if (!bSuppressCallbacks && bInFlight && ActiveIndex == TargetPage)
	{ OnMissionMidpoint.Broadcast(AnimationSerial, ActiveIndex); }
}

void UDCMissionTransitionSwitcher::HandleSlateIsTransitioningChanged(bool bIsTransitioning)
{
	TGuardValue<bool> Callback(bInsideSlateCallback, true);
	if (MyInputGuard.IsValid()) { Super::HandleSlateIsTransitioningChanged(bIsTransitioning); }
	if (!bIsTransitioning && !bSuppressCallbacks && bInFlight && MyAnimatedSwitcher.IsValid()
		&& !MyAnimatedSwitcher->IsTransitionPlaying() && GetDisplayedPageIndex() == TargetPage)
	{
		bInFlight = false; OnMissionFinished.Broadcast(AnimationSerial);
	}
}

void UDCMissionTransitionSwitcher::ReleaseSlateResources(bool bReleaseChildren)
{
	++CancelGeneration; ++AnimationSerial; bInFlight = false; TargetPage = INDEX_NONE;
	Super::ReleaseSlateResources(bReleaseChildren);
}
