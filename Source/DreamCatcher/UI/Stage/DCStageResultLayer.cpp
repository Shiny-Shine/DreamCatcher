#include "UI/Stage/DCStageResultLayer.h"
#include "UI/Stage/DCStageResultWidget.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Input/CommonUIInputTypes.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_R13_ResultEscape, "UI.Action.Escape");

UDCStageResultLayer::UDCStageResultLayer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InputConfig = EDCLyraWidgetInputMode::Menu;
	bIsBackHandler = true;
	bIsModal = true;
}

void UDCStageResultLayer::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_R13_ResultEscape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::ConsumeEscape)));
}

TSharedRef<SWidget> UDCStageResultLayer::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		Content = WidgetTree->ConstructWidget<UDCStageResultWidget>(UDCStageResultWidget::StaticClass(), TEXT("StageResult"));
		WidgetTree->RootWidget = Content;
		Content->SetStageTitleVisible(false);
	}
	return Super::RebuildWidget();
}

void UDCStageResultLayer::BindStage(ADCStageRunGameMode* InMode) { Mode = InMode; Refresh(); }

void UDCStageResultLayer::NativeConstruct()
{
	Super::NativeConstruct();
	Content->SetOwningPlayer(GetOwningPlayer());
	Content->OnRetryRequested.AddUniqueDynamic(this, &ThisClass::Retry);
	Content->OnExitRequested.AddUniqueDynamic(this, &ThisClass::ExitStage);
	Refresh();
}

void UDCStageResultLayer::Refresh()
{
	if (Content && Mode.IsValid() && Mode->HasRunEnded())
	{
		Content->SetTravelPending(false);
		Content->ShowResult(Mode->WasRunSuccessful());
	}
}

UWidget* UDCStageResultLayer::NativeGetDesiredFocusTarget() const
{
	return Content ? Content->GetResultFocusTarget() : nullptr;
}

void UDCStageResultLayer::Retry()
{
	if (Mode.IsValid() && Mode->RequestRestartStage()) { Content->SetTravelPending(true); }
}

void UDCStageResultLayer::ExitStage()
{
	if (Mode.IsValid() && Mode->RequestExitStage()) { Content->SetTravelPending(true); }
}

void UDCStageResultLayer::NativeDestruct()
{
	if (Content)
	{
		Content->OnRetryRequested.RemoveDynamic(this, &ThisClass::Retry);
		Content->OnExitRequested.RemoveDynamic(this, &ThisClass::ExitStage);
	}
	Mode.Reset();
	Super::NativeDestruct();
}
