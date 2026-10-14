#include "DCRifleMigrationLibrary.h"

#include "CoreGlobals.h"
#include "BlueprintEditorSettings.h"
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_StructOperation.h"
#include "K2Node_Variable.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace DCRifleNativeAudit
{
constexpr int32 MaxRows = 16384;
constexpr int32 MaxNodes = 8192;
constexpr int32 MaxDepth = 12;

bool Context(FDCRifleNativeContractReport& Report, bool& bSynthetic)
{
	bSynthetic = false;
	if (!IsInGameThread() || !FApp::IsUnattended()
		|| !FParse::Param(FCommandLine::Get(), TEXT("DCRifleNativeContractAudit")))
	{
		Report.Messages.Add(TEXT("Read-only audit requires the game thread, unattended process and -DCRifleNativeContractAudit."));
		return false;
	}
	if (IsRunningCommandlet())
	{
		FString Error;
		if (!UDCRifleMigrationLibrary::ValidateDiagnosticContext(Error))
		{
			Report.Messages.Add(Error);
			return false;
		}
		return true;
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (GIsEditor && GEditor && !GEditor->PlayWorld
		&& FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))
		&& GetDefault<UBlueprintEditorSettings>()->SaveOnCompile == SoC_Never
		&& FParse::Param(FCommandLine::Get(), TEXT("DCRifleNativeContractTests")))
	{
		bSynthetic = true;
		return true;
	}
#endif
	Report.Messages.Add(TEXT("Real asset audit requires the existing commandlet guard. Editor automation is synthetic-only."));
	return false;
}

bool ApprovedAsset(const FString& Package)
{
	const FString Root = TEXT("/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/");
	for (const FString& Original : UDCRifleMigrationLibrary::GetApprovedRifleClosurePackages())
	{
		int32 Slash = INDEX_NONE;
		Original.FindLastChar(TEXT('/'), Slash);
		if (Package == Root + Original.Mid(Slash + 1)) { return true; }
	}
	return Package == TEXT("/ShooterCore/Input/Abilities/Struct_UIMessaging")
		|| Package == TEXT("/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto")
		|| Package == TEXT("/Game/B_LyraGameInstance")
		|| Package == TEXT("/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base");
}

FString Hex(uint64 Value) { return FString::Printf(TEXT("0x%016llx"), static_cast<unsigned long long>(Value)); }

struct FReader
{
	FDCRifleNativeContractReport Report;
	TSet<FString> References;

	void Issue(const FString& Message) { Report.Messages.AddUnique(Message); }
	FString Ref(const UObject* Object)
	{
		if (!Object) { return TEXT("None"); }
		const FString Path = Object->GetPathName();
		if (Object->IsA<UClass>() || Object->IsA<UScriptStruct>() || Object->IsA<UEnum>()) { References.Add(Path); }
		return Path;
	}
	void Row(const FString& Kind, const FString& ContextName, const FString& Owner, const FString& Name,
		const FString& Type = FString(), const FString& Flags = FString(), int32 Index = INDEX_NONE)
	{
		if (Report.Rows.Num() >= MaxRows) { Issue(TEXT("Row limit exceeded; report is incomplete.")); return; }
		FDCRifleNativeContractRow& Entry = Report.Rows.AddDefaulted_GetRef();
		Entry.Kind = Kind; Entry.Context = ContextName; Entry.Owner = Owner; Entry.Name = Name;
		Entry.Type = Type; Entry.Flags = Flags; Entry.Index = Index;
	}
	FString Type(const FProperty* Property, int32 Depth = 0)
	{
		if (!Property) { Issue(TEXT("Missing reflected property.")); return TEXT("missing"); }
		if (Depth > MaxDepth) { Issue(TEXT("Property nesting limit exceeded.")); return TEXT("depth_limit"); }
		FString Result;
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property)) { Result = TEXT("array<") + Type(ArrayProperty->Inner, Depth + 1) + TEXT(">"); }
		else if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property)) { Result = TEXT("set<") + Type(SetProperty->ElementProp, Depth + 1) + TEXT(">"); }
		else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property)) { Result = TEXT("map<") + Type(MapProperty->KeyProp, Depth + 1) + TEXT(",") + Type(MapProperty->ValueProp, Depth + 1) + TEXT(">"); }
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property)) { Result = TEXT("struct(") + Ref(StructProperty->Struct) + TEXT(")"); }
		else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property)) { Result = TEXT("enum(") + Ref(EnumProperty->GetEnum()) + TEXT(";") + Type(EnumProperty->GetUnderlyingProperty(), Depth + 1) + TEXT(")"); }
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property); ByteProperty && ByteProperty->Enum) { Result = TEXT("enum(") + Ref(ByteProperty->Enum) + TEXT(";ByteProperty[1])"); }
		else if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property)) { Result = TEXT("softclass(") + Ref(SoftClassProperty->MetaClass) + TEXT(")"); }
		else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property)) { Result = TEXT("class(") + Ref(ClassProperty->MetaClass) + TEXT(")"); }
		else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property)) { Result = TEXT("softobject(") + Ref(SoftObjectProperty->PropertyClass) + TEXT(")"); }
		else if (const FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property)) { Result = TEXT("weakobject(") + Ref(WeakObjectProperty->PropertyClass) + TEXT(")"); }
		else if (const FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(Property)) { Result = TEXT("lazyobject(") + Ref(LazyObjectProperty->PropertyClass) + TEXT(")"); }
		else if (const FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property)) { Result = TEXT("object(") + Ref(ObjectPropertyBase->PropertyClass) + TEXT(")"); }
		else if (const FInterfaceProperty* InterfaceProperty = CastField<FInterfaceProperty>(Property)) { Result = TEXT("interface(") + Ref(InterfaceProperty->InterfaceClass) + TEXT(")"); }
		else if (const FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(Property)) { Result = TEXT("delegate(") + Signature(DelegateProperty->SignatureFunction.Get(), Depth + 1) + TEXT(")"); }
		else if (const FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property)) { Result = TEXT("multicast(") + Signature(MulticastDelegateProperty->SignatureFunction.Get(), Depth + 1) + TEXT(")"); }
		else
		{
			Result = Property->GetClass()->GetName();
			static const TSet<FString> Scalars = { TEXT("BoolProperty"), TEXT("ByteProperty"), TEXT("Int8Property"),
				TEXT("Int16Property"), TEXT("IntProperty"), TEXT("Int64Property"), TEXT("UInt16Property"), TEXT("UInt32Property"),
				TEXT("UInt64Property"), TEXT("FloatProperty"), TEXT("DoubleProperty"), TEXT("NameProperty"), TEXT("StrProperty"), TEXT("TextProperty") };
			if (!Scalars.Contains(Result)) { Issue(TEXT("Unsupported property kind: ") + Result + TEXT(" at ") + Property->GetPathName()); }
		}
		return Result + FString::Printf(TEXT("[%d]"), Property->ArrayDim);
	}
	FString Signature(const UFunction* Function, int32 Depth)
	{
		if (!Function) { Issue(TEXT("Unresolved delegate signature.")); return TEXT("missing"); }
		if (Depth > MaxDepth) { Issue(TEXT("Delegate nesting limit exceeded.")); return TEXT("depth_limit"); }
		Ref(Function->GetOuter());
		FString Result = Function->GetPathName() + TEXT(";flags=") + Hex(Function->FunctionFlags);
		for (TFieldIterator<FProperty> It(Function, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				Result += TEXT(";") + It->GetName() + TEXT(":") + Type(*It, Depth) + TEXT(":") + Hex(It->GetPropertyFlags());
			}
		}
		return Result;
	}
	void Function(const UFunction* Function, const FString& ContextName)
	{
		if (!Function) { Issue(TEXT("Unresolved function at ") + ContextName); return; }
		const FString Owner = Ref(Function->GetOuter());
		Row(TEXT("function"), ContextName, Owner, Function->GetName(), FString(), Hex(Function->FunctionFlags));
		int32 Index = 0;
		for (TFieldIterator<FProperty> It(Function, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				Row(TEXT("parameter"), ContextName, Owner, It->GetName(), Type(*It), Hex(It->GetPropertyFlags()), Index++);
			}
		}
	}
	void Structure(UStruct* Struct)
	{
		const FString Owner = Ref(Struct);
		Row(TEXT("super"), TEXT(""), Owner, TEXT("Super"), Ref(Struct->GetSuperStruct()));
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			Row(TEXT("property"), TEXT(""), Ref(It->GetOwnerStruct()), It->GetName(), Type(*It), Hex(It->GetPropertyFlags()));
		}
		if (const UClass* Class = Cast<UClass>(Struct))
		{
			Row(TEXT("class"), TEXT(""), Owner, Class->GetName(), TEXT(""), Hex(Class->GetClassFlags()));
			for (const FImplementedInterface& Interface : Class->Interfaces) { Row(TEXT("interface"), TEXT(""), Owner, TEXT("Interface"), Ref(Interface.Class.Get())); }
			for (TFieldIterator<UFunction> It(Class); It; ++It) { Function(*It, It->GetName()); }
		}
	}
	void Pin(const UEdGraphPin* Pin, const FString& NodePath)
	{
		const FEdGraphPinType& P = Pin->PinType;
		const FString TypeName = FString::Printf(TEXT("%s;%s;%s;container=%d;ref=%d;const=%d;weak=%d;wrapper=%d;value=%s;%s;%s;value_const=%d;value_weak=%d;value_wrapper=%d;member=%s;%s"),
			*P.PinCategory.ToString(), *P.PinSubCategory.ToString(), *Ref(P.PinSubCategoryObject.Get()),
			static_cast<int32>(P.ContainerType), P.bIsReference, P.bIsConst, P.bIsWeakPointer, P.bIsUObjectWrapper,
			*P.PinValueType.TerminalCategory.ToString(), *P.PinValueType.TerminalSubCategory.ToString(), *Ref(P.PinValueType.TerminalSubCategoryObject.Get()),
			P.PinValueType.bTerminalIsConst, P.PinValueType.bTerminalIsWeakPointer, P.PinValueType.bTerminalIsUObjectWrapper,
			*Ref(P.PinSubCategoryMemberReference.MemberParent.Get()), *P.PinSubCategoryMemberReference.MemberName.ToString());
		Row(TEXT("pin"), NodePath, TEXT(""), Pin->PinName.ToString(), TypeName, Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
	}
	void StructOperation(const UK2Node_StructOperation* Node, const FString& NodePath)
	{
		UScriptStruct* Struct = Node->StructType.Get();
		Row(TEXT("node_struct_operation"), NodePath, Ref(Node->GetClass()), Node->GetClass()->GetName(), Ref(Struct));
		if (!IsValid(Struct))
		{
			Issue(TEXT("Unresolved struct operation type: ") + NodePath);
			return;
		}
		int32 Index = 0;
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			Row(TEXT("struct_member"), NodePath, Ref(It->GetOwnerStruct()), It->GetName(), Type(*It), Hex(It->GetPropertyFlags()), Index++);
		}
	}
	void Blueprint(UBlueprint* Blueprint)
	{
		UClass* Scope = Blueprint->GeneratedClass ? Blueprint->GeneratedClass : Blueprint->SkeletonGeneratedClass;
		Row(TEXT("blueprint_parent"), TEXT(""), Blueprint->GetPathName(), TEXT("Parent"), Ref(Blueprint->ParentClass));
		if (!Scope) { Issue(TEXT("Blueprint has no loaded generated/skeleton class; no Compile attempted.")); }
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		int32 NodeCount = 0;
		for (const UEdGraph* Graph : Graphs)
		{
			if (!Graph) { continue; }
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				if (!Node) { continue; }
				if (++NodeCount > MaxNodes) { Issue(TEXT("Blueprint node limit exceeded.")); return; }
				const FString Key = Node->GetPathName(Blueprint);
				if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
				{
					UClass* Owner = Call->FunctionReference.GetMemberParentClass(Scope);
					const FName Name = Call->FunctionReference.GetMemberName();
					Row(TEXT("node_function"), Key, Ref(Owner), Name.ToString());
					Function(Owner ? Owner->FindFunctionByName(Name) : nullptr, Key);
				}
				// SetFieldsInStruct derives from MakeStruct. These value operations use StructType,
				// not VariableReference. Other struct-member/variable nodes retain normal resolution.
				const UK2Node_StructOperation* StructNode = Cast<UK2Node_StructOperation>(Node);
				if (StructNode && (Node->IsA<UK2Node_MakeStruct>() || Node->IsA<UK2Node_BreakStruct>()))
				{
					StructOperation(StructNode, Key);
				}
				else if (const UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node))
				{
					UClass* Owner = Variable->VariableReference.GetMemberParentClass(Scope);
					const FName Name = Variable->VariableReference.GetMemberName();
					UStruct* MemberScope = Owner && Variable->VariableReference.IsLocalScope() ? Variable->VariableReference.GetMemberScope(Owner) : Owner;
					const FProperty* Property = MemberScope ? FindFProperty<FProperty>(MemberScope, Name) : nullptr;
					Row(TEXT("node_variable"), Key, Ref(MemberScope), Name.ToString(), Property ? Type(Property) : TEXT("unresolved"));
					if (!Property) { Issue(TEXT("Unresolved variable: ") + Key + TEXT("|") + Name.ToString()); }
				}
				if (const UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node)) { Row(TEXT("node_cast"), Key, TEXT(""), TEXT("Target"), Ref(CastNode->TargetType.Get())); }
				for (const UEdGraphPin* P : Node->Pins) { if (P) { Pin(P, Key); } }
			}
		}
	}
	FDCRifleNativeContractReport Finish()
	{
		Report.ReferencedTypes = References.Array();
		Report.ReferencedTypes.Sort();
		Report.Rows.Sort([](const FDCRifleNativeContractRow& A, const FDCRifleNativeContractRow& B)
		{
			if (A.Kind != B.Kind) { return A.Kind < B.Kind; }
			if (A.Context != B.Context) { return A.Context < B.Context; }
			if (A.Owner != B.Owner) { return A.Owner < B.Owner; }
			if (A.Index != B.Index) { return A.Index < B.Index; }
			if (A.Name != B.Name) { return A.Name < B.Name; }
			if (A.Type != B.Type) { return A.Type < B.Type; }
			return A.Flags < B.Flags;
		});
		Report.bSucceeded = Report.Messages.IsEmpty();
		return MoveTemp(Report);
	}
};
}

FDCRifleNativeContractReport UDCRifleMigrationLibrary::InspectNativeTypeContract(UObject* NativeType)
{
	using namespace DCRifleNativeAudit;
	FReader Reader;
	bool bSynthetic;
	if (!Context(Reader.Report, bSynthetic)) { return Reader.Report; }
	if (!IsValid(NativeType) || !NativeType->GetOutermost()->GetName().StartsWith(TEXT("/Script/")))
	{
		Reader.Issue(TEXT("Already-loaded /Script class, struct or enum required; no implicit loads."));
		return Reader.Finish();
	}
	Reader.Report.SubjectPath = NativeType->GetPathName();
	if (UClass* Class = Cast<UClass>(NativeType)) { Reader.Structure(Class); }
	else if (UScriptStruct* Struct = Cast<UScriptStruct>(NativeType)) { Reader.Structure(Struct); }
	else if (UEnum* Enum = Cast<UEnum>(NativeType))
	{
		Reader.Ref(Enum);
		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			Reader.Row(TEXT("enum_value"), TEXT(""), Enum->GetPathName(), Enum->GetNameStringByIndex(Index), LexToString(Enum->GetValueByIndex(Index)), TEXT(""), Index);
		}
	}
	else { Reader.Issue(TEXT("Object is not a class, script struct or enum.")); }
	return Reader.Finish();
}

FDCRifleNativeContractReport UDCRifleMigrationLibrary::InspectAssetNativeContract(UObject* Asset)
{
	using namespace DCRifleNativeAudit;
	FReader Reader;
	bool bSynthetic;
	if (!Context(Reader.Report, bSynthetic)) { return Reader.Report; }
	if (!IsValid(Asset)) { Reader.Issue(TEXT("A loaded asset is required.")); return Reader.Finish(); }
	UPackage* Package = Asset->GetOutermost();
	if (bSynthetic ? Package != GetTransientPackage() : !ApprovedAsset(Package->GetName()))
	{
		Reader.Issue(TEXT("Asset outside approved22 or transient synthetic boundary."));
		return Reader.Finish();
	}
	Reader.Report.SubjectPath = Asset->GetPathName();
	const bool bDirtyBefore = Package->IsDirty();
	UClass* ClassBefore = Asset->GetClass();
	Reader.Row(TEXT("asset_class"), TEXT(""), Asset->GetPathName(), TEXT("Class"), Reader.Ref(ClassBefore));
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
	{
		UClass* Parent = Blueprint->ParentClass;
		UClass* Generated = Blueprint->GeneratedClass;
		const EBlueprintStatus Status = Blueprint->Status;
		Reader.Blueprint(Blueprint);
		if (Parent != Blueprint->ParentClass || Generated != Blueprint->GeneratedClass || Status != Blueprint->Status)
		{
			Reader.Issue(TEXT("Blueprint parent/generated/status changed during read-only inspection."));
		}
	}
	else if (UScriptStruct* Struct = Cast<UScriptStruct>(Asset)) { Reader.Structure(Struct); }
	TArray<UObject*> Objects;
	// Read classes of already-loaded owned objects (e.g. fragments/costs), never their values or CDO construction.
	GetObjectsWithOuter(Asset, Objects, EGetObjectsFlags::IncludeNestedObjects);
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset); Blueprint && Blueprint->GeneratedClass)
	{
		GetObjectsWithOuter(Blueprint->GeneratedClass, Objects, EGetObjectsFlags::IncludeNestedObjects);
		// false is essential: observe an existing CDO, never cause one to be constructed here.
		if (UObject* ExistingCDO = Blueprint->GeneratedClass->GetDefaultObject(false))
		{
			Objects.Add(ExistingCDO);
			GetObjectsWithOuter(ExistingCDO, Objects, EGetObjectsFlags::IncludeNestedObjects);
		}
	}
	TSet<const UObject*> SeenObjects;
	for (UObject* Object : Objects)
	{
		if (!Object || SeenObjects.Contains(Object)) { continue; }
		SeenObjects.Add(Object);
		UClass* NativeClass = Object->GetClass();
		while (NativeClass && !NativeClass->HasAnyClassFlags(CLASS_Native)) { NativeClass = NativeClass->GetSuperClass(); }
		Reader.Row(TEXT("owned_native_class"), TEXT(""), Asset->GetPathName(), Object->GetPathName(), Reader.Ref(NativeClass));
	}
	if (Package->IsDirty() != bDirtyBefore || Asset->GetClass() != ClassBefore) { Reader.Issue(TEXT("Subject class/package dirty state changed during inspection.")); }
	return Reader.Finish();
}
