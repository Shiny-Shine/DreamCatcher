#include "GameModes/DCFrontendGameMode.h"

#include "CommonPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCFrontendGameMode)

ADCFrontendGameMode::ADCFrontendGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = ACommonPlayerController::StaticClass();
	// The gameplay PlayerState subscribes to Experience and grants the fallback
	// combat PawnData. A menu needs only the CommonGame controller notification.
	PlayerStateClass = APlayerState::StaticClass();
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
}

void ADCFrontendGameMode::StartPlay()
{
	if (GEngine)
	{
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}
	Super::StartPlay();
}

bool ADCFrontendGameMode::ControllerCanRestart(AController*) { return false; }
UClass* ADCFrontendGameMode::GetDefaultPawnClassForController_Implementation(AController*) { return nullptr; }

bool ADCFrontendGameMode::RequestStartStory()
{
	if (bEndingPlay || bTravelPending || GetNetMode() != NM_Standalone || StoryMap.IsNull()) { return false; }
	const FString PackageName = StoryMap.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::IsValidLongPackageName(PackageName) || !FPackageName::DoesPackageExist(PackageName)) { return false; }
	bTravelPending = true;
	PendingDestination = FName(*PackageName);
	TravelTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ExecuteStoryTravel);
	return true;
}

void ADCFrontendGameMode::ExecuteStoryTravel()
{
	if (!bEndingPlay && bTravelPending && !GetWorld()->bIsTearingDown)
	{
		PerformStoryTravel(PendingDestination);
	}
}

void ADCFrontendGameMode::PerformStoryTravel(FName Destination)
{
	UGameplayStatics::OpenLevel(this, Destination);
}

void ADCFrontendGameMode::HandleTravelFailure(UWorld* World, ETravelFailure::Type, const FString&)
{
	if (World != GetWorld() || bEndingPlay || !bTravelPending) { return; }
	GetWorldTimerManager().ClearTimer(TravelTimer);
	bTravelPending = false;
	PendingDestination = NAME_None;
	OnStoryTravelFailed.Broadcast(NSLOCTEXT("DCFrontend", "TravelFailed", "스테이지를 열지 못했습니다. 다시 시도해 주세요."));
}

void ADCFrontendGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(TravelTimer);
	if (GEngine) { GEngine->OnTravelFailure().Remove(TravelFailureHandle); }
	OnStoryTravelFailed.Clear();
	Super::EndPlay(Reason);
}
