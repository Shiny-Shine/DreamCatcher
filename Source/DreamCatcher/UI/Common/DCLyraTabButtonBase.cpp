// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/Common/DCLyraTabButtonBase.h"

#include "CommonLazyImage.h"
#include "UI/Common/DCLyraTabListWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCLyraTabButtonBase)

class UObject;
struct FSlateBrush;

void UDCLyraTabButtonBase::SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrushFromLazyDisplayAsset(LazyObject);
	}
}

void UDCLyraTabButtonBase::SetIconBrush(const FSlateBrush& Brush)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrush(Brush);
	}
}

void UDCLyraTabButtonBase::SetTabLabelInfo_Implementation(const FDCLyraTabDescriptor& TabLabelInfo)
{
	SetButtonText(TabLabelInfo.TabText);
	SetIconBrush(TabLabelInfo.IconBrush);
}

