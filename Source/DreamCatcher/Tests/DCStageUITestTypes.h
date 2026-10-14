#pragma once

#include "Tests/DCStageFlowTestTypes.h"
#include "DCStageUITestTypes.generated.h"

UCLASS(NotBlueprintable, Transient, HideDropDown)
class ADCR13StageModeFixture : public ADCR11ModeFixture
{
	GENERATED_BODY()
public:
	ADCR13StageModeFixture();
	void UseExperienceUI(bool bEnable) { bUseExperienceUI = bEnable; }
};
