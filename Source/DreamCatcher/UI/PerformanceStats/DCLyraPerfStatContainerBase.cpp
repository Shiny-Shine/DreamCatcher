// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/PerformanceStats/DCLyraPerfStatContainerBase.h"

#include "Blueprint/WidgetTree.h"
#include "UI/PerformanceStats/DCLyraPerfStatWidgetBase.h"
#include "Settings/DCLyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraPerfStatContainerBase)

//////////////////////////////////////////////////////////////////////
// ULyraPerfStatsContainerBase

UDCLyraPerfStatContainerBase::UDCLyraPerfStatContainerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCLyraPerfStatContainerBase::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateVisibilityOfChildren();

	UDCLyraSettingsLocal::Get()->OnPerfStatDisplayStateChanged().AddUObject(this, &ThisClass::UpdateVisibilityOfChildren);
}

void UDCLyraPerfStatContainerBase::NativeDestruct()
{
	UDCLyraSettingsLocal::Get()->OnPerfStatDisplayStateChanged().RemoveAll(this);

	Super::NativeDestruct();
}

void UDCLyraPerfStatContainerBase::UpdateVisibilityOfChildren()
{
	UDCLyraSettingsLocal* UserSettings = UDCLyraSettingsLocal::Get();

	const bool bShowTextWidgets = (StatDisplayModeFilter == EDCLyraStatDisplayMode::TextOnly) || (StatDisplayModeFilter == EDCLyraStatDisplayMode::TextAndGraph);
	const bool bShowGraphWidgets = (StatDisplayModeFilter == EDCLyraStatDisplayMode::GraphOnly) || (StatDisplayModeFilter == EDCLyraStatDisplayMode::TextAndGraph);
	
	check(WidgetTree);
	WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		if (UDCLyraPerfStatWidgetBase* TypedWidget = Cast<UDCLyraPerfStatWidgetBase>(Widget))
		{
			const EDCLyraStatDisplayMode SettingMode = UserSettings->GetPerfStatDisplayState(TypedWidget->GetStatToDisplay());

			bool bShowWidget = false;
			switch (SettingMode)
			{
			case EDCLyraStatDisplayMode::Hidden:
				bShowWidget = false;
				break;
			case EDCLyraStatDisplayMode::TextOnly:
				bShowWidget = bShowTextWidgets;
				break;
			case EDCLyraStatDisplayMode::GraphOnly:
				bShowWidget = bShowGraphWidgets;
				break;
			case EDCLyraStatDisplayMode::TextAndGraph:
				bShowWidget = bShowTextWidgets || bShowGraphWidgets;
				break;
			}

			TypedWidget->SetVisibility(bShowWidget ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	});
}

