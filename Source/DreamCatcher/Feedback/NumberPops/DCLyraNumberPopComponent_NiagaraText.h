// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DCLyraNumberPopComponent.h"

#include "DCLyraNumberPopComponent_NiagaraText.generated.h"

class UDCLyraDamagePopStyleNiagara;
class UNiagaraComponent;
class UObject;

UCLASS(Blueprintable)
class DREAMCATCHER_API UDCLyraNumberPopComponent_NiagaraText : public UDCLyraNumberPopComponent
{
	GENERATED_BODY()

public:

	UDCLyraNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UDCLyraNumberPopComponent interface
	virtual void AddNumberPop(const FDCLyraNumberPopRequest& NewRequest) override;
	//~End of UDCLyraNumberPopComponent interface

protected:
	
	TArray<int32> DamageNumberArray;

	/** Style patterns to attempt to apply to the incoming number pops */
	UPROPERTY(EditDefaultsOnly, Category = "Number Pop|Style")
	TObjectPtr<UDCLyraDamagePopStyleNiagara> Style;

	//Niagara Component used to display the damage
	UPROPERTY(EditDefaultsOnly, Category = "Number Pop|Style")
	TObjectPtr<UNiagaraComponent> NiagaraComp;
};
