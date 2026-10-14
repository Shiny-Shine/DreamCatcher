#include "AI/Boss/DCBossData.h"

bool UDCBossData::IsValidConfiguration() const
{
	return FMath::IsFinite(MaxHealth) && MaxHealth >= 1.f
		&& FMath::IsFinite(PhaseTwoHealthRatio) && PhaseTwoHealthRatio > 0.f && PhaseTwoHealthRatio < 1.f
		&& FMath::IsFinite(OpeningDelay) && OpeningDelay >= .1f
		&& FMath::IsFinite(TransitionDuration) && TransitionDuration >= .1f
		&& FMath::IsFinite(PhaseOneGap) && PhaseOneGap >= .1f
		&& FMath::IsFinite(PhaseTwoGap) && PhaseTwoGap >= .1f;
}
