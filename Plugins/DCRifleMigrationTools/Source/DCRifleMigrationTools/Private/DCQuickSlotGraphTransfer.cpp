// One-off R7 bridge to engine graph duplication. No gameplay implementation.
#include "DCRifleMigrationLibrary.h"
#include "DCRifleDiagnosticGuard.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Pawn.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace DCQuickSlotTransfer
{
constexpr const TCHAR* OriginalHero = TEXT("/ShooterCore/Game/B_Hero_ShooterMannequin");
constexpr const TCHAR* StagingPawn = TEXT("/Game/LyraMigration/UI/R7/Equipment/Diagnostics/BP_QuickSlotInputSource");
constexpr const TCHAR* IntegrationPawn = TEXT("/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn");
constexpr const TCHAR* PreviewPawn = TEXT("/Game/LyraMigration/UI/R7/Equipment/Diagnostics/BP_DC_RiflePawn_QuickSlotPreview");
const FName FunctionName(TEXT("ChangeQuickbarSlot"));
const FName InputGraphName(TEXT("QuickSlotInput"));

const TSet<FName>& SenderNodes()
{
	static const TSet<FName> Names = {
		TEXT("K2Node_EnhancedInputAction_1"), TEXT("K2Node_EnhancedInputAction_2"),
		TEXT("K2Node_EnhancedInputAction_3"), TEXT("K2Node_EnhancedInputAction_4"),
		TEXT("K2Node_EnhancedInputAction_5"), TEXT("K2Node_CallFunction_19"),
		TEXT("K2Node_CallFunction_25"), TEXT("K2Node_CallFunction_26"),
		TEXT("K2Node_CallFunction_39"), TEXT("K2Node_CallFunction_41")};
	return Names;
}

UEdGraph* FindGraph(UBlueprint* Blueprint, FName Name)
{
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph && Graph->GetFName() == Name) { return Graph; }
	}
	return nullptr;
}

// No selected pin may reach an omitted node or a graph outside this copy.
bool HasClosedLinks(UEdGraph* Graph, const TSet<FName>* Selection)
{
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node || Node->GetOuter() != Graph) { return false; }
		if (Selection && !Selection->Contains(Node->GetFName())) { continue; }
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->GetOwningNode() != Node) { return false; }
			for (UEdGraphPin* Linked : Pin->LinkedTo)
			{
				UEdGraphNode* Other = Linked ? Linked->GetOwningNode() : nullptr;
				if (!Other || !Graph->Nodes.Contains(Other)
					|| (Selection && !Selection->Contains(Other->GetFName()))) { return false; }
			}
		}
	}
	return true;
}

} // namespace DCQuickSlotTransfer

bool UDCRifleMigrationLibrary::RetargetQuickSlotInputActions(UBlueprint* StagingBlueprint, FString& OutReport)
{
	using namespace DCQuickSlotTransfer;
	if (!ValidateDiagnosticContext(OutReport)) { return false; }
	const auto Reject = [&OutReport](const TCHAR* Reason) { OutReport = Reason; return false; };
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCR7QuickSlotTransfer")) || !IsValid(StagingBlueprint)
		|| StagingBlueprint->GetOutermost()->GetName() != StagingPawn
		|| StagingBlueprint->GetOuter() != StagingBlueprint->GetOutermost())
	{
		return Reject(TEXT("Retargeting is restricted to the fixed staging Blueprint and explicit QuickSlot commandlet."));
	}
	UEdGraph* Graph = FindGraph(StagingBlueprint, InputGraphName);
	if (!Graph || Graph->GetOuter() != StagingBlueprint || Graph->Nodes.Num() != 10 || !HasClosedLinks(Graph, &SenderNodes()))
	{
		return Reject(TEXT("Expected the isolated, named 10-node QuickSlotInput graph."));
	}
	const TCHAR* Suffixes[] = {TEXT("1"), TEXT("2"), TEXT("3"), TEXT("_CycleBackward"), TEXT("_CycleForward")};
	TMap<UObject*, UObject*> Replacements;
	TArray<TPair<UEdGraphNode*, UObject*>> Expected;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FName NodeName(*FString::Printf(TEXT("K2Node_EnhancedInputAction_%d"), Index + 1));
		UEdGraphNode* Node = FindObjectFast<UEdGraphNode>(Graph, NodeName);
		if (!Node || !Graph->Nodes.Contains(Node)
			|| Node->GetClass()->GetPathName() != TEXT("/Script/InputBlueprintNodes.K2Node_EnhancedInputAction"))
		{
			return Reject(TEXT("QuickSlot event node identity changed; no references replaced."));
		}
		const FObjectPropertyBase* Binding = FindFProperty<FObjectPropertyBase>(Node->GetClass(), TEXT("InputAction"));
		UObject* Current = Binding ? Binding->GetObjectPropertyValue_InContainer(Node) : nullptr;
		const FString Name = FString(TEXT("IA_QuickSlot")) + Suffixes[Index];
		const FString OriginalPath = FString(TEXT("/ShooterCore/Input/Actions/")) + Name + TEXT(".") + Name;
		const FString NewPath = FString(TEXT("/Game/LyraMigration/UI/R7/Equipment/Input/Actions/")) + Name + TEXT(".") + Name;
		UObject* Replacement = FindObject<UObject>(nullptr, *NewPath);
		if (!Current || !Replacement || Current->GetClass() != Replacement->GetClass()
			|| Replacement->GetClass()->GetPathName() != TEXT("/Script/EnhancedInput.InputAction")
			|| (Current->GetPathName() != OriginalPath && Current->GetPathName() != NewPath))
		{
			return Reject(TEXT("All five approved original/migrated InputActions must already be loaded; no fallback or Load is performed."));
		}
		const FProperty* ValueType = FindFProperty<FProperty>(Current->GetClass(), TEXT("ValueType"));
		if (!ValueType || !ValueType->Identical_InContainer(Current, Replacement))
		{
			return Reject(TEXT("InputAction value type differs; no reference replacement performed."));
		}
		Replacements.Add(Current, Replacement);
		Expected.Emplace(Node, Replacement);
	}
	DCRifleMigration::FEngineDiagnosticGuard Diagnostics;
	// The archive visits only this new graph tree, not the original actions or Hero.
	FArchiveReplaceObjectRef<UObject> References(Graph, Replacements,
		EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
	for (const TPair<UEdGraphNode*, UObject*>& Pair : Expected)
	{
		const FObjectPropertyBase* Binding = FindFProperty<FObjectPropertyBase>(Pair.Key->GetClass(), TEXT("InputAction"));
		UEdGraphPin* Literal = Pair.Key->FindPin(TEXT("InputAction"));
		if (!Binding || Binding->GetObjectPropertyValue_InContainer(Pair.Key) != Pair.Value
			|| !Literal || Literal->DefaultObject != Pair.Value)
		{
			return Reject(TEXT("Protected binding/pin remap mismatch. Discard process without saving."));
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(StagingBlueprint);
	if (!Diagnostics.IsClean()) { return Reject(TEXT("Retarget emitted engine diagnostics; do not save.")); }
	OutReport = TEXT("Retargeted five fixed QuickSlot node bindings and pin references in memory; no Compile/save. Compare source graphs and action data before saving.");
	return true;
}

namespace DCQuickSlotTransfer
{
// PostDuplicate may change self-context while the destination skeleton does not
// yet contain the copied function. Restore that original field before remapping.
bool RestoreCallReferences(UEdGraph* Source, UEdGraph* Copy)
{
	for (UEdGraphNode* Node : Copy->Nodes)
	{
		if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
		{
			UK2Node_CallFunction* Original = FindObjectFast<UK2Node_CallFunction>(Source, Call->GetFName());
			if (!Original || !Source->Nodes.Contains(Original)) { return false; }
			Call->FunctionReference = Original->FunctionReference;
		}
	}
	return true;
}
}

bool UDCRifleMigrationLibrary::CopyQuickSlotInputGraphs(
	UBlueprint* SourceBlueprint, UBlueprint* TargetBlueprint, FString& OutReport)
{
	using namespace DCQuickSlotTransfer;
	if (!ValidateDiagnosticContext(OutReport)) { return false; }
	const auto Reject = [&OutReport](const TCHAR* Reason) { OutReport = Reason; return false; };
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCR7QuickSlotTransfer")))
	{
		return Reject(TEXT("Explicit -DCR7QuickSlotTransfer is required. Nothing copied."));
	}
	if (!IsValid(SourceBlueprint) || !IsValid(TargetBlueprint) || SourceBlueprint == TargetBlueprint
		|| !SourceBlueprint->ParentClass || !TargetBlueprint->ParentClass
		|| !SourceBlueprint->ParentClass->IsChildOf(APawn::StaticClass())
		|| !TargetBlueprint->ParentClass->IsChildOf(APawn::StaticClass()))
	{
		return Reject(TEXT("Two distinct, already-loaded Pawn Blueprints are required."));
	}
	const FString SourcePackage = SourceBlueprint->GetOutermost()->GetName();
	const FString TargetPackage = TargetBlueprint->GetOutermost()->GetName();
	const bool bSourceStage = FString(FApp::GetProjectName()) == TEXT("LyraStarterGame")
		&& SourcePackage == OriginalHero && TargetPackage == StagingPawn;
	const bool bTargetStage = FString(FApp::GetProjectName()) == TEXT("DreamCatcher")
		&& SourcePackage == StagingPawn && (TargetPackage == IntegrationPawn || TargetPackage == PreviewPawn);
	if ((!bSourceStage && !bTargetStage) || SourceBlueprint->GetOuter() != SourceBlueprint->GetOutermost()
		|| TargetBlueprint->GetOuter() != TargetBlueprint->GetOutermost())
	{
		return Reject(TEXT("Only original Hero -> fixed staging Pawn -> integration Pawn/preview is approved."));
	}
	const bool bExistingTarget = FPackageName::DoesPackageExist(TargetPackage);
	if ((TargetPackage != IntegrationPawn && bExistingTarget)
		|| (bExistingTarget && TargetBlueprint->GetOutermost()->IsDirty())
		|| SourceBlueprint->GetOutermost()->IsDirty())
	{
		return Reject(TEXT("Staging/preview must be new. Loaded source and existing target must be clean."));
	}
	if (!SourceBlueprint->GeneratedClass || !TargetBlueprint->GeneratedClass
		|| !SourceBlueprint->SkeletonGeneratedClass || !TargetBlueprint->SkeletonGeneratedClass)
	{
		return Reject(TEXT("Blueprint generated/skeleton classes must exist before graph transfer."));
	}
	if (FindGraph(TargetBlueprint, FunctionName) || FindGraph(TargetBlueprint, InputGraphName)
		|| FindObjectFast<UObject>(TargetBlueprint, FunctionName) || FindObjectFast<UObject>(TargetBlueprint, InputGraphName)
		|| TargetBlueprint->GeneratedClass->FindFunctionByName(FunctionName))
	{
		return Reject(TEXT("Destination function/input graph already exists; no replacement or retry in-place."));
	}
	TArray<UEdGraph*> TargetGraphs;
	TargetBlueprint->GetAllGraphs(TargetGraphs);
	for (UEdGraph* Graph : TargetGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->GetClass()->GetPathName() == TEXT("/Script/InputBlueprintNodes.K2Node_EnhancedInputAction"))
			{
				const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Node->GetClass(), TEXT("InputAction"));
				const UObject* Action = Property ? Property->GetObjectPropertyValue_InContainer(Node) : nullptr;
				if (!Property || (Action && Action->GetName().StartsWith(TEXT("IA_QuickSlot"))))
				{
					return Reject(TEXT("Destination already binds QuickSlot or its action cannot be inspected."));
				}
			}
		}
	}
	UEdGraph* SourceFunction = FindGraph(SourceBlueprint, FunctionName);
	UEdGraph* SourceInputs = FindGraph(SourceBlueprint, bSourceStage ? FName(TEXT("EventGraph")) : InputGraphName);
	if (!SourceFunction || !SourceInputs || !SourceBlueprint->FunctionGraphs.Contains(SourceFunction)
		|| !SourceBlueprint->UbergraphPages.Contains(SourceInputs)
		|| SourceFunction->Nodes.Num() != 12 || !SourceFunction->SubGraphs.IsEmpty() || !SourceInputs->SubGraphs.IsEmpty()
		|| !Cast<UEdGraphSchema_K2>(SourceFunction->GetSchema()) || !Cast<UEdGraphSchema_K2>(SourceInputs->GetSchema()))
	{
		return Reject(TEXT("Expected the inspected 12-node function and flat K2 input graph; source changed."));
	}
	TSet<FName> Found;
	for (UEdGraphNode* Node : SourceInputs->Nodes)
	{
		if (Node && SenderNodes().Contains(Node->GetFName())) { Found.Add(Node->GetFName()); }
	}
	if (Found.Num() != 10 || (!bSourceStage && SourceInputs->Nodes.Num() != 10)
		|| !HasClosedLinks(SourceFunction, nullptr) || !HasClosedLinks(SourceInputs, &SenderNodes()))
	{
		return Reject(TEXT("Input node set/links differ from the inspected five isolated event-call pairs."));
	}

	DCRifleMigration::FEngineDiagnosticGuard Diagnostics;
	// Prune only an unsaved scratch clone; never PrepareForCopying/remove/rename on source nodes.
	// UObject itself is abstract in this engine, and K2 PostDuplicate needs a real
	// Blueprint owner with a skeleton. The transient child retains the original
	// function context during scratch duplication; it is never registered/saved.
	const FName ScratchName = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), FName(TEXT("DCQuickSlotScratch")));
	TStrongObjectPtr<UBlueprint> ScratchBlueprint(FKismetEditorUtilities::CreateBlueprint(
		SourceBlueprint->GeneratedClass.Get(), GetTransientPackage(), ScratchName, BPTYPE_Normal));
	if (!ScratchBlueprint.Get() || !Diagnostics.IsClean()
		|| !ScratchBlueprint->GeneratedClass || !ScratchBlueprint->SkeletonGeneratedClass)
	{
		return Reject(TEXT("Transient Blueprint initialization failed. No target graphs attached; do not save."));
	}
	ScratchBlueprint->ClearFlags(RF_Public | RF_Standalone);
	ScratchBlueprint->SetFlags(RF_Transient);
	// CreateBlueprint initializes a default EventGraph. Remove only that transient
	// graph to prevent CloneGraph from colliding with its name.
	if (UEdGraph* EmptyGraph = FindGraph(ScratchBlueprint.Get(), SourceInputs->GetFName()))
	{
		if (EmptyGraph->GetOuter() != ScratchBlueprint.Get())
		{
			return Reject(TEXT("Scratch graph is not locally owned; refusing removal."));
		}
		FBlueprintEditorUtils::RemoveGraph(ScratchBlueprint.Get(), EmptyGraph, EGraphRemoveFlags::MarkTransient);
	}
	if (!Diagnostics.IsClean()) { return Reject(TEXT("Scratch preparation emitted diagnostics; do not save.")); }
	UEdGraph* ScratchInputs = FEdGraphUtilities::CloneGraph(SourceInputs, ScratchBlueprint.Get());
	if (!ScratchInputs) { return Reject(TEXT("Scratch clone failed. Discard this process without saving.")); }
	const TArray<TObjectPtr<UEdGraphNode>> ScratchNodes = ScratchInputs->Nodes;
	for (UEdGraphNode* Node : ScratchNodes)
	{
		if (!SenderNodes().Contains(Node->GetFName()))
		{
			Node->BreakAllNodeLinks();
			ScratchInputs->Nodes.Remove(Node);
		}
	}
	if (!ScratchInputs->Rename(*InputGraphName.ToString(), nullptr, REN_DontCreateRedirectors | REN_NonTransactional)
		|| ScratchInputs->Nodes.Num() != 10 || !Diagnostics.IsClean())
	{
		return Reject(TEXT("Scratch input extraction failed. No target graphs attached; do not save."));
	}
	UEdGraph* NewFunction = FEdGraphUtilities::CloneGraph(SourceFunction, TargetBlueprint);
	UEdGraph* NewInputs = FEdGraphUtilities::CloneGraph(ScratchInputs, TargetBlueprint);
	if (!NewFunction || !NewInputs) { return Reject(TEXT("Target clone failed. Discard this process without saving.")); }
	if (!RestoreCallReferences(SourceFunction, NewFunction) || !RestoreCallReferences(SourceInputs, NewInputs))
	{
		return Reject(TEXT("Original call-reference restoration failed. Discard this process without saving."));
	}
	// Restrict self-class adaptation to the two new graph object trees only.
	TMap<UObject*, UObject*> SelfReferences;
	SelfReferences.Add(SourceBlueprint, TargetBlueprint);
	SelfReferences.Add(SourceBlueprint->GeneratedClass.Get(), TargetBlueprint->GeneratedClass.Get());
	SelfReferences.Add(SourceBlueprint->SkeletonGeneratedClass.Get(), TargetBlueprint->SkeletonGeneratedClass.Get());
	const EArchiveReplaceObjectFlags Flags = EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef;
	FArchiveReplaceObjectRef<UObject> FunctionReferences(NewFunction, SelfReferences, Flags);
	FArchiveReplaceObjectRef<UObject> InputReferences(NewInputs, SelfReferences, Flags);
	if (!Diagnostics.IsClean() || !HasClosedLinks(NewFunction, nullptr) || !HasClosedLinks(NewInputs, nullptr)
		|| NewFunction->Nodes.Num() != 12 || NewInputs->Nodes.Num() != 10)
	{
		return Reject(TEXT("Copied graph/engine diagnostics failed. Discard the process without saving."));
	}
	// The function already contains its original entry/signature: do not create another entry node.
	TargetBlueprint->Modify();
	TargetBlueprint->FunctionGraphs.Add(NewFunction);
	TargetBlueprint->UbergraphPages.Add(NewInputs);
	FBlueprintEditorUtils::MarkBlueprintAsModified(TargetBlueprint);
	if (!Diagnostics.IsClean() || SourceBlueprint->GetOutermost()->IsDirty())
	{
		return Reject(TEXT("Post-copy diagnostics/source clean-state failed. Discard the process without saving."));
	}
	OutReport = TEXT("Copied original ChangeQuickbarSlot (12 nodes) and input event/call pairs (10 nodes) in memory. No Compile/save. Compile, compare original pins/defaults, then separately save/reload; gameplay remains unverified.");
	return true;
}
