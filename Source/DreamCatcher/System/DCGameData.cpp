// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCGameData.h"
#include "DCAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCGameData)

UDCGameData::UDCGameData()
{
}

const UDCGameData& UDCGameData::UDCGameData::Get()
{
	return UDCAssetManager::Get().GetGameData();
}
