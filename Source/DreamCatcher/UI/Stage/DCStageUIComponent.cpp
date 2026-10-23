#include "UI/Stage/DCStageUIComponent.h"
#include "UI/Stage/DCStageResultLayer.h"
#include "UI/Stage/DCMissionClearWidget.h"
#include "UI/DCLyraSettingScreen.h"
#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Character/DCLyraHealthComponent.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Player/DCPlayerState.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_R13_Menu, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_R13_Modal, "UI.Layer.Modal");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_R13_StageSlot, "HUD.Slot.StageStatus");

UDCStageUIComponent::UDCStageUIComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UDCStageUIComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_Standalone) { return; }
	GetWorld()->GetTimerManager().SetTimer(ConnectTimer, this, &ThisClass::ConnectLayers, .1f, true);
}

void UDCStageUIComponent::ConnectLayers()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	if (!PC || !PC->IsLocalController() || !Mode || !Mode->UsesExperienceUI())
	{
		GetWorld()->GetTimerManager().ClearTimer(ConnectTimer); return;
	}
	UPrimaryGameLayout* Layout = UPrimaryGameLayout::GetPrimaryGameLayout(PC);
	if (!Layout || !Mode->GetStageDirector()) { return; }
	UCommonActivatableWidgetContainerBase* MenuStack = Layout->GetLayerWidget(TAG_R13_Menu);
	UCommonActivatableWidgetContainerBase* ModalStack = Layout->GetLayerWidget(TAG_R13_Modal);
	if (!MenuStack || !ModalStack) { return; }
	Root = Layout; Menu = MenuStack; Modal = ModalStack; Stage = Mode->GetStageDirector();
	MenuHandle = MenuStack->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::LayerChanged);
	ModalHandle = ModalStack->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::LayerChanged);
	Stage->OnStageEnded.AddUniqueDynamic(this, &ThisClass::StageEnded);
	GetWorld()->GetTimerManager().ClearTimer(ConnectTimer);
	SynchronizeInput();
	if (Mode->HasRunEnded()) { StageEnded(Stage.Get(), Mode->WasRunSuccessful()); }
}

void UDCStageUIComponent::LayerChanged(UCommonActivatableWidget*) { SynchronizeInput(); }

void UDCStageUIComponent::SynchronizeInput()
{
	if (bEnding) { return; }
	const bool bMenuActive = (Menu.IsValid() && Menu->GetActiveWidget()) || (Modal.IsValid() && Modal->GetActiveWidget());
	SetMenuInputBlocked(bMenuActive);
}

void UDCStageUIComponent::SetMenuInputBlocked(bool bBlock)
{
	if (!bBlock || bEnding) { ReleaseInput(); return; }
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	ADCPlayerState* PS = PC ? PC->GetPlayerState<ADCPlayerState>() : nullptr;
	UDCAbilitySystemComponent* ASC = PS ? PS->GetDCAbilitySystemComponent() : nullptr;
	if (bOwnsIgnore && BlockedASC.Get() == ASC && ASC) { return; }
	ReleaseInput();
	if (!PC || !ASC) { return; }
	BlockedASC = ASC;
	ASC->AddLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked);
	PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); bOwnsIgnore = true;
	ASC->ClearAbilityInput(); ASC->CancelAimInputAndState();
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(PC->GetPawn());
	if (Health && !Health->IsDeadOrDying() && Health->GetHealth() > 0.f)
	{
		ASC->CancelAbilitiesByFunc([](const UDCGameplayAbility* Ability, FGameplayAbilitySpecHandle)
		{
			return Ability->CanBeCanceled() && (Ability->GetActivationPolicy() == EDCAbilityActivationPolicy::OnInputTriggered
				|| Ability->GetActivationPolicy() == EDCAbilityActivationPolicy::WhileInputActive);
		}, false);
	}
}

void UDCStageUIComponent::ReleaseInput()
{
	if (BlockedASC.IsValid()) { BlockedASC->RemoveLooseGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked); }
	BlockedASC.Reset();
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()); PC && bOwnsIgnore)
	{
		PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false);
	}
	bOwnsIgnore = false;
}

void UDCStageUIComponent::CancelUnappliedSettings()
{
	for (UCommonActivatableWidgetContainerBase* Layer : { Menu.Get(), Modal.Get() })
	{
		if (!Layer) { continue; }
		const auto Widgets = Layer->GetWidgetList();
		for (UCommonActivatableWidget* Widget : Widgets)
		{
			if (UDCLyraSettingScreen* Settings = Cast<UDCLyraSettingScreen>(Widget)) { Settings->CancelForStageTransition(); }
		}
	}
}

void UDCStageUIComponent::StageEnded(ADCStageDirector*, bool)
{
	if (!bEnding) { ResultTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::PresentResult); }
}

void UDCStageUIComponent::PresentResult()
{
	ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	if (bEnding || Result || MissionClear || !Root.IsValid() || !Mode || !Mode->HasRunEnded() || Mode->IsStageTravelPending()) { return; }
	CancelUnappliedSettings();
	Menu->ClearWidgets(); Modal->ClearWidgets();
	if (Mode->ShouldPlayMissionClear())
	{
		MissionClear = Root->PushWidgetToLayerStack<UDCMissionClearWidget>(TAG_R13_Menu, UDCMissionClearWidget::StaticClass(),
			[Mode](UDCMissionClearWidget& Widget) { Widget.BindStage(Mode); });
		SynchronizeInput();
		return;
	}
	Result = Root->PushWidgetToLayerStack<UDCStageResultLayer>(TAG_R13_Menu, UDCStageResultLayer::StaticClass(),
		[Mode](UDCStageResultLayer& Widget) { Widget.BindStage(Mode); });
	SynchronizeInput();
}

bool UDCStageUIComponent::ReturnFromMenu()
{
	ADCStageRunGameMode* Mode = GetWorld()->GetAuthGameMode<ADCStageRunGameMode>();
	if (bEnding || !Mode || !Mode->UsesExperienceUI()) { return false; }
	CancelUnappliedSettings();
	return Mode->RequestExitStageFromMenu();
}

void UDCStageUIComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEnding = true;
	GetWorld()->GetTimerManager().ClearTimer(ConnectTimer); GetWorld()->GetTimerManager().ClearTimer(ResultTimer);
	if (Stage.IsValid()) { Stage->OnStageEnded.RemoveDynamic(this, &ThisClass::StageEnded); }
	if (Menu.IsValid()) { Menu->OnDisplayedWidgetChanged().Remove(MenuHandle); }
	if (Modal.IsValid()) { Modal->OnDisplayedWidgetChanged().Remove(ModalHandle); }
	CancelUnappliedSettings();
	if (Root.IsValid() && Result) { Root->FindAndRemoveWidgetFromLayer(Result); }
	if (MissionClear)
	{
		MissionClear->StopPresentation();
		if (Root.IsValid()) { Root->FindAndRemoveWidgetFromLayer(MissionClear); }
		MissionClear = nullptr;
	}
	Result = nullptr;
	ReleaseInput();
	Super::EndPlay(Reason);
}
