#include "UI/Stage/DCStageOverlayWidget.h"
#include "UI/Stage/DCStageResultWidget.h"
#include "UI/Boss/DCBossHealthWidget.h"
#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/World.h"
#include "EngineUtils.h"

TSharedRef<SWidget> UDCStageOverlayWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Root;
		StageDisplay = WidgetTree->ConstructWidget<UDCStageResultWidget>();
		BossDisplay = WidgetTree->ConstructWidget<UDCBossHealthWidget>();
		for (UWidget* Child : TArray<UWidget*> { StageDisplay.Get(), BossDisplay.Get() })
		{
			UCanvasPanelSlot* ChildSlot = Root->AddChildToCanvas(Child);
			ChildSlot->SetAnchors(FAnchors(0,0,1,1)); ChildSlot->SetOffsets(FMargin(0));
		}
		SetVisibility(ESlateVisibility::HitTestInvisible);
		BossDisplay->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UDCStageOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::Refresh, .1f, true);
	Refresh();
}

void UDCStageOverlayWidget::Refresh()
{
	const ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	if (!Mode || !Mode->UsesExperienceUI()) { return; }
	StageDisplay->SetStageTitle(Mode->GetStageTitle());
	if (!Boss.IsValid() && !Mode->HasRunEnded())
	{
		for (TActorIterator<ADCBossCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDeadOrDying()) { Boss = *It; break; }
		}
	}
	ADCBossCharacter* B = Boss.Get();
	const bool bShowBoss = B && !Mode->HasRunEnded() && !B->IsDeadOrDying() && B->GetBossData();
	BossDisplay->SetVisibility(bShowBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShowBoss)
	{
		BossDisplay->SetBossStatus(B->GetBossData()->DisplayName, B->GetHealthComponent()->GetHealth(),
			B->GetHealthComponent()->GetMaxHealth(), B->GetBossPhaseComponent()->GetBossPhase(),
			B->GetBossPhaseComponent()->GetBossRunState() == EDCBossRunState::Transitioning);
	}
}

void UDCStageOverlayWidget::NativeDestruct()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(UpdateTimer); }
	Boss.Reset();
	Super::NativeDestruct();
}
