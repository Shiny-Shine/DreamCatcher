#include "UI/Frontend/DCFrontendScreen.h"

#include "UI/Frontend/DCModeCardButton.h"
#include "UI/DCLyraSettingScreen.h"
#include "GameModes/DCFrontendGameMode.h"
#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"
#include "Types/SlateEnums.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCFrontendScreen)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DCFrontend_Menu, "UI.Layer.Menu");

UDCFrontendScreen::UDCFrontendScreen(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InputConfig = EDCLyraWidgetInputMode::Menu;
	bIsBackHandler = true;
	bAutoRestoreFocus = true;
}

void UDCFrontendScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	StoryButton->OnClicked().AddUObject(this, &ThisClass::StartStory);
	if (FreeButton) { FreeButton->OnClicked().AddUObject(this, &ThisClass::FreeModeUnavailable); }
	if (ChallengeButton) { ChallengeButton->OnClicked().AddUObject(this, &ThisClass::ChallengeModeUnavailable); }
	if (SettingsButton) { SettingsButton->OnClicked().AddUObject(this, &ThisClass::OpenSettings); }
	if (QuitButton) { QuitButton->OnClicked().AddUObject(this, &ThisClass::RequestQuit); }
	if (FreeButton && ChallengeButton)
	{
		// Explicit neighbors avoid ambiguous navigation between the overlapping artwork bounds.
		UCommonButtonBase* Cards[] = { StoryButton.Get(), FreeButton.Get(), ChallengeButton.Get() };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Cards[Index]->SetNavigationRuleExplicit(EUINavigation::Left, Cards[(Index + 2) % 3]);
			Cards[Index]->SetNavigationRuleExplicit(EUINavigation::Right, Cards[(Index + 1) % 3]);
			Cards[Index]->SetNavigationRuleExplicit(EUINavigation::Previous, Cards[(Index + 2) % 3]);
			Cards[Index]->SetNavigationRuleExplicit(EUINavigation::Next, Cards[(Index + 1) % 3]);
			Cards[Index]->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
			Cards[Index]->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
		}
	}
}

void UDCFrontendScreen::NativeConstruct()
{
	Super::NativeConstruct();
	Mode = GetWorld()->GetAuthGameMode<ADCFrontendGameMode>();
	if (Mode.IsValid())
	{
		TravelFailureHandle = Mode->OnStoryTravelFailed.AddUObject(this, &ThisClass::StoryTravelFailed);
	}
	SetTravelPending(Mode.IsValid() && Mode->IsStoryTravelPending());
	StatusText->SetText(FText::GetEmpty());
}

UWidget* UDCFrontendScreen::NativeGetDesiredFocusTarget() const { return StoryButton; }
bool UDCFrontendScreen::NativeOnHandleBackAction() { RequestQuit(); return true; }

void UDCFrontendScreen::SetTravelPending(bool bPending)
{
	bTravelPending = bPending;
	for (UCommonButtonBase* Button : { static_cast<UCommonButtonBase*>(StoryButton.Get()),
		static_cast<UCommonButtonBase*>(FreeButton.Get()), static_cast<UCommonButtonBase*>(ChallengeButton.Get()),
		SettingsButton.Get(), QuitButton.Get() })
	{
		if (Button) { Button->SetIsEnabled(!bPending); }
	}
}

void UDCFrontendScreen::StartStory()
{
	if (!IsActivated() || bTravelPending || bDialogOpen || SettingsScreen.IsValid()) { return; }
	if (Mode.IsValid() && Mode->RequestStartStory())
	{
		SetTravelPending(true);
		StatusText->SetText(NSLOCTEXT("DCFrontend", "Starting", "스테이지를 여는 중…"));
	}
	else
	{
		StoryTravelFailed(NSLOCTEXT("DCFrontend", "Unavailable", "시작할 스테이지를 확인할 수 없습니다."));
	}
}

void UDCFrontendScreen::StoryTravelFailed(const FText& Message)
{
	SetTravelPending(false);
	StatusText->SetText(Message);
	if (IsActivated()) { StoryButton->SetFocus(); }
}

void UDCFrontendScreen::FreeModeUnavailable()
{
	ShowUnavailableMode(NSLOCTEXT("DCFrontend", "FreeMode", "프리 모드"));
}

void UDCFrontendScreen::ChallengeModeUnavailable()
{
	ShowUnavailableMode(NSLOCTEXT("DCFrontend", "ChallengeMode", "챌린지 모드"));
}

void UDCFrontendScreen::ShowUnavailableMode(const FText& ModeName)
{
	if (!IsActivated() || bTravelPending || bDialogOpen || SettingsScreen.IsValid()) { return; }
	const FText Message = FText::Format(NSLOCTEXT("DCFrontend", "ModeComingSoon", "{0}는 준비 중입니다."), ModeName);
	ULocalPlayer* Player = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = Player ? Player->GetSubsystem<UCommonMessagingSubsystem>() : nullptr;
	if (!Messaging) { StatusText->SetText(Message); return; }
	bDialogOpen = true;
	Messaging->ShowConfirmation(UCommonGameDialogDescriptor::CreateConfirmationOk(ModeName, Message),
		FCommonMessagingResultDelegate::CreateWeakLambda(this, [this](ECommonMessagingResult)
	{
		bDialogOpen = false;
	}));
}

void UDCFrontendScreen::OpenSettings()
{
	if (!IsActivated() || bTravelPending || bDialogOpen || SettingsScreen.IsValid() || !SettingsScreenClass) { return; }
	if (UPrimaryGameLayout* Root = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer()))
	{
		UCommonActivatableWidget* Screen = Root->PushWidgetToLayerStack(TAG_DCFrontend_Menu, SettingsScreenClass);
		SettingsScreen = Screen;
		if (Screen)
		{
			SettingsDeactivateHandle = Screen->OnDeactivated().AddUObject(this, &ThisClass::SettingsClosed);
		}
	}
}

void UDCFrontendScreen::RequestQuit()
{
	if (!IsActivated() || bTravelPending || bDialogOpen || SettingsScreen.IsValid()) { return; }
	ULocalPlayer* Player = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = Player ? Player->GetSubsystem<UCommonMessagingSubsystem>() : nullptr;
	if (!Messaging) { return; }
	bDialogOpen = true;
	UCommonGameDialogDescriptor* Dialog = UCommonGameDialogDescriptor::CreateConfirmationYesNo(
		NSLOCTEXT("DCFrontend", "QuitTitle", "게임 종료"), NSLOCTEXT("DCFrontend", "QuitBody", "DreamCatcher를 종료할까요?"));
	Messaging->ShowConfirmation(Dialog, FCommonMessagingResultDelegate::CreateWeakLambda(this, [this](ECommonMessagingResult Result)
	{
		bDialogOpen = false;
		if (Result == ECommonMessagingResult::Confirmed)
		{
			UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		}
	}));
}

void UDCFrontendScreen::SettingsClosed()
{
	if (UCommonActivatableWidget* Screen = SettingsScreen.Get())
	{
		Screen->OnDeactivated().Remove(SettingsDeactivateHandle);
	}
	SettingsDeactivateHandle.Reset();
	SettingsScreen.Reset();
}

void UDCFrontendScreen::CloseSettings()
{
	UCommonActivatableWidget* Screen = SettingsScreen.Get();
	SettingsClosed();
	if (!Screen) { return; }
	if (UDCLyraSettingScreen* Settings = Cast<UDCLyraSettingScreen>(Screen)) { Settings->CancelForStageTransition(); }
	if (UPrimaryGameLayout* Root = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer()))
	{
		Root->FindAndRemoveWidgetFromLayer(Screen);
	}
}

void UDCFrontendScreen::NativeDestruct()
{
	CloseSettings();
	if (Mode.IsValid()) { Mode->OnStoryTravelFailed.Remove(TravelFailureHandle); }
	Mode.Reset();
	Super::NativeDestruct();
}
