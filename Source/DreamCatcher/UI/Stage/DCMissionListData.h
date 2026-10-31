#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DCMissionListData.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EDCMissionProgressSource : uint8
{
	CurrentEncounterDefeats,
	Guidance,
	External
};

/** Ordered authoring data. Never contains level actors or runtime progress. */
USTRUCT(BlueprintType)
struct FDCMissionDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName MissionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FText DisplayText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	EDCMissionProgressSource ProgressSource = EDCMissionProgressSource::Guidance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission", meta=(ClampMin="1", EditCondition="ProgressSource == EDCMissionProgressSource::External"))
	int32 TargetCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName TargetKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct FDCMissionProgress
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Mission") FName MissionId;
	UPROPERTY(BlueprintReadOnly, Category="Mission") FText DisplayText;
	UPROPERTY(BlueprintReadOnly, Category="Mission") EDCMissionProgressSource ProgressSource = EDCMissionProgressSource::Guidance;
	UPROPERTY(BlueprintReadOnly, Category="Mission") int32 CurrentCount = 0;
	UPROPERTY(BlueprintReadOnly, Category="Mission") int32 TargetCount = 0;
	UPROPERTY(BlueprintReadOnly, Category="Mission") bool bCompleted = false;
	UPROPERTY(BlueprintReadOnly, Category="Mission") bool bTracked = false;
};

/** Per-phase behavior and row class. Visual layout belongs to Widget Blueprints. */
USTRUCT(BlueprintType)
struct FDCMissionHUDStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(Units="s", ClampMin="0.0")) float CompletedHoldDuration = .6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(Units="s")) float MissionTransitionDuration = .4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Entries") TSubclassOf<UUserWidget> EntryWidgetClass;

	bool IsValid() const;
};

/** Final ordered presentation values, captured before the live phase resets. */
USTRUCT(BlueprintType)
struct FDCMissionTransitionSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Mission") int32 PhaseIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category="Mission") FText PhaseTitle;
	UPROPERTY(BlueprintReadOnly, Category="Mission") FDCMissionHUDStyle HUDStyle;
	UPROPERTY(BlueprintReadOnly, Category="Mission") TArray<FDCMissionProgress> Missions;
};

UCLASS(BlueprintType)
class DREAMCATCHER_API UDCMissionListData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission", meta=(TitleProperty="MissionId"))
	TArray<FDCMissionDefinition> Missions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName DefaultTrackedMissionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|HUD")
	FDCMissionHUDStyle HUDStyle;

	UFUNCTION(BlueprintPure, Category="Mission")
	bool ValidateDefinitions(TArray<FText>& OutErrors) const;
	/** Ambiguous/disabled/malformed definitions are never runtime mission identities. */
	const FDCMissionDefinition* FindEnabledMission(FName MissionId) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
