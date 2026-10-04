// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/DCCameraAssistInterface.h"
#include "GameFramework/PlayerController.h"
#include "PrimitiveComponentId.h"
#include "DreamCatcherPlayerController.generated.h"

class UDCPlayerHUDWidget;
class UInputMappingContext;
class UDCAbilitySystemComponent;

UCLASS()
class DREAMCATCHER_API ADreamCatcherPlayerController
	: public APlayerController
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
	// 로컬 플레이어에 적용할 기본 입력 매핑.
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	// 플레이어 HUD의 C++ 베이스 위젯 클래스.
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UDCPlayerHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCPlayerHUDWidget> HUDWidget;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;

	void ApplyInputMappingContexts();
	void CreateHUD();
	void BindHUDToCurrentPawn();
	
	void OnStartAutoRun();
	void OnEndAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnStartAutoRun"))
	void K2_OnStartAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEndAutoRun"))
	void K2_OnEndAutoRun();

private:
	bool bHideViewTargetPawnNextFrame = false;
};
