// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/DCCameraAssistInterface.h"
#include "CommonPlayerController.h"
#include "PrimitiveComponentId.h"
#include "DreamCatcherPlayerController.generated.h"

class UDCAbilitySystemComponent;

UCLASS()
class DREAMCATCHER_API ADreamCatcherPlayerController
	: public ACommonPlayerController
	, public IDCCameraAssistInterface
{
	GENERATED_BODY()
	
public:
	ADreamCatcherPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnCameraPenetratingTarget() override;
	virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents) override;

	UDCAbilitySystemComponent* GetDCAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "DC|Character")
	void SetIsAutoRunning(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "DC|Character")
	bool GetIsAutoRunning() const;

protected:
	virtual void BeginPlay() override;
	virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;

	
	void OnStartAutoRun();
	void OnEndAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnStartAutoRun"))
	void K2_OnStartAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEndAutoRun"))
	void K2_OnEndAutoRun();

private:
	bool bHideViewTargetPawnNextFrame = false;
};
