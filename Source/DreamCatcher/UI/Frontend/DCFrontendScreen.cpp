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
#include "Engine/GameInstance.h"
#include "System/DCDifficultySubsystem.h"
#include "UI/Foundation/DCButtonBase.h"

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
	if (EasyDifficultyButton) { EasyDifficultyButton->OnClicked().AddUObject(this, &ThisClass::SelectEasy); }
	if (NormalDifficultyButton) { NormalDifficultyButton->OnClicked().AddUObject(this, &ThisClass::SelectNormal); }
	if (HardDifficultyButton) { HardDifficultyButton->OnClicked().AddUObject(this, &ThisClass::SelectHard); }
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
	if (EasyDifficultyButton && NormalDifficultyButton && HardDifficultyButton)
	{
		UCommonButtonBase* Choices[] = { EasyDifficultyButton.Get(), NormalDifficultyButton.Get(), HardDifficultyButton.Get() };
		UCommonButtonBase* Cards[] = { StoryButton.Get(), FreeButton.Get(), ChallengeButton.Get() };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Choices[Index]->SetIsSelectable(true);
			Choices[Index]->SetNavigationRuleExplicit(EUINavigation::Left, Choices[(Index + 2) % 3]);
			Choices[Index]->SetNavigationRuleExplicit(EUINavigation::Right, Choices[(Index + 1) % 3]);
			Choices[Index]->SetNavigationRuleExplicit(EUINavigation::Previous, Choices[(Index + 2) % 3]);
			Choices[Index]->SetNavigationRuleExplicit(EUINavigation::Next, Choices[(Index + 1) % 3]);
			Choices[Index]->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
			Choices[Index]->SetNavigationRuleExplicit(EUINavigation::Down, Cards[Index] ? Cards[Index] : StoryButton.Get());
			if (Cards[Index]) { Cards[Index]->SetNavigationRuleExplicit(EUINavigation::Up, Choices[Index]); }
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
	RefreshDifficulty();
}

UWidget* UDCFrontendScreen::NativeGetDesiredFocusTarget() const { return StoryButton; }
bool UDCFrontendScreen::NativeOnHandleBackAction() { RequestQuit(); return true; }

void UDCFrontendScreen::SetTravelPending(bool bPending)
{
	bTravelPending = bPending;
	for (UCommonButtonBase* Button : { static_cast<UCommonButtonBase*>(StoryButton.Get()),
		static_cast<UCommonButtonBase*>(FreeButton.Get()), static_cast<UCommonButtonBase*>(ChallengeButton.Get()),
		SettingsButton.Get(), QuitButton.Get(), static_cast<UCommonButtonBase*>(EasyDifficultyButton.Get()),
		static_cast<UCommonButtonBase*>(NormalDifficultyButton.Get()), static_cast<UCommonButtonBase*>(HardDifficultyButton.Get()) })
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
		StoryTravelFailed(NSLOCTEXT("DCFrontend", "UnavailableWithDifficulty", "스테이지 또는 난이도 설정을 확인할 수 없습니다."));
	}
}

void UDCFrontendScreen::SelectDifficulty(EDCDifficulty Difficulty)
{
	if (!IsActivated() || bTravelPending || bDialogOpen || SettingsScreen.IsValid() || !Mode.IsValid() || !Mode->GetDifficultyCatalog()) { return; }
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (Instance->GetSubsystem<UDCDifficultySubsystem>()->SelectDifficulty(Difficulty)) { RefreshDifficulty(); }
	}
}

void UDCFrontendScreen::RefreshDifficulty()
{
	const UGameInstance* Instance = GetGameInstance();
	const UDCDifficultySubsystem* Difficulty = Instance ? Instance->GetSubsystem<UDCDifficultySubsystem>() : nullptr;
	const EDCDifficulty Selected = Difficulty ? Difficulty->GetSelectedDifficulty() : EDCDifficulty::Normal;
	const FText EasyText = NSLOCTEXT("DCDifficulty", "Easy", "쉬움");
	const FText NormalText = NSLOCTEXT("DCDifficulty", "Normal", "보통");
	const FText HardText = NSLOCTEXT("DCDifficulty", "Hard", "어려움");
	UDCButtonBase* Buttons[] = { EasyDifficultyButton.Get(), NormalDifficultyButton.Get(), HardDifficultyButton.Get() };
	const FText Labels[] = { EasyText, NormalText, HardText };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (Buttons[Index])
		{
			Buttons[Index]->SetButtonText(Labels[Index]);
			Buttons[Index]->SetIsSelected(static_cast<int32>(Selected) == Index, false);
		}
	}
	if (DifficultyStatusText)
	{
		const FText Name = Selected == EDCDifficulty::Easy ? EasyText : (Selected == EDCDifficulty::Hard ? HardText : NormalText);
		DifficultyStatusText->SetText(FText::Format(NSLOCTEXT("DCDifficulty", "Selected", "난이도: {0}"), Name));
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
