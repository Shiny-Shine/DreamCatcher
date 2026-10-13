#include "UI/Stage/DCStageUIBlueprintLibrary.h"
#include "UI/Stage/DCStageUIComponent.h"
#include "GameModes/DCStageRunGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

bool UDCStageUIBlueprintLibrary::ReturnFromStageMenu(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	ADCStageRunGameMode* Mode = World ? World->GetAuthGameMode<ADCStageRunGameMode>() : nullptr;
	APlayerController* PC = Mode && Mode->UsesExperienceUI() ? Mode->GetStagePlayerController() : nullptr;
	UDCStageUIComponent* UI = PC ? PC->FindComponentByClass<UDCStageUIComponent>() : nullptr;
	return UI && UI->ReturnFromMenu();
}
