// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UI/Common/DCLyraTabListWidgetBase.h"
#include "UI/Foundation/DCButtonBase.h"
#include "DCLyraTabButtonBase.generated.h"

#define UE_API DREAMCATCHER_API

class UCommonLazyImage;
class UObject;
struct FFrame;
struct FSlateBrush;

UCLASS(MinimalAPI, Abstract, Blueprintable, meta = (DisableNativeTick))
class UDCLyraTabButtonBase : public UDCButtonBase, public IDCLyraTabButtonInterface
{
	GENERATED_BODY()

public:

	UE_API void SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject);
	UE_API void SetIconBrush(const FSlateBrush& Brush);

protected:

	UFUNCTION()
	UE_API virtual void SetTabLabelInfo_Implementation(const FDCLyraTabDescriptor& TabLabelInfo) override;

private:

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonLazyImage> LazyImage_Icon;
};

#undef UE_API
