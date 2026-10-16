// Editor-only asset preparation; no engine patches and no asset saves here.
#if WITH_EDITOR

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "Animation/BlendProfile.h"
#include "Animation/Skeleton.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Engine/Blueprint.h"
#include "HAL/IConsoleManager.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace DCHaruDashMontageRepair
{
void PrepareProfileAndMontageInput()
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCHaruDashRepair")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR requires unattended -DCHaruDashRepair."));
		return;
	}
	USkeleton* Source = LoadObject<USkeleton>(nullptr, TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin"));
	USkeleton* Target = LoadObject<USkeleton>(nullptr, TEXT("/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton.하루리깅_Skeleton"));
	UBlueprint* Ability = LoadObject<UBlueprint>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Dash/GA_Hero_Dash.GA_Hero_Dash"));
	UBlendProfile* Original = Source ? Source->GetBlendProfile(TEXT("FastFeet")) : nullptr;
	if (!Original || !Target || Target == Source || !Ability || Target->GetBlendProfile(TEXT("FastFeet_Haru")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR expected original profile and a target without FastFeet_Haru."));
		return;
	}
	TArray<UEdGraph*> Graphs;
	Ability->GetAllGraphs(Graphs);
	UEdGraphPin* MontageInput = nullptr;
	int32 ConnectedMontageInputs = 0;
	for (UEdGraph* Graph : Graphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
			{
				if (Call->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UDCGameplayAbility, GetMontageForAvatarSkeleton))
				{
					UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR resolver already exists; refusing duplicate."));
					return;
				}
			}
			if (UEdGraphPin* Pin = Node->FindPin(TEXT("MontageToPlay"), EGPD_Input))
			{
				if (Pin->LinkedTo.Num() == 1)
				{
					MontageInput = Pin;
					++ConnectedMontageInputs;
				}
			}
		}
	}
	if (ConnectedMontageInputs != 1 || !MontageInput)
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR expected one connected montage task input; found %d."), ConnectedMontageInputs);
		return;
	}
	UFunction* ResolveFunction = UDCGameplayAbility::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UDCGameplayAbility, GetMontageForAvatarSkeleton));
	if (!ResolveFunction)
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR missing compiled resolver."));
		return;
	}
	Target->Modify();
	UBlendProfile* Compatible = Target->CreateNewBlendProfile(TEXT("FastFeet_Haru"));
	if (!Compatible)
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR failed to create profile; do not save."));
		return;
	}
	Compatible->Mode = Original->Mode;
	int32 Retained = 0;
	for (const FBlendProfileBoneEntry& Entry : Original->ProfileEntries)
	{
		if (Target->GetReferenceSkeleton().FindBoneIndex(Entry.BoneReference.BoneName) != INDEX_NONE)
		{
			Compatible->SetBoneBlendScale(Entry.BoneReference.BoneName, Entry.BlendScale, false, true);
			++Retained;
		}
	}
	Target->MarkPackageDirty();
	UEdGraph* Graph = MontageInput->GetOwningNode()->GetGraph();
	UEdGraphPin* OriginalOutput = MontageInput->LinkedTo[0];
	Ability->Modify();
	Graph->Modify();
	FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
	UK2Node_CallFunction* Resolver = Creator.CreateNode();
	Resolver->SetFromFunction(ResolveFunction);
	Resolver->FunctionReference.SetSelfMember(GET_FUNCTION_NAME_CHECKED(UDCGameplayAbility, GetMontageForAvatarSkeleton));
	Resolver->NodePosX = MontageInput->GetOwningNode()->NodePosX - 280;
	Resolver->NodePosY = MontageInput->GetOwningNode()->NodePosY + 240;
	Resolver->NodeComment = TEXT("Select the authored montage for the current avatar skeleton.");
	Creator.Finalize();
	UEdGraphPin* ResolverInput = Resolver->FindPin(TEXT("SourceMontage"), EGPD_Input);
	UEdGraphPin* ResolverOutput = Resolver->FindPin(TEXT("ReturnValue"), EGPD_Output);
	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (!ResolverInput || !ResolverOutput || !Schema || !Schema->TryCreateConnection(OriginalOutput, ResolverInput))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR failed input connection; discard without saving."));
		return;
	}
	MontageInput->BreakLinkTo(OriginalOutput);
	if (!Schema->TryCreateConnection(ResolverOutput, MontageInput))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_DASH_REPAIR failed output connection; discard without saving."));
		return;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Ability);
	UE_LOG(LogTemp, Display, TEXT("HARU_DASH_REPAIR_IN_MEMORY_OK retained=%d source_entries=%d graph_nodes_added=1 saves=0"), Retained, Original->GetNumBlendEntries());
}

FAutoConsoleCommand PrepareCommand(
	TEXT("DC.HaruDash.Prepare"),
	TEXT("Prepare HARU FastFeet and the dash montage resolver; requires -DCHaruDashRepair. Never saves assets."),
	FConsoleCommandDelegate::CreateStatic(&PrepareProfileAndMontageInput));
}

#endif // WITH_EDITOR
