// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCTeamPublicInfo.h"

#include "Net/UnrealNetwork.h"
#include "Teams/DCTeamInfoBase.h"
#include "Teams/DCTeamDisplayAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCTeamPublicInfo)

class FLifetimeProperty;

ADCTeamPublicInfo::ADCTeamPublicInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ADCTeamPublicInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, TeamDisplayAsset, COND_InitialOnly);
}

void ADCTeamPublicInfo::SetTeamDisplayAsset(TObjectPtr<UDCTeamDisplayAsset> NewDisplayAsset)
{
	check(HasAuthority());
	check(TeamDisplayAsset == nullptr);

	TeamDisplayAsset = NewDisplayAsset;

	TryRegisterWithTeamSubsystem();
}

void ADCTeamPublicInfo::OnRep_TeamDisplayAsset()
{
	TryRegisterWithTeamSubsystem();
}

