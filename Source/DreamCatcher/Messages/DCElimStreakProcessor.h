// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Messages/DCGameplayMessageProcessor.h"

#include "DCElimStreakProcessor.generated.h"

class APlayerState;
class UObject;
struct FGameplayTag;
struct FDCVerbMessage;
template <typename T> struct TObjectPtr;

// Tracks a streak of eliminations (X eliminations without being eliminated)
UCLASS(Abstract)
class UDCElimStreakProcessor : public UDCGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	virtual void StartListening() override;

protected:
	// The event to rebroadcast when a user gets a streak of a certain length
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FGameplayTag> ElimStreakTags;

private:
	void OnEliminationMessage(FGameplayTag Channel, const FDCVerbMessage& Payload);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, int32> PlayerStreakHistory;
};
