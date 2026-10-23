#pragma once

#include "UI/Foundation/DCButtonBase.h"
#include "DCModeCardButton.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

/** Editable UMG card using the already imported Lyra/CommonUI button contract. */
UCLASS(Abstract, Blueprintable)
class DREAMCATCHER_API UDCModeCardButton : public UDCButtonBase
{
	GENERATED_BODY()
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& Event) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& Event) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	FText ModeNumber;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	FText ModeLabel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	FText ModeTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card", meta=(MultiLine=true))
	FText ModeDescription;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	TObjectPtr<UTexture2D> Artwork;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mode Card")
	FLinearColor AccentColor = FLinearColor(.55f, .22f, 1.f, 1.f);

	// Composed card textures already contain the labels and outline.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> CardOutline;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> ArtworkImage;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> NumberText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DescriptionText;
private:
	void RefreshHighlight();
	bool bInFocusPath = false;
};
