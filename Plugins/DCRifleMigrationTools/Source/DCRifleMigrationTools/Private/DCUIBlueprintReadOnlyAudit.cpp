#include "DCRifleMigrationLibrary.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintEditorSettings.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "ISourceControlModule.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace DCUIReadAudit
{
constexpr int32 MaxWidgets = 4096;
constexpr int32 MaxGraphs = 128;
constexpr int32 MaxNodes = 4096;
constexpr int32 MaxPins = 32768;
constexpr int32 MaxAncestors = 64;
constexpr int32 MaxText = 32768;
constexpr int32 MaxTotalText = 4 * 1024 * 1024;

const TCHAR* const SourcePackages[] = {
	TEXT("/Game/UI/W_OverallUILayout"),
	TEXT("/Game/UI/Hud/W_DefaultHUDLayout"),
	TEXT("/Game/UI/Foundation/Dialogs/W_ControllerDisconnected"),
	TEXT("/ShooterCore/UserInterface/W_ShooterHUDLayout"),
	TEXT("/ShooterCore/UserInterface/HUD/W_WeaponReticleHost"),
	TEXT("/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName"),
	TEXT("/ShooterCore/UserInterface/HUD/W_QuickBar"),
	TEXT("/ShooterCore/UserInterface/HUD/W_QuickBarSlot")
};

bool Context(bool& bSynthetic, FString& Error)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")))
	{
		Error = TEXT("Runtime-subsystem diagnostic mode is outside the UI read scope.");
		return false;
	}
	bSynthetic = FParse::Param(FCommandLine::Get(), TEXT("DCR5UIContractTests"));
	if (bSynthetic)
	{
		if (!IsInGameThread() || IsRunningCommandlet() || !GIsEditor || !GEditor || GEditor->PlayWorld
			|| !FApp::IsUnattended() || FString(FApp::GetProjectName()) != TEXT("DreamCatcher")
			|| !FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))
			|| FParse::Param(FCommandLine::Get(), TEXT("DCR5UISourceAudit"))
			|| GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never
			|| ISourceControlModule::Get().IsEnabled() || FDebug::GetNumEnsureFailures() != 0)
		{
			Error = TEXT("Synthetic UI queries require an isolated unattended/nullrhi DreamCatcher Editor, no PIE/SCC/ensure, SaveOnCompile Never and -DCR5UIContractTests only.");
			return false;
		}
		return true;
	}
	if (FString(FApp::GetProjectName()) != TEXT("LyraStarterGame")
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCR5UISourceAudit")))
	{
		Error = TEXT("Source UI queries require the original Lyra commandlet with -DCR5UISourceAudit.");
		return false;
	}
	return UDCRifleMigrationLibrary::ValidateDiagnosticContext(Error);
}

bool Allowed(const UWidgetBlueprint* BP, bool bSynthetic)
{
	const UPackage* Package = BP->GetOutermost();
	const FString PackageName = Package->GetName();
	if (!bSynthetic)
	{
		for (const TCHAR* Name : SourcePackages)
		{
			if (PackageName == Name && BP->GetOuter() == Package
				&& BP->GetName() == FPackageName::GetLongPackageAssetName(PackageName))
			{
				return !Package->IsDirty();
			}
		}
		return false;
	}
	const FString Prefix = TEXT("/Game/LyraMigration/UI/Diagnostics/ReadFixture_");
	const FString Suffix = PackageName.Mid(Prefix.Len());
	FGuid Id;
	return PackageName.StartsWith(Prefix) && FGuid::ParseExact(Suffix, EGuidFormats::Digits, Id)
		&& BP->GetName() == TEXT("W_ReadFixture_") + Suffix && BP->GetOuter() == Package
		&& BP->HasAnyFlags(RF_Transient) && Package->HasAnyFlags(RF_Transient)
		&& !BP->HasAnyFlags(RF_WasLoaded) && !Package->GetLinker()
		&& !FPackageName::DoesPackageExist(PackageName);
}

struct FObjectState
{
	const UObject* Object;
	const UObject* Outer;
	const UClass* Class;
	EObjectFlags Flags;
	const UPackage* Package;
	bool bDirty;
};

struct FReader
{
	FDCUIBlueprintReadReport& Report;
	TMap<const UObject*, FObjectState> States;
	int32 TextCount = 0;
	bool bComplete = true;

	void Missing(const FString& Message)
	{
		bComplete = false;
		if (Report.Messages.Num() < 64) { Report.Messages.Add(Message.Left(MaxText)); }
	}

	bool Observe(const UObject* Object)
	{
		if (!Object) { return true; }
		if (!IsValid(Object) || Object->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects))
		{
			Missing(TEXT("Object is invalid or not fully loaded; no PostLoad requested."));
			return false;
		}
		if (!States.Contains(Object))
		{
			if (States.Num() >= 32768) { Missing(TEXT("Object snapshot bound exceeded.")); return false; }
			const UPackage* Package = Object->GetOutermost();
			States.Add(Object, {Object, Object->GetOuter(), Object->GetClass(), Object->GetFlags(), Package, Package->IsDirty()});
		}
		return true;
	}

	FString Text(const FString& Value)
	{
		if (Value.Len() > MaxText || TextCount + Value.Len() > MaxTotalText)
		{
			Missing(TEXT("UI report text bound exceeded; value omitted."));
			return FString();
		}
		TextCount += Value.Len();
		return Value;
	}

	FString Path(const UObject* Object)
	{
		return Object && Observe(Object) ? Text(Object->GetPathName()) : FString();
	}

	bool Unchanged() const
	{
		for (const auto& Pair : States)
		{
			const FObjectState& State = Pair.Value;
			if (!IsValid(State.Object) || State.Object->GetFlags() != State.Flags
				|| State.Object->GetOuter() != State.Outer || State.Object->GetClass() != State.Class
				|| State.Object->GetOutermost() != State.Package || State.Package->IsDirty() != State.bDirty)
			{
				return false;
			}
		}
		return true;
	}
};

bool HasClassPath(const UClass* Class, const TCHAR* Expected)
{
	// Do not load CommonUI or evaluate a property conversion to check its declared type.
	for (int32 Depth = 0; Class && Depth < MaxAncestors; ++Depth, Class = Class->GetSuperClass())
	{
		if (Class->GetPathName() == Expected) { return true; }
	}
	return false;
}
}

FDCUIBlueprintReadReport UDCRifleMigrationLibrary::InspectUIBlueprintReadOnly(UObject* Asset)
{
	using namespace DCUIReadAudit;
	FDCUIBlueprintReadReport Report;
	bool bSynthetic = false;
	FString Error;
	if (!Context(bSynthetic, Error)) { Report.Messages.Add(Error); return Report; }
	const UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Asset);
	if (!IsValid(BP) || !Allowed(BP, bSynthetic))
	{
		Report.Messages.Add(TEXT("Only the fixed original WidgetBlueprint8 or a uniquely named, transient, unsaved UI fixture is allowed."));
		return Report;
	}
	FReader Reader{Report};
	if (!Reader.Observe(BP)) { return Report; }
	Report.bInspectionStarted = true;
	const auto InitialStatus = BP->Status;
	const UClass* InitialGenerated = BP->GeneratedClass;
	const UClass* InitialParent = BP->ParentClass;
	const UWidgetTree* SourceTree = BP->WidgetTree;
	const UWidget* SourceRoot = nullptr;
	const bool bSourceTreeReady = SourceTree && Reader.Observe(SourceTree);
	struct FWidgetLinks { const UWidget* Widget; const UPanelSlot* Slot; const UPanelWidget* Parent; const UWidget* Content; };
	struct FGeneratedLinks { const UWidgetBlueprintGeneratedClass* Class; const UClass* Super; const UWidgetTree* Tree; const UWidget* Root; bool bTreeReady; };
	TArray<FWidgetLinks> WidgetLinks;
	TArray<FGeneratedLinks> GeneratedLinks;
	Report.SubjectPath = Reader.Path(BP);
	Report.SourceTreePath = Reader.Path(SourceTree);
	if (bSourceTreeReady)
	{
		SourceRoot = SourceTree->RootWidget;
		Report.SourceRootPath = Reader.Path(SourceRoot);
		// Public const source enumeration: exact WidgetTree outer, no widget virtual calls.
		TArray<const UWidget*> Widgets = BP->GetAllSourceWidgets();
		if (Widgets.Num() > MaxWidgets) { Reader.Missing(TEXT("Source widget count bound exceeded.")); }
		else
		{
			Widgets.Sort([](const UWidget& A, const UWidget& B) { return A.GetName() < B.GetName(); });
			for (const UWidget* Widget : Widgets)
			{
				if (!Reader.Observe(Widget) || Widget->GetOuter() != SourceTree) { Reader.Missing(TEXT("Invalid source widget outer.")); continue; }
				FDCUIWidgetTemplateRead Row;
				Row.Path = Reader.Path(Widget);
				Row.Name = Reader.Text(Widget->GetName());
				Row.ClassPath = Reader.Path(Widget->GetClass());
				const UPanelSlot* Slot = Widget->Slot;
				Row.SlotPath = Reader.Path(Slot);
				if (Slot && Reader.Observe(Slot))
				{
					Row.SlotClassPath = Reader.Path(Slot->GetClass());
					Row.ParentPath = Reader.Path(Slot->Parent);
					WidgetLinks.Add({Widget, Slot, Slot->Parent, Slot->Content});
				}
				else if (!Slot) { WidgetLinks.Add({Widget, nullptr, nullptr, nullptr}); }
				Report.Widgets.Add(MoveTemp(Row));
			}
		}
	}
	else { Reader.Missing(TEXT("Source WidgetTree pointer unavailable; no tree was created.")); }

	const UWidgetBlueprintGeneratedClass* Generated = Cast<UWidgetBlueprintGeneratedClass>(BP->GeneratedClass);
	const UObject* CDO = nullptr;
	const bool bGeneratedReady = Generated && Reader.Observe(Generated);
	Report.GeneratedClassPath = Reader.Path(Generated);
	if (bGeneratedReady)
	{
		CDO = Generated->GetDefaultObject(false);
		Report.ExistingCDOPath = Reader.Path(CDO);
		if (!CDO) { Reader.Missing(TEXT("Existing CDO unavailable; GetDefaultObject(false) did not create one.")); }
		const UWidgetBlueprintGeneratedClass* Current = Generated;
		for (int32 Depth = 0; Current; ++Depth)
		{
			if (Depth >= MaxAncestors) { Reader.Missing(TEXT("Generated class ancestry bound exceeded.")); break; }
			if (!Reader.Observe(Current)) { break; }
			FDCUIGeneratedTreeRead Row;
			Row.ClassPath = Reader.Path(Current);
			const UWidgetTree* Tree = Current->GetWidgetTreeArchetype();
			Row.TreePath = Reader.Path(Tree);
			const bool bTreeReady = Tree && Reader.Observe(Tree);
			GeneratedLinks.Add({Current, Current->GetSuperClass(), Tree, bTreeReady ? Tree->RootWidget.Get() : nullptr, bTreeReady});
			if (bTreeReady)
			{
				Row.RootPath = Reader.Path(Tree->RootWidget);
				if (Report.TreeOwnerClassPath.IsEmpty() && !Row.RootPath.IsEmpty()) { Report.TreeOwnerClassPath = Row.ClassPath; }
			}
			Report.GeneratedTrees.Add(MoveTemp(Row));
			// FindWidgetTreeOwningClass calls ConditionalPostLoad in UE5.8. Never use it here.
			Current = Cast<UWidgetBlueprintGeneratedClass>(Current->GetSuperClass());
		}
	}
	else { Reader.Missing(TEXT("Loaded WidgetBlueprintGeneratedClass unavailable.")); }

	struct FFieldState { const FObjectProperty* Property; EPropertyFlags Flags; const UObject* Value; };
	TArray<FFieldState> FieldStates;
	if (bSynthetic || BP->GetOutermost()->GetName() == SourcePackages[2])
	{
		for (const TCHAR* Name : {TEXT("HBox_SwitchUser"), TEXT("Button_ChangeUser")})
		{
			FDCUIObjectFieldRead Row;
			Row.Name = Name;
			Row.Status = TEXT("unavailable_cdo");
			if (CDO && Reader.Observe(CDO))
			{
				const FProperty* Property = FindFProperty<FProperty>(CDO->GetClass(), Name);
				// Hard object fields only. Soft/lazy references and property getters are not executed.
				const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
				Row.Status = Property ? TEXT("unsupported_type") : TEXT("missing_property");
				if (ObjectProperty && ObjectProperty->ArrayDim == 1 && !ObjectProperty->HasGetter())
				{
					Row.OwnerPath = Reader.Path(ObjectProperty->GetOwnerStruct());
					Row.DeclaredClassPath = Reader.Path(ObjectProperty->PropertyClass);
					const bool bBox = Row.Name == TEXT("HBox_SwitchUser");
					const bool bTypeAllowed = Reader.Observe(ObjectProperty->PropertyClass) && ObjectProperty->PropertyClass
						&& (bBox ? ObjectProperty->PropertyClass->IsChildOf(UHorizontalBox::StaticClass())
						: HasClassPath(ObjectProperty->PropertyClass, TEXT("/Script/CommonUI.CommonButtonBase"))
							|| (bSynthetic && ObjectProperty->PropertyClass->IsChildOf(UButton::StaticClass())));
					if (bTypeAllowed)
					{
						const TObjectPtr<UObject> ValuePtr = ObjectProperty->GetObjectPtrPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(CDO));
						Row.Status = TEXT("unresolved_reference");
						if (ValuePtr.IsResolved())
						{
							const UObject* Value = ValuePtr.Get();
							FieldStates.Add({ObjectProperty, ObjectProperty->GetPropertyFlags(), Value});
							Row.ValuePath = Reader.Path(Value);
							Row.ValueClassPath = Value && Reader.Observe(Value) ? Reader.Path(Value->GetClass()) : FString();
							Row.Status = Value ? TEXT("object") : TEXT("null");
						}
					}
				}
			}
			if (Row.Status != TEXT("null") && Row.Status != TEXT("object"))
			{
				Reader.Missing(FString::Printf(TEXT("Named CDO field %s: %s; runtime binding not inferred."), *Row.Name, *Row.Status));
			}
			Report.Fields.Add(MoveTemp(Row));
		}
	}

	TArray<UEdGraph*> Graphs;
	BP->GetAllGraphs(Graphs);
	if (Graphs.Num() > MaxGraphs) { Reader.Missing(TEXT("Graph count bound exceeded.")); }
	else
	{
		int32 NodeCount = 0;
		int32 PinCount = 0;
		for (const UEdGraph* Graph : Graphs)
		{
			if (!Graph || !Reader.Observe(Graph)) { Reader.Missing(TEXT("Null or unloaded graph.")); continue; }
			NodeCount += Graph->Nodes.Num();
			if (NodeCount > MaxNodes) { Reader.Missing(TEXT("Node count bound exceeded.")); break; }
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				const UEdGraphNode_Comment* Comment = Cast<UEdGraphNode_Comment>(Node);
				if (!Comment || !Reader.Observe(Comment)) { continue; }
				PinCount += Comment->Pins.Num();
				if (PinCount > MaxPins) { Reader.Missing(TEXT("Comment pin count bound exceeded.")); break; }
				FDCUICommentRead Row;
				Row.Path = Reader.Path(Comment);
				Row.PinCount = Comment->Pins.Num();
				Report.Comments.Add(MoveTemp(Row));
			}
			if (PinCount > MaxPins) { break; }
		}
	}
	Report.Comments.Sort([](const FDCUICommentRead& A, const FDCUICommentRead& B) { return A.Path < B.Path; });
	bool bFieldsUnchanged = true;
	for (const FFieldState& Field : FieldStates)
	{
		const TObjectPtr<UObject> ValuePtr = Field.Property->GetObjectPtrPropertyValue(Field.Property->ContainerPtrToValuePtr<void>(CDO));
		bFieldsUnchanged &= Field.Property->GetPropertyFlags() == Field.Flags
			&& ValuePtr.IsResolved() && ValuePtr.Get() == Field.Value;
	}
	bool bLinksUnchanged = true;
	for (const FWidgetLinks& Links : WidgetLinks)
	{
		bLinksUnchanged &= Links.Widget->Slot == Links.Slot
			&& (!Links.Slot || (Links.Slot->Parent == Links.Parent && Links.Slot->Content == Links.Content));
	}
	for (const FGeneratedLinks& Links : GeneratedLinks)
	{
		bLinksUnchanged &= Links.Class->GetSuperClass() == Links.Super && Links.Class->GetWidgetTreeArchetype() == Links.Tree
			&& (!Links.bTreeReady || Links.Tree->RootWidget == Links.Root);
	}
	Report.bReadOnlyStatePreserved = Reader.Unchanged() && bFieldsUnchanged && BP->Status == InitialStatus
		&& BP->GeneratedClass == InitialGenerated && BP->ParentClass == InitialParent && BP->WidgetTree == SourceTree
		&& (!bSourceTreeReady || SourceTree->RootWidget == SourceRoot) && bLinksUnchanged
		&& (!bGeneratedReady || Generated->GetDefaultObject(false) == CDO) && FDebug::GetNumEnsureFailures() == 0;
	if (!Report.bReadOnlyStatePreserved) { Reader.Missing(TEXT("Read-only state/ensure guard failed; do not save or repair assets.")); }
	Report.bSucceeded = Reader.bComplete && Report.bReadOnlyStatePreserved;
	return Report;
}
