#pragma once

#include "UI/Foundation/DCButtonBase.h"
#include "DCModeCardButton.generated.h"

class UImage;
class UTexture2D;

/** CommonUI card whose layout and hover animation are authored in UMG. */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCModeCardButton : public UDCButtonBase
{
	GENERATED_BODY()
protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	TObjectPtr<UTexture2D> Artwork;

	// Composed card textures already contain the labels and outline.
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> ArtworkImage;
};
