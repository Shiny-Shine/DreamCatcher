// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DreamCatcherPlayerController.generated.h"

class UDCPlayerHUDWidget;
class UInputMappingContext;

UCLASS()
class DREAMCATCHER_API ADreamCatcherPlayerController : public APlayerController
{
	GENERATED_BODY()

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
};