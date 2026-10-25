// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "ControlFlowNode.h"
#include "LoadingProcessInterface.h"

#include "DCLyraFrontendStateComponent.generated.h"

class FControlFlow;
struct FStreamableHandle;
class FString;
class FText;
class UObject;
struct FFrame;

enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
class UCommonActivatableWidget;
class UAudioComponent;
class USoundBase;
class UCommonUserInfo;
class UDCLyraExperienceDefinition;

UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCLyraFrontendStateComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:

	UDCLyraFrontendStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "UI|Audio")
	UAudioComponent* GetMenuMusicComponent() const { return MenuMusicComponent; }

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCFrontendMusicTestAccess;
#endif
	void OnExperienceLoaded(const UDCLyraExperienceDefinition* Experience);
	void StartMenuMusic();
	void StopMenuMusic();

	UFUNCTION()
	void OnUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

	void FlowStep_WaitForUserInitialization(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowPressStartScreen(FControlFlowNodeRef SubFlow);
	void FlowStep_TryJoinRequestedSession(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowIntroScreen(FControlFlowNodeRef SubFlow);
	void ContinueAfterIntro();
	void FlowStep_TryShowMainScreen(FControlFlowNodeRef SubFlow);

	bool bShouldShowLoadingScreen = true;
	bool bEndingPlay = false;
	TArray<TWeakObjectPtr<UCommonActivatableWidget>> OwnedScreens;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> PressStartScreenClass;

	// Separate from Press Start: replay even when the local user is already initialized.
	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> IntroScreenClass;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> MainScreenClass;

	// The existing cue retains its gain/pitch/loop settings. Start only after the menu is visible.
	UPROPERTY(EditAnywhere, Category = "UI|Audio")
	TObjectPtr<USoundBase> MenuMusic;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MenuMusicComponent;
	bool bMenuMusicStartRequested = false;

	TSharedPtr<FControlFlow> FrontEndFlow;
	TSharedPtr<FStreamableHandle> PendingScreenLoad;
	
	// If set, this is the in-progress press start screen task
	FControlFlowNodePtr InProgressPressStartScreen;
	FControlFlowNodePtr InProgressIntroScreen;

	FDelegateHandle OnJoinSessionCompleteEventHandle;
};
