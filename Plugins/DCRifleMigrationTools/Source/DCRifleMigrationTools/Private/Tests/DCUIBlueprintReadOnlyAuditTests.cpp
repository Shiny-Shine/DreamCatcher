#include "DCRifleMigrationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintEditorSettings.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelSlot.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "ISourceControlModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace DCUIReadTests
{
bool Context(FAutomationTestBase& Test)
{
	if (!IsInGameThread() || IsRunningCommandlet() || !GIsEditor || !GEditor || GEditor->PlayWorld
		|| FString(FApp::GetProjectName()) != TEXT("DreamCatcher") || !FApp::IsUnattended()
		|| !FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCR5UIContractTests"))
		|| FParse::Param(FCommandLine::Get(), TEXT("DCR5UISourceAudit"))
		|| FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics"))
		|| GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never
		|| ISourceControlModule::Get().IsEnabled() || FDebug::GetNumEnsureFailures() != 0)
	{
		Test.AddError(TEXT("Use a separate unattended/nullrhi DreamCatcher Editor with -DCR5UIContractTests, no PIE/SCC and SaveOnCompile Never."));
		return false;
	}
	return true;
}

struct FFixture
{
	FString PackageName;
	TStrongObjectPtr<UPackage> Package;
	TStrongObjectPtr<UWidgetBlueprint> BP;

	bool NoFiles(FAutomationTestBase& Test) const
	{
		bool bOK = Test.TestFalse(TEXT("Fixture was not saved as an asset package"), FPackageName::DoesPackageExist(PackageName));
		const FString Base = FPackageName::LongPackageNameToFilename(PackageName);
		for (const TCHAR* Suffix : {TEXT(".uasset"), TEXT(".umap"), TEXT(".uexp"), TEXT(".ubulk"), TEXT(".uptnl")})
		{
			bOK &= Test.TestFalse(TEXT("No fixture file or companion created"), IFileManager::Get().FileExists(*(Base + Suffix)));
		}
		return bOK;
	}

	bool Create(FAutomationTestBase& Test, bool bNamedFields = true, UClass* Parent = UUserWidget::StaticClass(), bool bTree = true, bool bWrongBoxType = false)
	{
		const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		PackageName = TEXT("/Game/LyraMigration/UI/Diagnostics/ReadFixture_") + Suffix;
		if (!NoFiles(Test)) { return false; }
		Package.Reset(CreatePackage(*PackageName));
		Package->SetFlags(RF_Transient);
		BP.Reset(Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package.Get(),
			FName(*(TEXT("W_ReadFixture_") + Suffix)), BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass())));
		if (!Test.TestNotNull(TEXT("Synthetic WidgetBlueprint created"), BP.Get())) { return false; }
		BP->SetFlags(RF_Transient);
		if (!Test.TestNotNull(TEXT("Synthetic source WidgetTree"), BP->WidgetTree.Get())) { return false; }
		if (bTree)
		{
			UHorizontalBox* Box = NewObject<UHorizontalBox>(BP->WidgetTree, TEXT("FixtureRoot"), RF_Transient | RF_ArchetypeObject);
			UButton* Button = NewObject<UButton>(BP->WidgetTree, TEXT("FixtureButton"), RF_Transient | RF_ArchetypeObject);
			Box->bIsVariable = false;
			Button->bIsVariable = false;
			BP->WidgetTree->RootWidget = Box;
			if (!Test.TestNotNull(TEXT("Template child slot"), Box->AddChild(Button))) { return false; }
		}
		if (bNamedFields)
		{
			// Native Lyra BindWidget classes are not dependencies of this Editor test module.
			// Two explicitly typed synthetic properties exercise null/value reflection only.
			FEdGraphPinType BoxType;
			BoxType.PinCategory = UEdGraphSchema_K2::PC_Object;
			BoxType.PinSubCategoryObject = bWrongBoxType ? UButton::StaticClass() : UHorizontalBox::StaticClass();
			FEdGraphPinType ButtonType;
			ButtonType.PinCategory = UEdGraphSchema_K2::PC_Object;
			ButtonType.PinSubCategoryObject = UButton::StaticClass();
			if (!Test.TestTrue(TEXT("Box property created"), FBlueprintEditorUtils::AddMemberVariable(BP.Get(), TEXT("HBox_SwitchUser"), BoxType))
				|| !Test.TestTrue(TEXT("Button property created"), FBlueprintEditorUtils::AddMemberVariable(BP.Get(), TEXT("Button_ChangeUser"), ButtonType))) { return false; }
		}
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(BP.Get(), EBlueprintCompileOptions::SkipGarbageCollection | EBlueprintCompileOptions::SkipSave, &Results);
		if (!Test.TestEqual(TEXT("Synthetic Compile errors"), Results.NumErrors, 0)
			|| !Test.TestEqual(TEXT("Synthetic Compile warnings"), Results.NumWarnings, 0)
			|| !Test.TestTrue(TEXT("Synthetic Compile status"), BP->Status == BS_UpToDate)
			|| !Test.TestNotNull(TEXT("Synthetic generated class"), BP->GeneratedClass.Get())) { return false; }
		return NoFiles(Test);
	}
};

const FDCUIObjectFieldRead* Field(const FDCUIBlueprintReadReport& Report, const TCHAR* Name)
{
	return Report.Fields.FindByPredicate([Name](const FDCUIObjectFieldRead& Row) { return Row.Name == Name; });
}

void Messages(FAutomationTestBase& Test, const FDCUIBlueprintReadReport& Report)
{
	for (const FString& Message : Report.Messages) { Test.AddInfo(Message); }
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCUIReadRejectedInputs, "DreamCatcher.MigrationTools.UIRead.RejectedInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCUIReadRejectedInputs::RunTest(const FString& Parameters)
{
	using namespace DCUIReadTests;
	if (!Context(*this)) { return false; }
	TestFalse(TEXT("Null input rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(nullptr).bInspectionStarted);
	TStrongObjectPtr<UEdGraph> WrongType(NewObject<UEdGraph>(GetTransientPackage(), NAME_None, RF_Transient));
	TestFalse(TEXT("Concrete non-WidgetBlueprint rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(WrongType.Get()).bInspectionStarted);
	TestFalse(TEXT("Existing script class rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(UUserWidget::StaticClass()).bInspectionStarted);
	TStrongObjectPtr<UWidgetBlueprint> WrongName(NewObject<UWidgetBlueprint>(GetTransientPackage(), NAME_None, RF_Transient));
	TestFalse(TEXT("Generic transient Blueprint outside fixture scope rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(WrongName.Get()).bInspectionStarted);
	FFixture Fixture;
	if (!Fixture.Create(*this)) { return false; }
	Fixture.BP->ClearFlags(RF_Transient); // Only our own in-memory fixture, never a real asset.
	TestFalse(TEXT("Correct path without transient asset flag rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Fixture.BP.Get()).bInspectionStarted);
	Fixture.BP->SetFlags(RF_Transient);
	Fixture.Package->ClearFlags(RF_Transient);
	TestFalse(TEXT("Correct path without transient package flag rejected"), UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Fixture.BP.Get()).bInspectionStarted);
	Fixture.Package->SetFlags(RF_Transient);
	Fixture.NoFiles(*this);
	TestEqual(TEXT("No engine ensures"), FDebug::GetNumEnsureFailures(), SIZE_T{0});
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCUIReadTree, "DreamCatcher.MigrationTools.UIRead.TemplateTreeAndComments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCUIReadTree::RunTest(const FString& Parameters)
{
	using namespace DCUIReadTests;
	if (!Context(*this)) { return false; }
	FFixture Fixture;
	if (!Fixture.Create(*this)) { return false; }
	UWidgetBlueprint* BP = Fixture.BP.Get();
	UEdGraph* Graph = BP->UbergraphPages.IsEmpty() ? nullptr : BP->UbergraphPages[0].Get();
	if (!Graph)
	{
		Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("FixtureGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
	}
	UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transient);
	Graph->AddNode(Comment, false, false);
	Comment->CreateNewGuid();
	const auto Status = BP->Status;
	const EObjectFlags Flags = BP->GetFlags();
	const bool bDirty = Fixture.Package->IsDirty();
	UClass* Generated = BP->GeneratedClass;
	UWidgetTree* Tree = BP->WidgetTree;
	UWidget* Root = Tree->RootWidget;
	const auto Report = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(BP);
	Messages(*this, Report);
	TestTrue(TEXT("Bounded UI query succeeded"), Report.bSucceeded);
	TestTrue(TEXT("Reader state snapshots unchanged"), Report.bReadOnlyStatePreserved);
	TestEqual(TEXT("Exact source tree pointer path"), Report.SourceTreePath, Tree->GetPathName());
	TestEqual(TEXT("Exact root pointer path"), Report.SourceRootPath, Root->GetPathName());
	TestEqual(TEXT("Two source-owned templates"), Report.Widgets.Num(), 2);
	const auto* Child = Report.Widgets.FindByPredicate([](const FDCUIWidgetTemplateRead& Row) { return Row.Name == TEXT("FixtureButton"); });
	if (TestNotNull(TEXT("Child template observed"), Child))
	{
		TestEqual(TEXT("Parent path retained"), Child->ParentPath, Root->GetPathName());
		TestFalse(TEXT("Slot path retained"), Child->SlotPath.IsEmpty());
		TestEqual(TEXT("Template class retained"), Child->ClassPath, UButton::StaticClass()->GetPathName());
	}
	TestEqual(TEXT("Own generated class supplies a tree root"), Report.TreeOwnerClassPath, Generated->GetPathName());
	if (TestEqual(TEXT("One comment observed"), Report.Comments.Num(), 1))
	{
		TestEqual(TEXT("Actual zero comment pins"), Report.Comments[0].PinCount, 0);
	}
	Comment->CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, TEXT("FixturePin"));
	const auto WithPin = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(BP);
	TestTrue(TEXT("Nonzero comment pin count can be read without Compile"), WithPin.bSucceeded);
	if (TestEqual(TEXT("One comment after fixture pin addition"), WithPin.Comments.Num(), 1))
	{
		TestEqual(TEXT("Pins are counted, not assumed empty"), WithPin.Comments[0].PinCount, 1);
	}
	TestTrue(TEXT("No Compile/status change"), BP->Status == Status);
	TestTrue(TEXT("Asset flags unchanged"), BP->GetFlags() == Flags);
	TestEqual(TEXT("Dirty state unchanged"), Fixture.Package->IsDirty(), bDirty);
	TestTrue(TEXT("Generated/tree/root pointers unchanged"), BP->GeneratedClass == Generated && BP->WidgetTree == Tree && Tree->RootWidget == Root);
	// Child source tree may be empty while its already-loaded parent owns the generated tree.
	FFixture ParentFixture;
	FFixture Inherited;
	if (ParentFixture.Create(*this) && Inherited.Create(*this, false, ParentFixture.BP->GeneratedClass, false))
	{
		const auto ChildReport = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Inherited.BP.Get());
		Messages(*this, ChildReport);
		TestTrue(TEXT("Inherited tree query succeeded without PostLoad"), ChildReport.bSucceeded);
		TestTrue(TEXT("Child source root remains empty"), ChildReport.SourceRootPath.IsEmpty());
		TestEqual(TEXT("Inherited owner distinguished from child class"), ChildReport.TreeOwnerClassPath, ParentFixture.BP->GeneratedClass->GetPathName());
		Inherited.NoFiles(*this);
	}
	if (ParentFixture.Package.IsValid()) { ParentFixture.NoFiles(*this); }
	Fixture.NoFiles(*this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCUIReadNamedFields, "DreamCatcher.MigrationTools.UIRead.NamedFieldsAndReadOnlyState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCUIReadNamedFields::RunTest(const FString& Parameters)
{
	using namespace DCUIReadTests;
	if (!Context(*this)) { return false; }
	FFixture Fixture;
	if (!Fixture.Create(*this)) { return false; }
	UObject* CDO = Fixture.BP->GeneratedClass->GetDefaultObject(false);
	if (!TestNotNull(TEXT("Existing synthetic CDO"), CDO)) { return false; }
	FObjectPropertyBase* BoxField = FindFProperty<FObjectPropertyBase>(CDO->GetClass(), TEXT("HBox_SwitchUser"));
	if (!TestNotNull(TEXT("Synthetic typed Box property"), BoxField)) { return false; }
	const EPropertyFlags PropertyFlags = BoxField->GetPropertyFlags();
	const auto Initial = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Fixture.BP.Get());
	Messages(*this, Initial);
	TestTrue(TEXT("CDO query succeeds"), Initial.bSucceeded);
	TestEqual(TEXT("Exactly two named fields, no arbitrary value enumeration"), Initial.Fields.Num(), 2);
	const auto* NullField = Field(Initial, TEXT("HBox_SwitchUser"));
	if (TestNotNull(TEXT("Null CDO field row exists"), NullField))
	{
		TestEqual(TEXT("Null is distinct from a missing property"), NullField->Status, FString(TEXT("null")));
	}
	// Set only the transient fixture CDO. The reader has no setters and must not initialize binding.
	const UWidget* Root = Fixture.BP->WidgetTree->RootWidget;
	BoxField->SetObjectPropertyValue_InContainer(CDO, Fixture.BP->WidgetTree->RootWidget);
	const auto Observed = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Fixture.BP.Get());
	TestTrue(TEXT("Explicit synthetic object reference observed"), Observed.bSucceeded);
	const auto* ValueField = Field(Observed, TEXT("HBox_SwitchUser"));
	if (TestNotNull(TEXT("Object row exists"), ValueField))
	{
		TestEqual(TEXT("Non-null status"), ValueField->Status, FString(TEXT("object")));
		TestEqual(TEXT("Pointer path preserved"), ValueField->ValuePath, Fixture.BP->WidgetTree->RootWidget->GetPathName());
	}
	TestTrue(TEXT("Property flags not bypassed or changed"), BoxField->GetPropertyFlags() == PropertyFlags);
	TestTrue(TEXT("CDO field pointer unchanged by query"), BoxField->GetObjectPropertyValue_InContainer(CDO) == Root);
	TestTrue(TEXT("Existing CDO pointer unchanged"), Fixture.BP->GeneratedClass->GetDefaultObject(false) == CDO);
	BoxField->SetObjectPropertyValue_InContainer(CDO, nullptr);
	FFixture Missing;
	if (Missing.Create(*this, false))
	{
		const auto MissingReport = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(Missing.BP.Get());
		TestFalse(TEXT("Missing fields are not declared complete"), MissingReport.bSucceeded);
		TestTrue(TEXT("Incomplete read still preserves state"), MissingReport.bReadOnlyStatePreserved);
		const auto* MissingField = Field(MissingReport, TEXT("HBox_SwitchUser"));
		if (TestNotNull(TEXT("Missing property has explicit row"), MissingField))
		{
			TestEqual(TEXT("Missing status is not null"), MissingField->Status, FString(TEXT("missing_property")));
		}
		Missing.NoFiles(*this);
	}
	FFixture WrongTyped;
	if (WrongTyped.Create(*this, true, UUserWidget::StaticClass(), true, true))
	{
		const auto WrongReport = UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(WrongTyped.BP.Get());
		TestFalse(TEXT("Named property with wrong class fails closed"), WrongReport.bSucceeded);
		const auto* WrongField = Field(WrongReport, TEXT("HBox_SwitchUser"));
		if (TestNotNull(TEXT("Wrong-type field row exists"), WrongField))
		{
			TestEqual(TEXT("Wrong class was not dereferenced"), WrongField->Status, FString(TEXT("unsupported_type")));
			TestTrue(TEXT("Wrong-type value was not read"), WrongField->ValuePath.IsEmpty());
		}
		TestTrue(TEXT("Wrong-type read preserves state"), WrongReport.bReadOnlyStatePreserved);
		WrongTyped.NoFiles(*this);
	}
	Fixture.NoFiles(*this);
	TestEqual(TEXT("No engine ensures"), FDebug::GetNumEnsureFailures(), SIZE_T{0});
	return !HasAnyErrors();
}

#endif
