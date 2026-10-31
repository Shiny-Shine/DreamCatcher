#include "UI/Stage/DCStageResultLayer.h"
#include "UI/Stage/DCStageResultWidget.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
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
	if (!ContentHost)
	{
		// A native host only; the selected Blueprint owns every visible result element.
		ContentHost = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultHost"));
		WidgetTree->RootWidget = ContentHost;
	}
	EnsureContent();
	return Super::RebuildWidget();
}

void UDCStageResultLayer::UnbindContent()
{
	if (Content)
	{
		Content->OnRetryRequested.RemoveDynamic(this, &ThisClass::Retry);
		Content->OnExitRequested.RemoveDynamic(this, &ThisClass::ExitStage);
	}
}

void UDCStageResultLayer::EnsureContent()
{
	if (!ContentHost || !Mode.IsValid()) { return; }
	const TSubclassOf<UDCStageResultWidget> DesiredClass = Mode->GetResultWidgetClass();
	if (Content && Content->GetClass() != DesiredClass.Get())
	{
		UnbindContent(); ContentHost->SetContent(nullptr); Content = nullptr;
	}
	if (!DesiredClass || DesiredClass.Get() == UDCStageResultWidget::StaticClass()) { return; }
	if (!Content)
	{
		Content = WidgetTree->ConstructWidget<UDCStageResultWidget>(DesiredClass,
			MakeUniqueObjectName(WidgetTree, DesiredClass.Get(), TEXT("StageResult")));
		if (!Content) { return; }
		if (!Content->HasDesignerContent())
		{
			UE_LOG(LogTemp, Error, TEXT("Result Widget Class requires the named Designer panel, texts and buttons."));
			Content = nullptr; return;
		}
		ContentHost->SetContent(Content);
	}
	Content->SetOwningPlayer(GetOwningPlayer());
	Content->SetStageTitleVisible(false);
	Content->OnRetryRequested.AddUniqueDynamic(this, &ThisClass::Retry);
	Content->OnExitRequested.AddUniqueDynamic(this, &ThisClass::ExitStage);
}

void UDCStageResultLayer::BindStage(ADCStageRunGameMode* InMode)
{
	Mode = InMode; EnsureContent(); Refresh();
}

void UDCStageResultLayer::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureContent(); Refresh();
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
	UnbindContent();
	Mode.Reset();
	Super::NativeDestruct();
}
