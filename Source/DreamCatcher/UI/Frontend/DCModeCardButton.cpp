#include "UI/Frontend/DCModeCardButton.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCModeCardButton)

void UDCModeCardButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (ArtworkImage)
	{
		ArtworkImage->SetBrushFromTexture(Artwork);
		ArtworkImage->SetVisibility(Artwork ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
