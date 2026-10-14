#include "DCRifleInheritedOverrides.h"
#include "DCRifleDiagnosticGuard.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace DCRifleMigration
{
namespace
{
UBlueprint* LoadBlueprint(const FString& Package)
{
	return LoadObject<UBlueprint>(nullptr, *(Package + TEXT(".") + FPackageName::GetShortName(Package)));
}

FString MappedPackage(const FString& Source, const TMap<FString, FString>& Map)
{
	const FString* Destination = Map.Find(Source);
	return Destination ? *Destination : Source;
}

FString CanonicalValue(FString Value, const FString& TemplatePath, const TMap<FString, FString>& Map)
{
	// Owned-object paths include the root template path; normalize those first.
	Value.ReplaceInline(*TemplatePath, TEXT("@Component@"), ESearchCase::CaseSensitive);
	TArray<FString> Sources;
	Map.GenerateKeyArray(Sources);
	Sources.Sort([](const FString& A, const FString& B) { return A.Len() > B.Len(); });
	for (const FString& Source : Sources)
	{
		Value.ReplaceInline(*(Source + TEXT(".")), *(Map.FindChecked(Source) + TEXT(".")), ESearchCase::CaseSensitive);
	}
	return Value;
}

bool ReadProperties(UActorComponent* Template, const TMap<FString, FString>& Map,
	TMap<FString, FString>& Out, TArray<FString>& Messages)
{
	Out.Reset();
	if (!IsValid(Template))
	{
		Messages.Add(TEXT("Cannot inspect a missing component override."));
		return false;
	}
	TArray<UObject*> Objects;
	Objects.Add(Template);
	GetObjectsWithOuter(Template, Objects, EGetObjectsFlags::IncludeNestedObjects, RF_Transient);
	const FString RootPath = Template->GetPathName();
	for (UObject* Object : Objects)
	{
		const FString ObjectKey = Object == Template ? TEXT("@Component@") : Object->GetPathName(Template);
		Out.Add(ObjectKey + TEXT("|Class"), CanonicalValue(Object->GetClass()->GetPathName(), RootPath, Map));
		for (FProperty* Property = Object->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			// Same reflected-property selection as Engine's FComponentComparisonHelper.
			if (!Property->ShouldDuplicateValue())
			{
				continue;
			}
			for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
			{
				FString Value;
				// Data == Delta forces full export. A null Delta can return false for
				// default/null values: that means "no difference", not a read error.
				if (!Property->ExportText_InContainer(Index, Value, Object, Object, Object, PPF_None))
				{
					Messages.Add(TEXT("Cannot export component property: ") + Property->GetName());
					return false;
				}
				const FString Key = FString::Printf(TEXT("%s|%s[%d]"), *ObjectKey, *Property->GetName(), Index);
				Out.Add(Key, CanonicalValue(MoveTemp(Value), RootPath, Map));
			}
		}
	}
	return true;
}

USCS_Node* ResolveNode(const FInheritedOverrideSnapshot& Snapshot, const TMap<FString, FString>& Map,
	UBlueprint* Destination, TArray<FString>& Messages)
{
	UBlueprint* Owner = LoadBlueprint(MappedPackage(Snapshot.OwnerPackage, Map));
	if (Owner && Owner->GeneratedClass && Owner->SimpleConstructionScript && Destination
		&& Destination->ParentClass && Destination->ParentClass->IsChildOf(Owner->GeneratedClass))
	{
		for (USCS_Node* Node : Owner->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->VariableGuid == Snapshot.NodeGuid && Node->GetVariableName() == Snapshot.VariableName)
			{
				return Node;
			}
		}
	}
	Messages.Add(TEXT("Cannot match copied SCS owner/GUID/name/ancestry: ") + Snapshot.SourcePackage
		+ TEXT(" / ") + Snapshot.VariableName.ToString());
	return nullptr;
}

bool IsCompiled(UBlueprint* Blueprint)
{
	return Blueprint && Blueprint->GeneratedClass
		&& (Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
}
}

bool FInheritedOverrideState::Capture(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages)
{
	Snapshots.Reset();
	ExpectedCounts.Reset();
	for (const TPair<FString, FString>& Pair : PackageMap)
	{
		// Canonical property paths require the object/class leaf names to be retained.
		if (!FPackageName::GetShortName(Pair.Key).Equals(FPackageName::GetShortName(Pair.Value), ESearchCase::CaseSensitive))
		{
			Messages.Add(TEXT("Override preservation currently requires unchanged asset leaf names: ") + Pair.Key);
			return false;
		}
		UObject* Asset = LoadObject<UObject>(nullptr, *(Pair.Key + TEXT(".") + FPackageName::GetShortName(Pair.Key)));
		if (!Asset)
		{
			Messages.Add(TEXT("Cannot load original: ") + Pair.Key);
			return false;
		}
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		if (!Blueprint)
		{
			continue;
		}
		if (!IsCompiled(Blueprint))
		{
			Messages.Add(TEXT("Original Blueprint is not compiled; do not compile/save it automatically: ") + Pair.Key);
			return false;
		}
		ExpectedCounts.Add(Pair.Key, 0);
		UInheritableComponentHandler* Handler = Blueprint->GetInheritableComponentHandler(false);
		if (!Handler)
		{
			continue;
		}
		for (auto It = Handler->CreateRecordIterator(); It; ++It)
		{
			const FComponentOverrideRecord& Record = *It;
			const FComponentKey& Key = Record.ComponentKey;
			UClass* OwnerClass = Key.GetComponentOwner();
			UBlueprint* Owner = OwnerClass ? Cast<UBlueprint>(OwnerClass->ClassGeneratedBy) : nullptr;
			if (!Key.IsSCSKey() || !Owner || !Key.FindSCSNode() || !IsValid(Record.ComponentTemplate)
				|| Record.ComponentTemplate->GetClass()->ClassGeneratedBy)
			{
				Messages.Add(TEXT("Unverified override kind (UCS, missing template, or Blueprint-defined component class); no copy: ") + Pair.Key);
				return false;
			}
			FInheritedOverrideSnapshot Snapshot;
			Snapshot.SourcePackage = Pair.Key;
			Snapshot.OwnerPackage = Owner->GetOutermost()->GetName();
			Snapshot.NodeGuid = Key.GetAssociatedGuid();
			Snapshot.VariableName = Key.GetSCSVariableName();
			Snapshot.OriginalTemplate.Reset(Record.ComponentTemplate);
			if (!ReadProperties(Record.ComponentTemplate, PackageMap, Snapshot.Properties, Messages))
			{
				return false;
			}
			// This is a data snapshot, not an inherited template owned by a UClass.
			// BlueprintSupport's deferred initializer assumes UClass Outer when the
			// inherited-template flag is present. Strip template identity BEFORE
			// construction, as AActor::CreateComponentFromTemplate does.
			FObjectDuplicationParameters SnapshotParameters(Record.ComponentTemplate, GetTransientPackage());
			SnapshotParameters.DestName = MakeUniqueObjectName(GetTransientPackage(),
				Record.ComponentTemplate->GetClass(), TEXT("DCOverrideSnapshot"));
			SnapshotParameters.FlagMask = RF_AllFlags & ~(RF_ArchetypeObject | RF_Transactional
				| RF_WasLoaded | RF_Public | RF_Standalone | RF_InheritableComponentTemplate);
			SnapshotParameters.PortFlags = PPF_DuplicateVerbatim;
			Snapshot.Template.Reset(Cast<UActorComponent>(StaticDuplicateObjectEx(SnapshotParameters)));
			if (!Snapshot.Template.IsValid())
			{
				Messages.Add(TEXT("Could not snapshot original override: ") + Pair.Key);
				return false;
			}
			// Only the root snapshot is marked transient. Do not force this flag on
			// every owned object; their persistent property values must be preserved.
			Snapshot.Template->SetFlags(RF_Transient);
			Snapshots.Add(MoveTemp(Snapshot));
			++ExpectedCounts.FindChecked(Pair.Key);
		}
	}
	Messages.Add(FString::Printf(TEXT("Captured %d original SCS overrides before copy."), Snapshots.Num()));
	return true;
}

bool FInheritedOverrideState::RestoreAndCompile(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages)
{
	FEngineDiagnosticGuard Diagnostics;
	if (!Diagnostics.Check(TEXT("before_override_restore"), Messages)) { return false; }
	TArray<UBlueprint*> Blueprints;
	TMap<UObject*, UObject*> Replacements;
	for (const TPair<FString, FString>& Pair : PackageMap)
	{
		UObject* Source = LoadObject<UObject>(nullptr, *(Pair.Key + TEXT(".") + FPackageName::GetShortName(Pair.Key)));
		UObject* Target = FindObject<UObject>(nullptr, *(Pair.Value + TEXT(".") + FPackageName::GetShortName(Pair.Value)));
		if (!Source || !Target)
		{
			Messages.Add(TEXT("Missing copied asset, cannot restore: ") + Pair.Value);
			return false;
		}
		Replacements.Add(Source, Target);
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Target))
		{
			Blueprints.Add(Blueprint);
		}
	}
	// Compile parents first, then reacquire each child's handler/node after reinstancing.
	Blueprints.Sort([](const UBlueprint& A, const UBlueprint& B)
	{
		const auto Depth = [](UClass* Class) { int32 N = 0; for (; Class; Class = Class->GetSuperClass()) { ++N; } return N; };
		return Depth(A.GeneratedClass) < Depth(B.GeneratedClass);
	});
	for (UBlueprint* Blueprint : Blueprints)
	{
		if (!Diagnostics.Check(TEXT("before_override_blueprint"), Messages)) { return false; }
		for (const TPair<FString, FString>& Pair : PackageMap)
		{
			// Populate class/CDO references from live objects after any preceding compile.
			UBlueprint* Source = Cast<UBlueprint>(FindObject<UObject>(nullptr, *(Pair.Key + TEXT(".") + FPackageName::GetShortName(Pair.Key))));
			UBlueprint* Target = Cast<UBlueprint>(FindObject<UObject>(nullptr, *(Pair.Value + TEXT(".") + FPackageName::GetShortName(Pair.Value))));
			if (Source && Source->GeneratedClass && Target && Target->GeneratedClass)
			{
				Replacements.Add(Source->GeneratedClass, Target->GeneratedClass);
				Replacements.Add(Source->GeneratedClass->GetDefaultObject(), Target->GeneratedClass->GetDefaultObject());
				if (Source->SimpleConstructionScript && Target->SimpleConstructionScript)
				{
					for (USCS_Node* OldNode : Source->SimpleConstructionScript->GetAllNodes())
					{
						for (USCS_Node* NewNode : Target->SimpleConstructionScript->GetAllNodes())
						{
							if (OldNode && NewNode && OldNode->VariableGuid == NewNode->VariableGuid
								&& OldNode->ComponentTemplate && NewNode->ComponentTemplate)
							{
								Replacements.Add(OldNode->ComponentTemplate, NewNode->ComponentTemplate);
							}
						}
					}
				}
			}
		}
		TMap<const FInheritedOverrideSnapshot*, UActorComponent*> Pending;
		for (const FInheritedOverrideSnapshot& Snapshot : Snapshots)
		{
			if (MappedPackage(Snapshot.SourcePackage, PackageMap) != Blueprint->GetOutermost()->GetName())
			{
				continue;
			}
			USCS_Node* Node = ResolveNode(Snapshot, PackageMap, Blueprint, Messages);
			if (!Node)
			{
				return false;
			}
			UInheritableComponentHandler* Handler = Blueprint->GetInheritableComponentHandler(true);
			if (!Handler)
			{
				Messages.Add(TEXT("Could not create destination override handler."));
				return false;
			}
			const FComponentKey Key(Node);
			UActorComponent* Target = Handler->GetOverridenComponentTemplate(Key);
			if (!Target)
			{
				Target = Handler->CreateOverridenComponentTemplate(Key);
			}
			if (!Diagnostics.Check(TEXT("after_override_template"), Messages)) { return false; }
			if (!Target || Target->GetClass() != Snapshot.Template->GetClass()
				|| Target->GetOutermost() != Blueprint->GetOutermost())
			{
				Messages.Add(TEXT("Override target class/package mismatch."));
				return false;
			}
			Replacements.Add(Snapshot.OriginalTemplate.Get(), Target);
			Pending.Add(&Snapshot, Target);
		}
		for (const TPair<const FInheritedOverrideSnapshot*, UActorComponent*>& Item : Pending)
		{
			UEngine::FCopyPropertiesForUnrelatedObjectsParams Params;
			Params.bDoDelta = false;
			Params.bNotifyObjectReplacement = false;
			Params.bClearReferences = false;
			Params.bReplaceInternalReferenceUponRead = true;
			Replacements.Add(Item.Key->Template.Get(), Item.Value);
			Params.OptionalReplacementMappings = &Replacements;
			UEngine::CopyPropertiesForUnrelatedObjects(Item.Key->Template.Get(), Item.Value, Params);
			if (!Diagnostics.Check(TEXT("after_override_properties"), Messages)) { return false; }
			// Apply only known mappings inside the copy; do not null unknown references.
			FArchiveReplaceObjectRef<UObject> Replace(Item.Value, Replacements,
				EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
			if (!Diagnostics.Check(TEXT("after_override_references"), Messages)) { return false; }
			Blueprint->MarkPackageDirty();
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection);
		if (!Diagnostics.Check(TEXT("after_override_compile"), Messages)) { return false; }
		if (!IsCompiled(Blueprint))
		{
			Messages.Add(TEXT("Repaired Blueprint failed Compile: ") + Blueprint->GetPathName());
			return false;
		}
	}
	int32 Compared = 0;
	return Compare(PackageMap, Messages, Compared) && Diagnostics.Check(TEXT("after_override_compare"), Messages);
}

bool FInheritedOverrideState::Compare(const TMap<FString, FString>& PackageMap, TArray<FString>& Messages, int32& OutCompared) const
{
	OutCompared = 0;
	bool bMatches = true;
	for (const TPair<FString, int32>& Pair : ExpectedCounts)
	{
		UBlueprint* Blueprint = LoadBlueprint(MappedPackage(Pair.Key, PackageMap));
		UInheritableComponentHandler* Handler = Blueprint ? Blueprint->GetInheritableComponentHandler(false) : nullptr;
		int32 Count = 0;
		if (Handler)
		{
			for (auto It = Handler->CreateRecordIterator(); It; ++It) { ++Count; }
		}
		if (!Blueprint || Count != Pair.Value)
		{
			Messages.Add(TEXT("Override record count differs: ") + Pair.Key);
			bMatches = false;
		}
	}
	for (const FInheritedOverrideSnapshot& Snapshot : Snapshots)
	{
		UBlueprint* Blueprint = LoadBlueprint(MappedPackage(Snapshot.SourcePackage, PackageMap));
		USCS_Node* Node = ResolveNode(Snapshot, PackageMap, Blueprint, Messages);
		UInheritableComponentHandler* Handler = Blueprint ? Blueprint->GetInheritableComponentHandler(false) : nullptr;
		UActorComponent* Target = Node && Handler ? Handler->GetOverridenComponentTemplate(FComponentKey(Node)) : nullptr;
		TMap<FString, FString> Actual;
		// Only expected ORIGINAL references are translated. Translating actual old
		// references too would conceal a failed reference remap in the copy.
		const TMap<FString, FString> NoTranslation;
		if (!ReadProperties(Target, NoTranslation, Actual, Messages))
		{
			bMatches = false;
			continue;
		}
		++OutCompared;
		for (const TPair<FString, FString>& Property : Snapshot.Properties)
		{
			const FString* Value = Actual.Find(Property.Key);
			if (!Value || !Value->Equals(Property.Value, ESearchCase::CaseSensitive))
			{
				const FString ActualText = Value ? Value->Left(256) : TEXT("<missing>");
				Messages.Add(FString::Printf(TEXT("Override property differs: %s / %s expected=%s actual=%s"),
					*Snapshot.SourcePackage, *Property.Key, *Property.Value.Left(256), *ActualText));
				bMatches = false;
			}
		}
		for (const TPair<FString, FString>& Property : Actual)
		{
			if (!Snapshot.Properties.Contains(Property.Key))
			{
				Messages.Add(TEXT("Unexpected override property/object: ") + Property.Key);
				bMatches = false;
			}
		}
		if (!Snapshot.Template->AreNativePropertiesIdenticalTo(Target))
		{
			Messages.Add(TEXT("Native component properties differ: ") + Snapshot.SourcePackage);
			bMatches = false;
		}
	}
	Messages.Add(FString::Printf(TEXT("Override comparison: expected=%d compared=%d match=%d"), Snapshots.Num(), OutCompared, bMatches));
	return bMatches;
}
}
