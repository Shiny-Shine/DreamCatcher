#include "DCRifleScopedCopy.h"

#include "BlueprintEditorSettings.h"
#include "BlueprintEditorLibrary.h"
#include "DCRifleDiagnosticGuard.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_MacroInstance.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "Serialization/ArchiveReplaceObjectAndStructPropertyRef.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"

namespace DCRifleMigration
{
namespace
{
UObject* AssetInPackage(const FString& Package, bool bLoad)
{
	const FString Path = Package + TEXT(".") + FPackageName::GetShortName(Package);
	return bLoad ? LoadObject<UObject>(nullptr, *Path) : FindObject<UObject>(nullptr, *Path);
}

FString TranslatePath(FString Path, const TMap<FString, FString>& Map)
{
	for (const TPair<FString, FString>& Pair : Map)
	{
		// The dot boundary also covers class/CDO/graph/subobject paths; not similar prefixes.
		Path.ReplaceInline(*(Pair.Key + TEXT(".")), *(Pair.Value + TEXT(".")), ESearchCase::CaseSensitive);
	}
	return Path;
}

bool IsRetiredClassObject(UObject* Object)
{
	UClass* Class = Cast<UClass>(Object);
	if (!Class) { Class = Object->GetTypedOuter<UClass>(); }
	if (!Class && Object->HasAnyFlags(RF_ClassDefaultObject)) { Class = Object->GetClass(); }
	return Class && Class->HasAnyClassFlags(CLASS_NewerVersionExists);
}

// The base archive only traverses descendants of SearchObject. Explicitly reject any
// serialization outside the exact destination package, including custom Serialize paths.
class FCopyReferenceArchive : public FArchiveReplaceObjectAndStructPropertyRef<UObject>
{
public:
	FCopyReferenceArchive(UObject* Object, const TMap<UObject*, UObject*>& Replacements,
		const TMap<FString, FString>& InPackages, FEngineDiagnosticGuard& InDiagnostics)
		: FArchiveReplaceObjectAndStructPropertyRef(Object, Replacements,
			EArchiveReplaceObjectFlags::DelayStart | EArchiveReplaceObjectFlags::IgnoreOuterRef
			| EArchiveReplaceObjectFlags::IncludeClassGeneratedByRef),
		Packages(InPackages), Destination(Object->GetOutermost()), Diagnostics(InDiagnostics)
	{
		SerializeSearchObject();
	}
	bool bEscaped = false;
	FString UnstagedHierarchy;
	virtual FArchive& operator<<(FSoftObjectPath& Path) override
	{
		// Includes unresolved soft refs, without loading or rewriting arbitrary strings.
		const FString Old = Path.ToString();
		const FString New = TranslatePath(Old, Packages);
		if (Old != New) { Path.SetPath(New); }
		return *this;
	}
protected:
	virtual void SerializeObject(UObject* Object) override
	{
		// An engine call already in progress cannot be unwound here; prevent the
		// archive's remaining queued objects from being edited after it reports failure.
		if (!Diagnostics.IsClean() || bEscaped || !UnstagedHierarchy.IsEmpty()) { return; }
		if (!Object || Object->GetOutermost() != Destination)
		{
			bEscaped = true;
			return;
		}
		// Retired compiler products are not authored data and must not have their
		// old class/CDO frames edited. The compiler owns their retirement.
		if (IsRetiredClassObject(Object)) { return; }
		if (UStruct* Struct = Cast<UStruct>(Object))
		{
			if (ReplacementMap.Contains(Struct->GetSuperStruct()))
			{
				// Never rewrite a live SuperStruct behind the reinstancer's back.
				UnstagedHierarchy = Struct->GetPathName();
				return;
			}
		}
		FArchiveReplaceObjectAndStructPropertyRef<UObject>::SerializeObject(Object);
	}
private:
	const TMap<FString, FString>& Packages;
	UPackage* Destination;
	FEngineDiagnosticGuard& Diagnostics;
};

bool RemapCopies(const TMap<FString, FString>& Map, FEngineDiagnosticGuard& Diagnostics, TArray<FString>& Messages)
{
	TMap<UObject*, UObject*> Replacements;
	TArray<UObject*> CopyObjects;
	for (const TPair<FString, FString>& Pair : Map)
	{
		UPackage* Original = FindPackage(nullptr, *Pair.Key);
		UPackage* Copy = FindPackage(nullptr, *Pair.Value);
		if (!Original || !Copy || Original == Copy)
		{
			Messages.Add(TEXT("Missing/distinct copy package required: ") + Pair.Value);
			return false;
		}
		TArray<UObject*> Originals;
		GetObjectsWithOuter(Original, Originals, EGetObjectsFlags::IncludeNestedObjects);
		GetObjectsWithOuter(Copy, CopyObjects, EGetObjectsFlags::IncludeNestedObjects);
		// Reacquire by package-relative identity after each compile, not stale duplicate maps.
		for (UObject* Object : Originals)
		{
			if (IsRetiredClassObject(Object)) { continue; }
			const FString NewPath = Pair.Value + TEXT(".") + Object->GetPathName(Original);
			if (UObject* NewObject = FindObject<UObject>(nullptr, *NewPath))
			{
				if (NewObject->GetOutermost() == Copy) { Replacements.Add(Object, NewObject); }
			}
		}
		UBlueprint* OldBP = Cast<UBlueprint>(AssetInPackage(Pair.Key, false));
		UBlueprint* NewBP = Cast<UBlueprint>(AssetInPackage(Pair.Value, false));
		if (OldBP && NewBP)
		{
			if (!OldBP->GeneratedClass || !NewBP->GeneratedClass)
			{
				Messages.Add(TEXT("Cannot remap a missing generated class: ") + Pair.Key);
				return false;
			}
			Replacements.Add(OldBP->GeneratedClass, NewBP->GeneratedClass);
			if (OldBP->SkeletonGeneratedClass && NewBP->SkeletonGeneratedClass)
			{
				Replacements.Add(OldBP->SkeletonGeneratedClass, NewBP->SkeletonGeneratedClass);
			}
		}
	}
	for (UObject* Object : CopyObjects)
	{
		if (!Diagnostics.Check(TEXT("before_reference_object"), Messages)) { return false; }
		if (!Map.FindKey(Object->GetOutermost()->GetName()))
		{
			Messages.Add(TEXT("Refusing reference update outside exact copy packages."));
			return false;
		}
		FCopyReferenceArchive Archive(Object, Replacements, Map, Diagnostics);
		if (!Diagnostics.Check(TEXT("after_reference_object"), Messages)) { return false; }
		if (Archive.bEscaped)
		{
			Messages.Add(TEXT("Reference serializer attempted to leave destination package."));
			return false;
		}
		if (!Archive.UnstagedHierarchy.IsEmpty())
		{
			Messages.Add(TEXT("Hierarchy must be rebuilt by the compiler before reference traversal: ") + Archive.UnstagedHierarchy);
			return false;
		}
	}
	return true;
}
}

FDCRifleTypeReferenceReport ReadTypeReferences(UBlueprint* Blueprint)
{
	FDCRifleTypeReferenceReport Result;
	if (!IsValid(Blueprint))
	{
		Result.Messages.Add(TEXT("A loaded Blueprint is required."));
		return Result;
	}
	// Persistent authored graphs only, not compiler-generated intermediate expansion graphs.
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	TSet<UEdGraphNode*> Nodes;
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph)
		{
			for (UEdGraphNode* Node : Graph->Nodes) { if (Node) { Nodes.Add(Node); } }
		}
	}
	for (UEdGraphNode* Node : Nodes)
	{
		const FString Key = Node->GetPathName(Blueprint);
		if (UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Node))
		{
			UEdGraph* Graph = Macro->GetMacroGraph();
			UBlueprint* Owner = Macro->GetSourceBlueprint();
			if (!Graph || !Owner)
			{
				Result.Messages.Add(TEXT("Unresolved macro graph/owner: ") + Node->GetPathName());
				return Result;
			}
			Result.References.Add(Key + TEXT("|MacroGraph=") + Graph->GetPathName());
			Result.References.Add(Key + TEXT("|MacroOwner=") + Owner->GetPathName());
		}
		if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
		{
			Result.References.Add(Key + TEXT("|CastType=") + GetPathNameSafe(CastNode->TargetType.Get()));
		}
		if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
		{
			Result.References.Add(Key + TEXT("|FunctionOwner=") + GetPathNameSafe(Call->FunctionReference.GetMemberParentClass()));
		}
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin) { continue; }
			const FString PinKey = Key + TEXT("|") + Pin->PinName.ToString() + FString::FromInt(Pin->Direction);
			Result.References.Add(PinKey + TEXT("|Type=") + GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get()));
			Result.References.Add(PinKey + TEXT("|MemberOwner=") + GetPathNameSafe(Pin->PinType.PinSubCategoryMemberReference.MemberParent.Get()));
			Result.References.Add(PinKey + TEXT("|ValueType=") + GetPathNameSafe(Pin->PinType.PinValueType.TerminalSubCategoryObject.Get()));
		}
	}
	Result.References.Sort();
	Result.BlueprintState = FString::Printf(TEXT("parent=%s generated=%p skeleton=%p status=%d dirty=%d"),
		*GetPathNameSafe(Blueprint->ParentClass), static_cast<void*>(Blueprint->GeneratedClass.Get()),
		static_cast<void*>(Blueprint->SkeletonGeneratedClass.Get()), static_cast<int32>(Blueprint->Status), Blueprint->GetOutermost()->IsDirty());
	Result.bSucceeded = true;
	return Result;
}

bool FOriginalBlueprintGuard::Capture(TArray<FString>& Messages)
{
	bInitialized = false;
	Entries.Reset();
	Files.Reset();
	for (TObjectIterator<UBlueprint> It; It; ++It)
	{
		UBlueprint* BP = *It;
		FEntry Entry;
		Entry.Blueprint.Reset(BP);
		Entry.Parent.Reset(BP->ParentClass);
		Entry.Generated.Reset(BP->GeneratedClass);
		Entry.Skeleton.Reset(BP->SkeletonGeneratedClass);
		Entry.Status = static_cast<uint8>(BP->Status.GetValue());
		Entry.bDirty = BP->GetOutermost()->IsDirty();
		const FDCRifleTypeReferenceReport Types = ReadTypeReferences(BP);
		if (!Types.bSucceeded) { Messages.Append(Types.Messages); return false; }
		Entry.TypeReferences = Types.References;
		Entries.Add(MoveTemp(Entry));
		FString Filename;
		if (FPackageName::DoesPackageExist(BP->GetOutermost()->GetName(), &Filename))
		{
			Filename = FPaths::ConvertRelativePathToFull(Filename);
			const TArray<FString> WatchedFiles = { Filename, FPaths::ChangeExtension(Filename, TEXT("uexp")),
				FPaths::ChangeExtension(Filename, TEXT("ubulk")), FPaths::ChangeExtension(Filename, TEXT("uptnl")) };
			for (const FString& File : WatchedFiles)
			{
				const bool bExists = IFileManager::Get().FileExists(*File);
				const FMD5Hash Hash = bExists ? FMD5Hash::HashFile(*File) : FMD5Hash();
				if (bExists && !Hash.IsValid()) { Messages.Add(TEXT("Unreadable original BP file: ") + File); return false; }
				Files.Add(File, Hash);
			}
		}
	}
	Messages.Add(FString::Printf(TEXT("Original BP guard captured %d loaded blueprints and %d file states."), Entries.Num(), Files.Num()));
	bInitialized = true;
	return true;
}

bool FOriginalBlueprintGuard::Verify(TArray<FString>& Messages) const
{
	if (!bInitialized) { Messages.Add(TEXT("Original BP guard was not initialized.")); return false; }
	bool bValid = true;
	for (const FEntry& Entry : Entries)
	{
		UBlueprint* BP = Entry.Blueprint.Get();
		const FDCRifleTypeReferenceReport Types = ReadTypeReferences(BP);
		if (!BP || BP->ParentClass != Entry.Parent.Get() || BP->GeneratedClass != Entry.Generated.Get()
			|| BP->SkeletonGeneratedClass != Entry.Skeleton.Get() || static_cast<uint8>(BP->Status.GetValue()) != Entry.Status
			|| BP->GetOutermost()->IsDirty() != Entry.bDirty || !Types.bSucceeded || Types.References != Entry.TypeReferences)
		{
			Messages.Add(TEXT("Original Blueprint identity/status/dirty/type references changed: ") + GetPathNameSafe(BP));
			bValid = false;
		}
	}
	for (const TPair<FString, FMD5Hash>& Pair : Files)
	{
		const bool bExists = IFileManager::Get().FileExists(*Pair.Key);
		const FMD5Hash Hash = bExists ? FMD5Hash::HashFile(*Pair.Key) : FMD5Hash();
		if (bExists != Pair.Value.IsValid() || (bExists && (!Hash.IsValid() || Hash != Pair.Value)))
		{
			Messages.Add(TEXT("Original Blueprint file changed/unreadable: ") + Pair.Key);
			bValid = false;
		}
	}
	return bValid;
}

bool CompareTypeReferences(const TMap<FString, FString>& Map, TArray<FString>& Messages)
{
	for (const TPair<FString, FString>& Pair : Map)
	{
		UBlueprint* Source = Cast<UBlueprint>(AssetInPackage(Pair.Key, false));
		if (!Source) { continue; }
		UBlueprint* Target = Cast<UBlueprint>(AssetInPackage(Pair.Value, false));
		FDCRifleTypeReferenceReport Before = ReadTypeReferences(Source);
		const FDCRifleTypeReferenceReport After = ReadTypeReferences(Target);
		if (!Before.bSucceeded || !After.bSucceeded)
		{
			Messages.Append(Before.Messages); Messages.Append(After.Messages); return false;
		}
		for (FString& Ref : Before.References) { Ref = TranslatePath(Ref, Map); }
		Before.References.Sort();
		if (!Target || TranslatePath(GetPathNameSafe(Source->ParentClass), Map) != GetPathNameSafe(Target->ParentClass)
			|| Before.References != After.References)
		{
			Messages.Add(TEXT("Copied parent/macro/cast/pin subtype differs from mapped original: ") + Pair.Value);
			int32 Differences = 0;
			for (const FString& Ref : Before.References)
			{
				if (!After.References.Contains(Ref) && Differences++ < 8) { Messages.Add(TEXT("Missing type reference: ") + Ref); }
			}
			Differences = 0;
			for (const FString& Ref : After.References)
			{
				if (!Before.References.Contains(Ref) && Differences++ < 8) { Messages.Add(TEXT("Unexpected type reference: ") + Ref); }
			}
			return false;
		}
	}
	return true;
}

bool CopyScoped(const TMap<FString, FString>& Map, FOriginalBlueprintGuard& Guard, TArray<FString>& Messages)
{
	FEngineDiagnosticGuard Diagnostics;
	if (!Diagnostics.Check(TEXT("before_scoped_copy"), Messages)) { return false; }
	if (Map.IsEmpty()) { Messages.Add(TEXT("Empty scoped copy refused.")); return false; }
	if (GetDefault<UBlueprintEditorSettings>()->SaveOnCompile != SoC_Never)
	{
		Messages.Add(TEXT("Save on Compile must be Never; individual duplication compiles internally.")); return false;
	}
	TArray<TStrongObjectPtr<UObject>> Sources;
	TArray<FString> Order;
	Map.GenerateKeyArray(Order);
	Order.Sort();
	for (const FString& Source : Order)
	{
		const FString& Destination = Map.FindChecked(Source);
		if (FindPackage(nullptr, *Destination) || FPackageName::DoesPackageExist(Destination)
			|| !FPackageName::GetShortName(Source).Equals(FPackageName::GetShortName(Destination), ESearchCase::CaseSensitive))
		{
			Messages.Add(TEXT("Scoped copy requires new packages and unchanged leaf names: ") + Destination); return false;
		}
		UObject* Asset = AssetInPackage(Source, true);
		if (!Diagnostics.Check(TEXT("after_source_load"), Messages)) { return false; }
		if (!Asset || Asset->GetClass()->ClassGeneratedBy || Asset->GetOutermost()->ContainsMap()
			|| !Asset->GetOutermost()->GetExternalPackages().IsEmpty())
		{
			Messages.Add(TEXT("Unsupported source (missing/map/external package/BP-defined asset class): ") + Source); return false;
		}
		Sources.Emplace(Asset);
	}
	if (!Guard.Capture(Messages) || !Diagnostics.Check(TEXT("after_original_snapshot"), Messages)) { return false; }
	TArray<TStrongObjectPtr<UObject>> Copies;
	for (int32 Index = 0; Index < Order.Num(); ++Index)
	{
		ObjectTools::FPackageGroupName Name;
		Name.PackageName = Map.FindChecked(Order[Index]);
		Name.ObjectName = FPackageName::GetShortName(Name.PackageName);
		TSet<UPackage*> Refused;
		// No existing destination may reach DuplicateSingleObject's overwrite/delete branch.
		if (FindPackage(nullptr, *Name.PackageName) || FPackageName::DoesPackageExist(Name.PackageName)) { return false; }
		UObject* Copy = ObjectTools::DuplicateSingleObject(Sources[Index].Get(), Name, Refused, false);
		if (!Diagnostics.Check(TEXT("after_individual_duplicate"), Messages)) { return false; }
		if (!Copy || !Refused.IsEmpty()) { Messages.Add(TEXT("Individual native duplication failed: ") + Name.PackageName); return false; }
		Copies.Emplace(Copy);
		if (!Guard.Verify(Messages)) { return false; }
	}
	if (!Guard.Verify(Messages)) { return false; }
	TArray<UBlueprint*> Blueprints;
	for (const TStrongObjectPtr<UObject>& Copy : Copies)
	{
		if (UBlueprint* BP = Cast<UBlueprint>(Copy.Get())) { Blueprints.Add(BP); }
	}
	// Parent first; macro/function libraries before their ordinary consumers at equal depth.
	Blueprints.Sort([](const UBlueprint& A, const UBlueprint& B)
	{
		const auto Depth = [](const UBlueprint& BP) { int32 N = 0; for (UClass* C = BP.ParentClass; C; C = C->GetSuperClass()) { ++N; } return N; };
		const int32 AD = Depth(A), BD = Depth(B);
		if (AD != BD) { return AD < BD; }
		const auto Library = [](const UBlueprint& BP) { return BP.BlueprintType == BPTYPE_MacroLibrary || BP.BlueprintType == BPTYPE_FunctionLibrary; };
		if (Library(A) != Library(B)) { return Library(A); }
		return A.GetPathName() < B.GetPathName();
	});
	// Establish copied parent classes through the engine's lifecycle BEFORE any
	// archive traverses generated classes/CDOs. Parent-first order handles chains.
	for (UBlueprint* BP : Blueprints)
	{
		if (!Diagnostics.Check(TEXT("before_reparent"), Messages)) { return false; }
		UClass* OldParent = BP->ParentClass;
		UBlueprint* OriginalParent = OldParent ? Cast<UBlueprint>(OldParent->ClassGeneratedBy) : nullptr;
		const FString* ParentPackage = OriginalParent ? Map.Find(OriginalParent->GetOutermost()->GetName()) : nullptr;
		if (!ParentPackage) { continue; }
		UBlueprint* NewParent = Cast<UBlueprint>(AssetInPackage(*ParentPackage, false));
		if (!NewParent || !NewParent->GeneratedClass || !Map.FindKey(BP->GetOutermost()->GetName()))
		{
			Messages.Add(TEXT("Missing copied parent or non-copy reparent target: ") + BP->GetPathName()); return false;
		}
		UBlueprintEditorLibrary::ReparentBlueprint(BP, NewParent->GeneratedClass);
		if (!Diagnostics.Check(TEXT("after_engine_reparent"), Messages)) { return false; }
		if (!BP->GeneratedClass || BP->ParentClass != NewParent->GeneratedClass
			|| BP->GeneratedClass->GetSuperClass() != NewParent->GeneratedClass
			|| (BP->Status != BS_UpToDate && BP->Status != BS_UpToDateWithWarnings))
		{
			Messages.Add(TEXT("Engine reparent/compile did not establish copied hierarchy: ") + BP->GetPathName()); return false;
		}
		if (!Guard.Verify(Messages)) { return false; }
	}
	for (UBlueprint* BP : Blueprints)
	{
		// Previous compilation may regenerate classes/CDOs. Never reuse stale replacement pointers.
		if (!RemapCopies(Map, Diagnostics, Messages)) { return false; }
		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection);
		if (!Diagnostics.Check(TEXT("after_copy_compile"), Messages)) { return false; }
		if (!BP->GeneratedClass || (BP->Status != BS_UpToDate && BP->Status != BS_UpToDateWithWarnings))
		{
			Messages.Add(TEXT("Scoped copy failed Blueprint compile: ") + BP->GetPathName()); return false;
		}
		if (!Guard.Verify(Messages)) { return false; }
	}
	// Data-only bundles need remapping too, and last compile may refresh CDO references.
	if (!RemapCopies(Map, Diagnostics, Messages) || !CompareTypeReferences(Map, Messages)) { return false; }
	Messages.Add(TEXT("Scoped duplicate/reference/compile passed. No global consolidation, original save, or destination save."));
	return Guard.Verify(Messages) && Diagnostics.Check(TEXT("after_scoped_copy"), Messages);
}
}
