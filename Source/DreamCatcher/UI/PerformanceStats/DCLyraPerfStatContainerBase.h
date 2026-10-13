// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "Performance/DCLyraPerformanceStatTypes.h"
#include "DCLyraPerfStatContainerBase.generated.h"

class UObject;
struct FFrame;

/**
 * ULyraPerfStatsContainerBase
 *
 * Panel that contains a set of ULyraPerfStatWidgetBase widgets and manages
 * their visibility based on user settings.
 */
 UCLASS(Abstract)
class UDCLyraPerfStatContainerBase : public UCommonUserWidget
{
public:
	UDCLyraPerfStatContainerBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	GENERATED_BODY()

	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	UFUNCTION(BlueprintCallable)
	void UpdateVisibilityOfChildren();

protected:
	// Are we showing text or graph stats?
	UPROPERTY(EditAnywhere, Category=Display)
	EDCLyraStatDisplayMode StatDisplayModeFilter = EDCLyraStatDisplayMode::TextAndGraph;
};
