// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCLyraCircumferenceMarkerWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraCircumferenceMarkerWidget)

class SWidget;

UDCLyraCircumferenceMarkerWidget::UDCLyraCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
	// DreamCatcher: make the original zero-initialized UMG default explicit.
	bReticleCornerOutsideSpreadRadius = false;
}

void UDCLyraCircumferenceMarkerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMarkerWidget.Reset();
}

TSharedRef<SWidget> UDCLyraCircumferenceMarkerWidget::RebuildWidget()
{
	// Preserve Lyra's UMG bridge: its OutsideRadius property was not forwarded.
	// Slate keeps a deterministic false default rather than the source's uninitialized bit.
	MyMarkerWidget = SNew(SDCLyraCircumferenceMarkerWidget)
		.MarkerBrush(&MarkerImage)
		.Radius(this->Radius)
		.MarkerList(this->MarkerList);

	return MyMarkerWidget.ToSharedRef();
}

void UDCLyraCircumferenceMarkerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyMarkerWidget->SetRadius(Radius);
	MyMarkerWidget->SetMarkerList(MarkerList);
}

void UDCLyraCircumferenceMarkerWidget::SetRadius(float InRadius)
{
	Radius = InRadius;
	if (MyMarkerWidget.IsValid())
	{
		MyMarkerWidget->SetRadius(InRadius);
	}
}
