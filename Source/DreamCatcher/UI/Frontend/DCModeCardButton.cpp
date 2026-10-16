#include "UI/Frontend/DCModeCardButton.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCModeCardButton)

void UDCModeCardButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (NumberText) { NumberText->SetText(ModeNumber); }
	if (LabelText) { LabelText->SetText(ModeLabel); }
	if (TitleText) { TitleText->SetText(ModeTitle); }
	if (DescriptionText) { DescriptionText->SetText(ModeDescription); }
	if (ArtworkImage)
	{
		ArtworkImage->SetBrushFromTexture(Artwork);
		ArtworkImage->SetVisibility(Artwork ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	RefreshHighlight();
}

void UDCModeCardButton::RefreshHighlight()
{
	if (CardOutline)
	{
		CardOutline->SetBrushColor((IsHovered() || bInFocusPath) ? AccentColor : FLinearColor(.75f, .75f, .8f, 1.f));
	}
}

void UDCModeCardButton::NativeOnHovered() { Super::NativeOnHovered(); RefreshHighlight(); }
void UDCModeCardButton::NativeOnUnhovered() { Super::NativeOnUnhovered(); RefreshHighlight(); }
void UDCModeCardButton::NativeOnAddedToFocusPath(const FFocusEvent& Event)
{
	Super::NativeOnAddedToFocusPath(Event); bInFocusPath = true; RefreshHighlight();
}
void UDCModeCardButton::NativeOnRemovedFromFocusPath(const FFocusEvent& Event)
{
	Super::NativeOnRemovedFromFocusPath(Event); bInFocusPath = false; RefreshHighlight();
}
