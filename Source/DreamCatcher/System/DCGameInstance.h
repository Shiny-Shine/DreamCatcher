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

	virtual void HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
		ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext) override;

protected:
	virtual void Init() override;
};
