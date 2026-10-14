// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DCTeamInfoBase.h"

#include "DCTeamPublicInfo.generated.h"

class UDCTeamCreationComponent;
class UDCTeamDisplayAsset;
class UObject;
struct FFrame;

UCLASS()
class ADCTeamPublicInfo : public ADCTeamInfoBase
{
	GENERATED_BODY()

	friend UDCTeamCreationComponent;

public:
	ADCTeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UDCTeamDisplayAsset* GetTeamDisplayAsset() const { return TeamDisplayAsset; }

private:
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	void SetTeamDisplayAsset(TObjectPtr<UDCTeamDisplayAsset> NewDisplayAsset);

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamDisplayAsset)
	TObjectPtr<UDCTeamDisplayAsset> TeamDisplayAsset;
};
