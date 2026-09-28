// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"

#include "DCGameInstance.generated.h"

UCLASS(Config = Game)
class DREAMCATCHER_API UDCGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:
	UDCGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Init() override;
};
