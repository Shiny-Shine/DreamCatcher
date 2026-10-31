#include "UI/Stage/DCMissionListData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace DCMissionData
{
bool DefinitionIsValid(const FDCMissionDefinition& Value)
{
	if (Value.MissionId.IsNone() || Value.DisplayText.IsEmpty()) { return false; }
	switch (Value.ProgressSource)
	{
	case EDCMissionProgressSource::CurrentEncounterDefeats: return true;
	case EDCMissionProgressSource::Guidance: return !Value.TargetKey.IsNone();
	case EDCMissionProgressSource::External: return Value.TargetCount > 0;
	default: return false;
	}
}
}

bool FDCMissionHUDStyle::IsValid() const
{
	return FMath::IsFinite(CompletedHoldDuration) && CompletedHoldDuration >= 0.f
		&& FMath::IsFinite(MissionTransitionDuration) && MissionTransitionDuration > 0.f;
}

const FDCMissionDefinition* UDCMissionListData::FindEnabledMission(FName MissionId) const
{
	if (MissionId.IsNone()) { return nullptr; }
	const FDCMissionDefinition* Found = nullptr;
	for (const FDCMissionDefinition& Definition : Missions)
	{
		if (Definition.MissionId != MissionId) { continue; }
		if (Found) { return nullptr; } // A duplicate ID is ambiguous even if one copy is disabled.
		Found = &Definition;
	}
	return Found && Found->bEnabled && DCMissionData::DefinitionIsValid(*Found) ? Found : nullptr;
}

bool UDCMissionListData::ValidateDefinitions(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();
	TSet<FName> Seen;
	for (int32 Index = 0; Index < Missions.Num(); ++Index)
	{
		const FDCMissionDefinition& Definition = Missions[Index];
		if (Definition.MissionId.IsNone() || Seen.Contains(Definition.MissionId))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("Mission[%d] requires a non-empty unique MissionId."), Index)));
		}
		Seen.Add(Definition.MissionId);
		if (!DCMissionData::DefinitionIsValid(Definition))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("Mission[%d] requires display text, a supported source, a Guidance TargetKey, and a positive External TargetCount."), Index)));
		}
	}
	if (!DefaultTrackedMissionId.IsNone() && !FindEnabledMission(DefaultTrackedMissionId))
	{
		OutErrors.Add(FText::FromString(TEXT("DefaultTrackedMissionId must identify one enabled valid mission.")));
	}
	return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UDCMissionListData::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateDefinitions(Errors);
	if (!HUDStyle.IsValid()) { Errors.Add(FText::FromString(TEXT("Mission HUD transition timing is invalid; the HUD will use its safe timing defaults."))); }
	for (const FText& Error : Errors) { Context.AddError(Error); }
	return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty()
		? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
