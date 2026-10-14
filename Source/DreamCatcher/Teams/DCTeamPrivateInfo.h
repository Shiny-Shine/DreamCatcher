// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DCTeamInfoBase.h"

#include "DCTeamPrivateInfo.generated.h"

class UObject;

UCLASS()
class ADCTeamPrivateInfo : public ADCTeamInfoBase
{
	GENERATED_BODY()

public:
	ADCTeamPrivateInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
