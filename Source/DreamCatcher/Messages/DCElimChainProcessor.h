// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Messages/DCGameplayMessageProcessor.h"

#include "DCElimChainProcessor.generated.h"

class APlayerState;
class UObject;
struct FGameplayTag;
struct FDCVerbMessage;
template <typename T> struct TObjectPtr;

USTRUCT()
struct FDCPlayerElimChainInfo
{
	GENERATED_BODY()

	double LastEliminationTime = 0.0;

	int32 ChainCounter = 1;
};

// Tracks a chain of eliminations (X eliminations without more than Y seconds passing between each one)
UCLASS(Abstract)
class UDCElimChainProcessor : public UDCGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	virtual void StartListening() override;

protected:
	UPROPERTY(EditDefaultsOnly)
	float ChainTimeLimit = 4.5f;

	// The event to rebroadcast when a user gets a chain of a certain length
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FGameplayTag> ElimChainTags;

private:
	void OnEliminationMessage(FGameplayTag Channel, const FDCVerbMessage& Payload);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, FDCPlayerElimChainInfo> PlayerChainHistory;
};
