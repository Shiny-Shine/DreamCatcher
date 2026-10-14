#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DCStageUIBlueprintLibrary.generated.h"

/** Explicit adapter for the R13 copy of the original in-game menu. */
UCLASS()
class DREAMCATCHER_API UDCStageUIBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Stage|UI", meta=(WorldContext="WorldContextObject"))
	static bool ReturnFromStageMenu(const UObject* WorldContextObject);
};
