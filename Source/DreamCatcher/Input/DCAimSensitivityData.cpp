// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCAimSensitivityData.h"

#include "Settings/DCSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAimSensitivityData)

UDCAimSensitivityData::UDCAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SensitivityMap =
	{
		{ EDCGamepadSensitivity::Slow,			0.5f },
		{ EDCGamepadSensitivity::SlowPlus,		0.75f },
		{ EDCGamepadSensitivity::SlowPlusPlus,	0.9f },
		{ EDCGamepadSensitivity::Normal,		1.0f },
		{ EDCGamepadSensitivity::NormalPlus,	1.1f },
		{ EDCGamepadSensitivity::NormalPlusPlus,1.25f },
		{ EDCGamepadSensitivity::Fast,			1.5f },
		{ EDCGamepadSensitivity::FastPlus,		1.75f },
		{ EDCGamepadSensitivity::FastPlusPlus,	2.0f },
		{ EDCGamepadSensitivity::Insane,		2.5f },
	};
}

const float UDCAimSensitivityData::SensitivtyEnumToFloat(const EDCGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}

