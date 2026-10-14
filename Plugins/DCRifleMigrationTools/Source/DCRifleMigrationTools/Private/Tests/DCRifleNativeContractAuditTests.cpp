#include "DCRifleMigrationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EdGraph/EdGraph.h"
#include "BlueprintEditorSettings.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_SetFieldsInStruct.h"
#include "K2Node_StructOperation.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DCRifleNativeAuditTests
{
bool Context(FAutomationTestBase& Test)
{
	if (IsRunningCommandlet() || !GIsEditor || !GEditor || GEditor->PlayWorld || !FApp::IsUnattended()
		|| !FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))
		|| GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCRifleNativeContractAudit"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCRifleNativeContractTests")))
	{
		Test.AddError(TEXT("Use a separate unattended Editor with -DCRifleNativeContractAudit -DCRifleNativeContractTests."));
		return false;
	}
	return true;
}
const FDCRifleNativeContractRow* Find(const FDCRifleNativeContractReport& Report, const TCHAR* Kind, const TCHAR* Name)
{
	return Report.Rows.FindByPredicate([Kind, Name](const FDCRifleNativeContractRow& R) { return R.Kind == Kind && R.Name == Name; });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeSchemaTest, "DreamCatcher.MigrationTools.NativeContract.NativeSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeSchemaTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeAuditTests;
	if (!Context(*this)) { return false; }
	const bool bDirty = AActor::StaticClass()->GetOutermost()->IsDirty();
	const auto Report = UDCRifleMigrationLibrary::InspectNativeTypeContract(AActor::StaticClass());
	TestTrue(TEXT("Native Actor schema is supported"), Report.bSucceeded);
	for (const FString& Message : Report.Messages) { AddInfo(Message); }
	TestNotNull(TEXT("Reflected function names included"), Find(Report, TEXT("function"), TEXT("K2_SetActorLocation")));
	const auto* Parameter = Report.Rows.FindByPredicate([](const FDCRifleNativeContractRow& Row)
	{
		return Row.Kind == TEXT("parameter") && Row.Context == TEXT("K2_SetActorLocation") && Row.Name == TEXT("NewLocation");
	});
	if (TestNotNull(TEXT("Function parameters included"), Parameter))
	{
		TestEqual(TEXT("Parameter order"), Parameter->Index, 0);
		TestTrue(TEXT("Struct path retained"), Parameter->Type.Contains(TEXT("/Script/CoreUObject.Vector")));
	}
	const auto* Delegate = Find(Report, TEXT("property"), TEXT("OnActorBeginOverlap"));
	if (TestNotNull(TEXT("Delegate property included"), Delegate))
	{
		TestTrue(TEXT("Delegate signature includes argument names and types, not just its path"),
			Delegate->Type.Contains(TEXT("OtherActor:object(/Script/Engine.Actor)")));
	}
	TestTrue(TEXT("Native package dirty flag unchanged"), bDirty == AActor::StaticClass()->GetOutermost()->IsDirty());
	const auto EnumReport = UDCRifleMigrationLibrary::InspectNativeTypeContract(StaticEnum<EBlueprintType>());
	TestTrue(TEXT("Native enum accepted"), EnumReport.bSucceeded);
	TestNotNull(TEXT("Enum member names included"), Find(EnumReport, TEXT("enum_value"), TEXT("BPTYPE_Normal")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeBlueprintReadTest, "DreamCatcher.MigrationTools.NativeContract.BlueprintReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeBlueprintReadTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeAuditTests;
	if (!Context(*this)) { return false; }
	const FName Name(*(TEXT("NativeContractFixture_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	TStrongObjectPtr<UBlueprint> BP(FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), GetTransientPackage(), Name, BPTYPE_Normal));
	if (!TestNotNull(TEXT("Transient Blueprint fixture"), BP.Get()) || BP->UbergraphPages.IsEmpty()) { return false; }
	UEdGraph* Graph = BP->UbergraphPages[0];
	UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph);
	Graph->AddNode(Call, false, false);
	Call->CreateNewGuid();
	Call->FunctionReference.SetExternalMember(TEXT("K2_SetActorLocation"), AActor::StaticClass());
	Call->AllocateDefaultPins();
	UK2Node_VariableGet* Variable = NewObject<UK2Node_VariableGet>(Graph);
	Graph->AddNode(Variable, false, false);
	Variable->CreateNewGuid();
	Variable->VariableReference.SetExternalMember(TEXT("bHidden"), AActor::StaticClass());
	Variable->AllocateDefaultPins();
	const bool bDirty = BP->GetOutermost()->IsDirty();
	const auto Status = BP->Status;
	UClass* Generated = BP->GeneratedClass;
	const int32 NodeCount = Graph->Nodes.Num();
	const auto Report = UDCRifleMigrationLibrary::InspectAssetNativeContract(BP.Get());
	TestTrue(TEXT("Authored graph inspected without compiling"), Report.bSucceeded);
	for (const FString& Message : Report.Messages) { AddInfo(Message); }
	TestNotNull(TEXT("Function member name reported"), Find(Report, TEXT("node_function"), TEXT("K2_SetActorLocation")));
	TestNotNull(TEXT("Variable member name reported"), Find(Report, TEXT("node_variable"), TEXT("bHidden")));
	TestTrue(TEXT("Reflected function return parameter reported"), Report.Rows.ContainsByPredicate([](const FDCRifleNativeContractRow& R)
	{
		return R.Kind == TEXT("parameter") && R.Name == TEXT("ReturnValue") && !R.Flags.IsEmpty();
	}));
	TestEqual(TEXT("Nodes not rewritten"), Graph->Nodes.Num(), NodeCount);
	TestTrue(TEXT("Generated class unchanged"), BP->GeneratedClass == Generated);
	TestTrue(TEXT("Compile status unchanged"), BP->Status == Status);
	TestEqual(TEXT("Package dirty flag unchanged"), BP->GetOutermost()->IsDirty(), bDirty);
	Call->FunctionReference.SetExternalMember(TEXT("DC_MissingFixtureFunction"), AActor::StaticClass());
	const auto Missing = UDCRifleMigrationLibrary::InspectAssetNativeContract(BP.Get());
	TestFalse(TEXT("Unresolved function fails closed instead of declaring compatibility"), Missing.bSucceeded);
	TestTrue(TEXT("Failure has evidence"), Missing.Messages.ContainsByPredicate([](const FString& Message) { return Message.Contains(TEXT("Unresolved function")); }));
	TestTrue(TEXT("Failure did not rewrite the reference"), Call->FunctionReference.GetMemberName() == TEXT("DC_MissingFixtureFunction"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeRejectionTest, "DreamCatcher.MigrationTools.NativeContract.RejectedInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeRejectionTest::RunTest(const FString& Parameters)
{
	if (!DCRifleNativeAuditTests::Context(*this)) { return false; }
	TestFalse(TEXT("Null asset refused"), UDCRifleMigrationLibrary::InspectAssetNativeContract(nullptr).bSucceeded);
	TestFalse(TEXT("Null native type refused"), UDCRifleMigrationLibrary::InspectNativeTypeContract(nullptr).bSucceeded);
	// UObject is abstract; reject a valid concrete instance, not a failed allocation.
	TStrongObjectPtr<UEdGraph> Object(NewObject<UEdGraph>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Concrete transient object fixture"), Object.Get())) { return false; }
	TestFalse(TEXT("Object instance is not a native type"), UDCRifleMigrationLibrary::InspectNativeTypeContract(Object.Get()).bSucceeded);
	TestFalse(TEXT("Synthetic asset query cannot inspect a real script package"), UDCRifleMigrationLibrary::InspectAssetNativeContract(AActor::StaticClass()).bSucceeded);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeStructOperationsTest, "DreamCatcher.MigrationTools.NativeContract.StructOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeStructOperationsTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeAuditTests;
	if (!Context(*this)) { return false; }
	const FName Name(*(TEXT("NativeStructFixture_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	TStrongObjectPtr<UBlueprint> BP(FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), GetTransientPackage(), Name, BPTYPE_Normal));
	if (!TestNotNull(TEXT("Transient struct-operation Blueprint"), BP.Get()) || BP->UbergraphPages.IsEmpty()) { return false; }
	UEdGraph* Graph = BP->UbergraphPages[0];
	UScriptStruct* FixtureStruct = FDCRifleNativeContractRow::StaticStruct();
	if (!TestNotNull(TEXT("Already registered reflected fixture struct"), FixtureStruct)) { return false; }
	UK2Node_MakeStruct* Make = NewObject<UK2Node_MakeStruct>(Graph);
	UK2Node_BreakStruct* Break = NewObject<UK2Node_BreakStruct>(Graph);
	UK2Node_SetFieldsInStruct* SetFields = NewObject<UK2Node_SetFieldsInStruct>(Graph);
	const TArray<UK2Node_StructOperation*> StructNodes = { Make, Break, SetFields };
	TArray<FString> NodePaths;
	TArray<TArray<UEdGraphPin*>> PinsBefore;
	for (UK2Node_StructOperation* StructNode : StructNodes)
	{
		if (!TestNotNull(TEXT("Concrete struct-operation node"), StructNode)) { return false; }
		Graph->AddNode(StructNode, false, false);
		StructNode->CreateNewGuid();
		StructNode->StructType = FixtureStruct;
		StructNode->AllocateDefaultPins();
		NodePaths.Add(StructNode->GetPathName(BP.Get()));
		PinsBefore.Add(StructNode->Pins);
		TestFalse(TEXT("Struct operation has allocated pins"), StructNode->Pins.IsEmpty());
	}
	UK2Node_VariableGet* Variable = NewObject<UK2Node_VariableGet>(Graph);
	Graph->AddNode(Variable, false, false);
	Variable->CreateNewGuid();
	Variable->VariableReference.SetExternalMember(TEXT("bHidden"), AActor::StaticClass());
	Variable->AllocateDefaultPins();
	const auto NodesBefore = Graph->Nodes;
	const bool bDirtyBefore = BP->GetOutermost()->IsDirty();
	const auto StatusBefore = BP->Status;
	UClass* GeneratedBefore = BP->GeneratedClass;
	const auto CheckUnchanged = [this, &BP, Graph, &NodesBefore, &StructNodes, &PinsBefore, bDirtyBefore, StatusBefore, GeneratedBefore]()
	{
		TestTrue(TEXT("Audit preserves graph nodes"), Graph->Nodes == NodesBefore);
		TestEqual(TEXT("Audit preserves dirty state"), BP->GetOutermost()->IsDirty(), bDirtyBefore);
		TestTrue(TEXT("Audit preserves Blueprint status/class"), BP->Status == StatusBefore && BP->GeneratedClass == GeneratedBefore);
		for (int32 Index = 0; Index < StructNodes.Num(); ++Index)
		{
			TestTrue(TEXT("Audit preserves pins"), StructNodes[Index]->Pins == PinsBefore[Index]);
		}
	};
	const auto Report = UDCRifleMigrationLibrary::InspectAssetNativeContract(BP.Get());
	TestTrue(TEXT("Make/Break/SetFields do not require a variable reference"), Report.bSucceeded);
	for (const FString& Message : Report.Messages) { AddInfo(Message); }
	TestEqual(TEXT("All three concrete operations are recorded"), Report.Rows.FilterByPredicate([](const FDCRifleNativeContractRow& Row)
	{
		return Row.Kind == TEXT("node_struct_operation");
	}).Num(), 3);
	for (const FString& NodePath : NodePaths)
	{
		TestTrue(TEXT("Operation records its reflected member type"), Report.Rows.ContainsByPredicate([&NodePath](const FDCRifleNativeContractRow& Row)
		{
			return Row.Kind == TEXT("struct_member") && Row.Context == NodePath && Row.Name == TEXT("Index") && Row.Type == TEXT("IntProperty[1]");
		}));
		TestFalse(TEXT("Operation is not misclassified as an unresolved variable"), Report.Rows.ContainsByPredicate([&NodePath](const FDCRifleNativeContractRow& Row)
		{
			return Row.Kind == TEXT("node_variable") && Row.Context == NodePath;
		}));
		TestTrue(TEXT("Operation pins still appear in the audit"), Report.Rows.ContainsByPredicate([&NodePath](const FDCRifleNativeContractRow& Row)
		{
			return Row.Kind == TEXT("pin") && Row.Context == NodePath;
		}));
	}
	TestNotNull(TEXT("Ordinary variable resolution is retained"), Find(Report, TEXT("node_variable"), TEXT("bHidden")));
	TestTrue(TEXT("Struct reference is retained"), Report.ReferencedTypes.Contains(FixtureStruct->GetPathName()));
	CheckUnchanged();
	// Leave cached pins intact: a stale pin must not disguise a missing authoritative StructType.
	for (UK2Node_StructOperation* StructNode : StructNodes)
	{
		StructNode->StructType = nullptr;
		const auto MissingStruct = UDCRifleMigrationLibrary::InspectAssetNativeContract(BP.Get());
		TestFalse(TEXT("Missing struct type fails closed"), MissingStruct.bSucceeded);
		TestTrue(TEXT("Missing struct has diagnostic evidence"), MissingStruct.Messages.ContainsByPredicate([](const FString& Message)
		{
			return Message.Contains(TEXT("Unresolved struct operation type:"));
		}));
		TestTrue(TEXT("Audit does not repair StructType from cached pins"), StructNode->StructType == nullptr);
		CheckUnchanged();
		StructNode->StructType = FixtureStruct;
	}
	Variable->VariableReference.SetExternalMember(TEXT("DC_MissingFixtureVariable"), AActor::StaticClass());
	const auto MissingVariable = UDCRifleMigrationLibrary::InspectAssetNativeContract(BP.Get());
	TestFalse(TEXT("Ordinary missing variable still fails closed"), MissingVariable.bSucceeded);
	TestTrue(TEXT("Missing variable has diagnostic evidence"), MissingVariable.Messages.ContainsByPredicate([](const FString& Message)
	{
		return Message.Contains(TEXT("Unresolved variable:")) && Message.Contains(TEXT("DC_MissingFixtureVariable"));
	}));
	TestTrue(TEXT("Audit does not repair an invalid variable reference"), Variable->VariableReference.GetMemberName() == TEXT("DC_MissingFixtureVariable"));
	CheckUnchanged();
	return !HasAnyErrors();
}

#endif
