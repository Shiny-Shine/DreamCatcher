// Isolated transient fixture. No production assets, saves, world instances or global consolidation.
#include "DCRifleScopedCopy.h"
#include "DCRifleDiagnosticGuard.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BlueprintEditorSettings.h"
#include "CoreGlobals.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Tunnel.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleScopedCopyTest,
	"DreamCatcher.MigrationTools.ExplicitCopy.ScopedReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCRifleScopedCopyTest::RunTest(const FString& Parameters)
{
	// Registered as EditorContext: -ExecCmds Automation runs in a headless editor,
	// not a commandlet. This fixture never invokes the public real-asset copy entry point;
	// that entry point retains its separate IsRunningCommandlet requirement.
	if (!GIsEditor || IsRunningCommandlet() || !IsInGameThread() || GIsPlayInEditorWorld
		|| !FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))
		|| GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCRifleCopyDiagnostics")))
	{
		AddError(TEXT("Use separate headless Editor Automation (-unattended -nullrhi -DCRifleCopyDiagnostics), on the game thread, outside PIE, with Save on Compile = Never; not a commandlet.")); return false;
	}
	// DuplicateSingleObject rejects the read-only /Temp mount. A unique /Game name
	// satisfies that API without saving: fixture sources remain RF_Transient, compile
	// auto-save is disabled above, and source/copy file absence is asserted below.
	const FString Root = TEXT("/Game/LyraMigration/Rifle/Diagnostics/Explicit/ScopedFixture_")
		+ FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FText RootReason;
	if (!TestTrue(TEXT("Fixture root supports native asset duplication"),
		FPackageName::IsValidLongPackageName(Root, false, &RootReason)))
	{
		AddInfo(RootReason.ToString());
		return false;
	}
	const auto Create = [&Root](const TCHAR* Name, UClass* Parent, EBlueprintType Type)
	{
		UPackage* Package = CreatePackage(*(Root / TEXT("Source") / Name));
		Package->SetFlags(RF_Transient);
		return FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(Name), Type);
	};
	const auto Compile = [](UBlueprint* BP)
	{
		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection);
	};
	DCRifleMigration::FEngineDiagnosticGuard Diagnostics;
	TArray<FString> DiagnosticMessages;
	TStrongObjectPtr<UBlueprint> Parent(Create(TEXT("BP_Weapon"), AActor::StaticClass(), BPTYPE_Normal));
	if (!TestNotNull(TEXT("Parent"), Parent.Get())) { return false; }
	// A connected latent EventGraph forces a persistent parent frame. The previous
	// disconnected-call fixture did not cover the real WeaponInstance CDO failure.
	if (Parent->UbergraphPages.IsEmpty()) { AddError(TEXT("Parent EventGraph missing")); return false; }
	UEdGraph* ParentGraph = Parent->UbergraphPages[0];
	UK2Node_CustomEvent* Event = NewObject<UK2Node_CustomEvent>(ParentGraph);
	ParentGraph->AddNode(Event, false, false);
	Event->CreateNewGuid();
	Event->CustomFunctionName = TEXT("FrameProbe");
	Event->AllocateDefaultPins();
	FEdGraphPinType TextType;
	TextType.PinCategory = UEdGraphSchema_K2::PC_String;
	UEdGraphPin* EventText = Event->CreateUserDefinedPin(TEXT("ProbeText"), TextType, EGPD_Output);
	const auto AddCall = [ParentGraph](const TCHAR* FunctionName)
	{
		UFunction* Function = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FunctionName);
		if (!Function) { return static_cast<UK2Node_CallFunction*>(nullptr); }
		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(ParentGraph);
		ParentGraph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->SetFromFunction(Function);
		Node->AllocateDefaultPins();
		return Node;
	};
	UK2Node_CallFunction* Delay = AddCall(TEXT("Delay"));
	UK2Node_CallFunction* Print = AddCall(TEXT("PrintString"));
	if (!TestNotNull(TEXT("Delay function"), Delay) || !TestNotNull(TEXT("Print function"), Print)) { return false; }
	if (!Event->FindPin(UEdGraphSchema_K2::PN_Then) || !Delay->FindPin(UEdGraphSchema_K2::PN_Execute)
		|| !Delay->FindPin(UEdGraphSchema_K2::PN_Then) || !Print->FindPin(UEdGraphSchema_K2::PN_Execute) || !Print->FindPin(TEXT("InString")))
	{
		AddError(TEXT("Fixture event/delay/consumer pins missing.")); return false;
	}
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if (!TestNotNull(TEXT("Persistent text parameter"), EventText)
		|| !TestTrue(TEXT("Event executes delay"), Schema->TryCreateConnection(Event->FindPin(UEdGraphSchema_K2::PN_Then), Delay->FindPin(UEdGraphSchema_K2::PN_Execute)))
		|| !TestTrue(TEXT("Delay executes consumer"), Schema->TryCreateConnection(Delay->FindPin(UEdGraphSchema_K2::PN_Then), Print->FindPin(UEdGraphSchema_K2::PN_Execute)))
		|| !TestTrue(TEXT("Text survives latent frame"), Schema->TryCreateConnection(EventText, Print->FindPin(TEXT("InString"))))) { return false; }
	Compile(Parent.Get());
	UBlueprintGeneratedClass* ParentClass = Cast<UBlueprintGeneratedClass>(Parent->GeneratedClass.Get());
	if (!TestNotNull(TEXT("Parent generated class"), ParentClass)
		|| !TestNotNull(TEXT("Parent owns UberGraph function"), ParentClass->UberGraphFunction.Get())
		|| !TestNotNull(TEXT("Parent owns persistent frame property"), ParentClass->UberGraphFramePointerProperty)) { return false; }
	TStrongObjectPtr<UBlueprint> Child(Create(TEXT("BP_Rifle"), Parent->GeneratedClass, BPTYPE_Normal));
	TStrongObjectPtr<UBlueprint> Sibling(Create(TEXT("BP_Pistol"), Parent->GeneratedClass, BPTYPE_Normal));
	TStrongObjectPtr<UBlueprint> Macro(Create(TEXT("WeaponMacros"), UObject::StaticClass(), BPTYPE_MacroLibrary));
	TStrongObjectPtr<UBlueprint> Function(Create(TEXT("WeaponFunctions"), UBlueprintFunctionLibrary::StaticClass(), BPTYPE_FunctionLibrary));
	if (!TestNotNull(TEXT("Child"), Child.Get()) || !TestNotNull(TEXT("Unselected sibling"), Sibling.Get())
		|| !TestNotNull(TEXT("Macro"), Macro.Get()) || !TestNotNull(TEXT("Function library"), Function.Get())) { return false; }
	Compile(Child.Get()); Compile(Sibling.Get());
	UEdGraph* MacroGraph = FBlueprintEditorUtils::CreateNewGraph(Macro.Get(), TEXT("GetWeapon"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddMacroGraph(Macro.Get(), MacroGraph, true, nullptr);
	UK2Node_Tunnel* Entry = nullptr;
	UK2Node_Tunnel* Exit = nullptr;
	for (UEdGraphNode* Node : MacroGraph->Nodes)
	{
		if (UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node))
		{
			if (Tunnel->bCanHaveOutputs) { Entry = Tunnel; }
			if (Tunnel->bCanHaveInputs) { Exit = Tunnel; }
		}
	}
	if (!TestNotNull(TEXT("Macro entry"), Entry) || !TestNotNull(TEXT("Macro exit"), Exit)) { return false; }
	FEdGraphPinType WeaponType;
	WeaponType.PinCategory = UEdGraphSchema_K2::PC_Object;
	WeaponType.PinSubCategoryObject = Parent->GeneratedClass;
	UEdGraphPin* Input = Entry->CreateUserDefinedPin(TEXT("WeaponIn"), WeaponType, EGPD_Output);
	UEdGraphPin* Output = Exit->CreateUserDefinedPin(TEXT("WeaponOut"), WeaponType, EGPD_Input);
	if (!TestNotNull(TEXT("Typed input"), Input) || !TestNotNull(TEXT("Typed output"), Output)) { return false; }
	Input->MakeLinkTo(Output);
	Compile(Macro.Get());
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(Function.Get(), TEXT("UseWeapon"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph(Function.Get(), FunctionGraph, true, static_cast<UClass*>(nullptr));
	UK2Node_MacroInstance* Instance = NewObject<UK2Node_MacroInstance>(FunctionGraph);
	FunctionGraph->AddNode(Instance, false, false);
	Instance->CreateNewGuid();
	Instance->SetMacroGraph(MacroGraph);
	Instance->AllocateDefaultPins();
	Compile(Function.Get());
	UFunction* UseWeapon = Function->GeneratedClass->FindFunctionByName(TEXT("UseWeapon"));
	if (!TestNotNull(TEXT("Library function generated"), UseWeapon) || Child->UbergraphPages.IsEmpty()) { return false; }
	UEdGraph* ChildGraph = Child->UbergraphPages[0];
	UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(ChildGraph);
	ChildGraph->AddNode(Call, false, false);
	Call->CreateNewGuid();
	Call->SetFromFunction(UseWeapon);
	Call->AllocateDefaultPins();
	Compile(Child.Get());
	// Reacquire classes/defaults after compilation rather than retaining a stale frame.
	ParentClass = Cast<UBlueprintGeneratedClass>(Parent->GeneratedClass.Get());
	if (!ParentClass || !ParentClass->UberGraphFramePointerProperty || !Child->GeneratedClass)
	{
		AddError(TEXT("Fixture persistent parent frame unavailable after compile.")); return false;
	}
	FPointerToUberGraphFrame* SourceFrame = ParentClass->UberGraphFramePointerProperty->ContainerPtrToValuePtr<FPointerToUberGraphFrame>(Child->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Child CDO owns an inherited persistent frame"), SourceFrame->RawPointer)) { return false; }
	CastChecked<AActor>(Child->GeneratedClass->GetDefaultObject())->Tags.Add(TEXT("PreserveAfterReparent"));
	TMap<FString, FString> Map;
	for (UBlueprint* BP : { Parent.Get(), Child.Get(), Macro.Get(), Function.Get() })
	{
		Map.Add(BP->GetOutermost()->GetName(), Root / TEXT("Copy") / BP->GetName());
	}
	// All objects are our own transient fixture, not user packages.
	for (UBlueprint* BP : { Parent.Get(), Child.Get(), Sibling.Get(), Macro.Get(), Function.Get() })
	{
		BP->GetOutermost()->SetDirtyFlag(false);
	}
	UClass* OriginalParent = Sibling->ParentClass;
	UClass* OriginalGenerated = Sibling->GeneratedClass;
	DCRifleMigration::FOriginalBlueprintGuard Guard;
	TArray<FString> Messages;
	if (!Diagnostics.Check(TEXT("after_frame_fixture"), DiagnosticMessages))
	{
		for (const FString& Message : DiagnosticMessages) { AddError(Message); }
		return false;
	}
	// The engine warns once for reparenting our child to a separately duplicated
	// parent. Expect exactly that warning, not ensures/errors or arbitrary warnings.
	AddExpectedMessagePlain(TEXT("class hierarchy is changing, there could be possible data loss!"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	if (!TestTrue(TEXT("Bounded native copy with macro/function type refs"), DCRifleMigration::CopyScoped(Map, Guard, Messages)))
	{
		for (const FString& Message : Messages) { AddInfo(Message); }
		return false;
	}
	TestTrue(TEXT("All original BP states unchanged"), Guard.Verify(Messages));
	TestEqual(TEXT("Unselected sibling retains original parent"), Sibling->ParentClass.Get(), OriginalParent);
	TestEqual(TEXT("Unselected sibling not regenerated"), Sibling->GeneratedClass.Get(), OriginalGenerated);
	TestFalse(TEXT("Unselected sibling not dirtied"), Sibling->GetOutermost()->IsDirty());
	TestTrue(TEXT("Copied macro owner/graph and all pin types match"), DCRifleMigration::CompareTypeReferences(Map, Messages));
	const FString ParentPackage = Map.FindChecked(Parent->GetOutermost()->GetName());
	const FString ChildPackage = Map.FindChecked(Child->GetOutermost()->GetName());
	UBlueprint* CopiedParent = FindObject<UBlueprint>(nullptr, *(ParentPackage + TEXT(".") + Parent->GetName()));
	UBlueprint* CopiedChild = FindObject<UBlueprint>(nullptr, *(ChildPackage + TEXT(".") + Child->GetName()));
	if (!TestNotNull(TEXT("Copied parent"), CopiedParent) || !TestNotNull(TEXT("Copied child"), CopiedChild)) { return false; }
	UBlueprintGeneratedClass* CopiedParentClass = Cast<UBlueprintGeneratedClass>(CopiedParent->GeneratedClass.Get());
	if (!TestNotNull(TEXT("Copied parent class"), CopiedParentClass)
		|| !TestNotNull(TEXT("Copied parent frame property"), CopiedParentClass->UberGraphFramePointerProperty)) { return false; }
	UObject* ChildCDO = CopiedChild->GeneratedClass->GetDefaultObject();
	FPointerToUberGraphFrame* Frame = CopiedParentClass->UberGraphFramePointerProperty->ContainerPtrToValuePtr<FPointerToUberGraphFrame>(ChildCDO);
	TestNotNull(TEXT("Copied child retains persistent frame"), Frame->RawPointer);
#if VALIDATE_UBER_GRAPH_PERSISTENT_FRAME
	TestEqual(TEXT("Copied child frame key matches new parent"), Frame->UberGraphFunctionKey, CopiedParentClass->UberGraphFunctionKey);
#endif
	TestTrue(TEXT("Child CDO default preserved across reparent"), CastChecked<AActor>(ChildCDO)->Tags.Contains(TEXT("PreserveAfterReparent")));
	const FString CopiedFunctionPackage = Map.FindChecked(Function->GetOutermost()->GetName());
	UBlueprint* CopiedFunction = FindObject<UBlueprint>(nullptr, *(CopiedFunctionPackage + TEXT(".") + Function->GetName()));
	if (!TestNotNull(TEXT("Copied function library"), CopiedFunction)) { return false; }
	UK2Node_MacroInstance* CopiedMacroCall = nullptr;
	for (UEdGraph* Graph : CopiedFunction->FunctionGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_MacroInstance* MacroCall = Cast<UK2Node_MacroInstance>(Node)) { CopiedMacroCall = MacroCall; }
		}
	}
	if (!TestNotNull(TEXT("Original macro node retained in copied function graph"), CopiedMacroCall)) { return false; }
	UEdGraph* CopiedMacroGraph = CopiedMacroCall->GetMacroGraph();
	if (!TestNotNull(TEXT("Copied macro graph resolved"), CopiedMacroGraph)) { return false; }
	TestEqual(TEXT("Macro graph points to copied library"), CopiedMacroGraph->GetOutermost()->GetName(), Map.FindChecked(Macro->GetOutermost()->GetName()));
	CopiedMacroCall->SetMacroGraph(MacroGraph);
	TestFalse(TEXT("Mixed original macro reference detected"), DCRifleMigration::CompareTypeReferences(Map, Messages));
	CopiedMacroCall->SetMacroGraph(CopiedMacroGraph);
	TestTrue(TEXT("Macro fixture restored"), DCRifleMigration::CompareTypeReferences(Map, Messages));
	for (const TPair<FString, FString>& Pair : Map)
	{
		TestFalse(TEXT("No source file saved"), FPackageName::DoesPackageExist(Pair.Key));
		TestFalse(TEXT("No copy file saved"), FPackageName::DoesPackageExist(Pair.Value));
	}
	// Negative test: a changed original must fail, not be silently repaired/marked clean.
	Sibling->ParentClass = AActor::StaticClass();
	TestFalse(TEXT("Deliberate original parent change detected"), Guard.Verify(Messages));
	Sibling->ParentClass = OriginalParent;
	TestTrue(TEXT("Fixture baseline restored"), Guard.Verify(Messages));
	DCRifleMigration::FOriginalBlueprintGuard SecondGuard;
	TestFalse(TEXT("Existing destination is refused, never overwritten"), DCRifleMigration::CopyScoped(Map, SecondGuard, Messages));
	TestTrue(TEXT("No engine ensures/errors across fixture and native copy"), Diagnostics.Check(TEXT("fixture_complete"), DiagnosticMessages));
	return !HasAnyErrors();
}

#endif
