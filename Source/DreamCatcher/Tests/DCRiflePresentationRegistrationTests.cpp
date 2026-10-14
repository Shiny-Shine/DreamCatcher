// Read-only tests for the explicitly approved presentation/state native redirects.
// No actor/CDO creation, delegate broadcast, Blueprint mutation, or asset saves.
#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "CoreGlobals.h"
#include "Containers/ArrayView.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCRiflePresentationRegistrationTests
{
struct FRedirectEntry
{
	ECoreRedirectFlags Kind;
	const TCHAR* Source;
	const TCHAR* Target;
};

static const FRedirectEntry Types[] =
{
	{ ECoreRedirectFlags::Type_Class, TEXT("/Script/LyraGame.LyraHealthComponent"), TEXT("/Script/DreamCatcher.DCLyraHealthComponent") },
	{ ECoreRedirectFlags::Type_Class, TEXT("/Script/LyraGame.LyraPawnComponent_CharacterParts"), TEXT("/Script/DreamCatcher.DCLyraPawnComponent_CharacterParts") },
	{ ECoreRedirectFlags::Type_Class, TEXT("/Script/LyraGame.AsyncAction_ObserveTeamColors"), TEXT("/Script/DreamCatcher.DCLyraAsyncAction_ObserveTeamColors") },
	{ ECoreRedirectFlags::Type_Class, TEXT("/Script/LyraGame.LyraTeamAgentInterface"), TEXT("/Script/DreamCatcher.DCTeamAgentInterface") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAbilityMontageFailureMessage"), TEXT("/Script/DreamCatcher.DCAbilityMontageFailureMessage") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraVerbMessage"), TEXT("/Script/DreamCatcher.DCVerbMessage") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraCharacterPart"), TEXT("/Script/DreamCatcher.DCLyraCharacterPart") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraCharacterPartHandle"), TEXT("/Script/DreamCatcher.DCLyraCharacterPartHandle") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraCharacterPartList"), TEXT("/Script/DreamCatcher.DCLyraCharacterPartList") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAppliedCharacterPartEntry"), TEXT("/Script/DreamCatcher.DCLyraAppliedCharacterPartEntry") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAnimLayerSelectionEntry"), TEXT("/Script/DreamCatcher.DCLyraAnimLayerSelectionEntry") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAnimLayerSelectionSet"), TEXT("/Script/DreamCatcher.DCLyraAnimLayerSelectionSet") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAnimBodyStyleSelectionEntry"), TEXT("/Script/DreamCatcher.DCLyraAnimBodyStyleSelectionEntry") },
	{ ECoreRedirectFlags::Type_Struct, TEXT("/Script/LyraGame.LyraAnimBodyStyleSelectionSet"), TEXT("/Script/DreamCatcher.DCLyraAnimBodyStyleSelectionSet") },
	{ ECoreRedirectFlags::Type_Enum, TEXT("/Script/LyraGame.ELyraDeathState"), TEXT("/Script/DreamCatcher.EDCLyraDeathState") },
	{ ECoreRedirectFlags::Type_Enum, TEXT("/Script/LyraGame.ECharacterCustomizationCollisionMode"), TEXT("/Script/DreamCatcher.EDCLyraCharacterCustomizationCollisionMode") },
};
static const FRedirectEntry Delegates[] =
{
	{ ECoreRedirectFlags::Type_Function, TEXT("/Script/LyraGame.LyraHealth_DeathEvent__DelegateSignature"), TEXT("/Script/DreamCatcher.DCLyraHealth_DeathEvent__DelegateSignature") },
	{ ECoreRedirectFlags::Type_Function, TEXT("/Script/LyraGame.LyraHealth_AttributeChanged__DelegateSignature"), TEXT("/Script/DreamCatcher.DCLyraHealth_AttributeChanged__DelegateSignature") },
	{ ECoreRedirectFlags::Type_Function, TEXT("/Script/LyraGame.LyraSpawnedCharacterPartsChanged__DelegateSignature"), TEXT("/Script/DreamCatcher.DCLyraSpawnedCharacterPartsChanged__DelegateSignature") },
	{ ECoreRedirectFlags::Type_Function, TEXT("/Script/LyraGame.TeamColorObservedAsyncDelegate__DelegateSignature"), TEXT("/Script/DreamCatcher.DCLyraTeamColorObservedAsyncDelegate__DelegateSignature") },
};
static_assert(UE_ARRAY_COUNT(Types) == 16, "Keep the approved type boundary.");
static_assert(UE_ARRAY_COUNT(Delegates) == 4, "Keep the approved delegate boundary.");

bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Read-only tests require the DreamCatcher editor"),
		GIsEditor && !IsRunningCommandlet() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher"));
}

UObject* LoadMapped(FAutomationTestBase& Test, const FRedirectEntry& Entry)
{
	UObject* Target = FindObject<UObject>(nullptr, Entry.Target);
	if (!Test.TestNotNull(FString::Printf(TEXT("Target is already registered: %s"), Entry.Target), Target)) { return nullptr; }
	const bool bKindMatches =
		(Entry.Kind == ECoreRedirectFlags::Type_Class && Target->IsA<UClass>()) ||
		(Entry.Kind == ECoreRedirectFlags::Type_Struct && Target->IsA<UScriptStruct>()) ||
		(Entry.Kind == ECoreRedirectFlags::Type_Enum && Target->IsA<UEnum>()) ||
		(Entry.Kind == ECoreRedirectFlags::Type_Function && Target->IsA<UFunction>());
	if (!Test.TestTrue(FString::Printf(TEXT("Target kind: %s"), Entry.Target), bKindMatches)) { return nullptr; }
	const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(Entry.Kind, FCoreRedirectObjectName(FString(Entry.Source)));
	const FCoreRedirectObjectName Expected(FString(Entry.Target));
	if (!Test.TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), Expected.ToString())) { return nullptr; }
	FSoftObjectPath Path(Entry.Source);
	if (!Test.TestTrue(TEXT("Fix up native path before loading"), Path.FixupCoreRedirects())) { return nullptr; }
	if (!Test.TestEqual(TEXT("Only the approved destination may be loaded"), Path.ToString(), FString(Entry.Target))) { return nullptr; }
	if (!Test.TestTrue(TEXT("Fixed-up native path resolves to the expected object"), Path.TryLoad() == Target)) { return nullptr; }
	return Target;
}

enum class EParameterKind : uint8 { Bool, Int, Float, Object };
struct FExpectedParameter
{
	const TCHAR* Name;
	EParameterKind Kind;
	const TCHAR* ObjectClassPath = nullptr;
	bool bConst = false;
};

void CheckParameters(FAutomationTestBase& Test, UFunction* Signature, TConstArrayView<FExpectedParameter> Expected)
{
	Test.TestTrue(TEXT("Dynamic multicast signature flags"), Signature->HasAllFunctionFlags(FUNC_Delegate | FUNC_MulticastDelegate | FUNC_Public));
	Test.TestNull(TEXT("Multicast signature returns void"), Signature->GetReturnProperty());
	const uint64 RelevantFlags = static_cast<uint64>(CPF_Parm | CPF_ReturnParm | CPF_OutParm | CPF_ReferenceParm | CPF_ConstParm);
	int32 Index = 0;
	for (TFieldIterator<FProperty> It(Signature, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_Parm)) { continue; }
		if (!Test.TestTrue(TEXT("No unexpected delegate parameters"), Index < Expected.Num())) { break; }
		const FExpectedParameter& Parameter = Expected[Index];
		Test.TestEqual(FString::Printf(TEXT("Delegate parameter order %d"), Index), It->GetName(), FString(Parameter.Name));
		uint64 ExpectedFlags = static_cast<uint64>(CPF_Parm);
		if (Parameter.bConst) { ExpectedFlags |= static_cast<uint64>(CPF_ConstParm); }
		Test.TestEqual(TEXT("Delegate parameter const/reference/out qualifiers"),
			static_cast<uint64>(It->GetPropertyFlags()) & RelevantFlags, ExpectedFlags);
		switch (Parameter.Kind)
		{
		case EParameterKind::Bool:
			Test.TestNotNull(TEXT("Bool parameter"), CastField<FBoolProperty>(*It));
			break;
		case EParameterKind::Int:
			Test.TestNotNull(TEXT("Int32 parameter"), CastField<FIntProperty>(*It));
			break;
		case EParameterKind::Float:
			Test.TestNotNull(TEXT("Float parameter"), CastField<FFloatProperty>(*It));
			break;
		case EParameterKind::Object:
		{
			const FObjectPropertyBase* ObjectParameter = CastField<FObjectPropertyBase>(*It);
			UClass* ExpectedClass = FindObject<UClass>(nullptr, Parameter.ObjectClassPath);
			const bool bHasProperty = Test.TestNotNull(TEXT("Object parameter"), ObjectParameter);
			const bool bHasClass = Test.TestNotNull(TEXT("Parameter class already registered"), ExpectedClass);
			if (bHasProperty && bHasClass)
			{
				Test.TestTrue(TEXT("Exact delegate object parameter type"), ObjectParameter->PropertyClass == ExpectedClass);
			}
			break;
		}
		}
		++Index;
	}
	Test.TestEqual(TEXT("Exact delegate parameter count"), Index, Expected.Num());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRiflePresentationTypesTest,
	"DreamCatcher.R5.PresentationRegistration.NativeTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRiflePresentationTypesTest::RunTest(const FString& Parameters)
{
	using namespace DCRiflePresentationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	for (const FRedirectEntry& Entry : Types)
	{
		UObject* Target = LoadMapped(*this, Entry);
		if (UClass* Class = Cast<UClass>(Target))
		{
			TestTrue(TEXT("Registered class is native"), Class->HasAnyClassFlags(CLASS_Native));
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRiflePresentationEnumsTest,
	"DreamCatcher.R5.PresentationRegistration.EnumValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRiflePresentationEnumsTest::RunTest(const FString& Parameters)
{
	using namespace DCRiflePresentationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	// Values are from both headers and the source audit, not assumed from type names.
	UEnum* Death = FindObject<UEnum>(nullptr, TEXT("/Script/DreamCatcher.EDCLyraDeathState"));
	UEnum* Collision = FindObject<UEnum>(nullptr, TEXT("/Script/DreamCatcher.EDCLyraCharacterCustomizationCollisionMode"));
	if (!TestNotNull(TEXT("Death enum registered"), Death) || !TestNotNull(TEXT("Collision enum registered"), Collision)) { return false; }
	const TCHAR* DeathNames[] = { TEXT("NotDead"), TEXT("DeathStarted"), TEXT("DeathFinished"), TEXT("EDCLyraDeathState_MAX") };
	TestEqual(TEXT("Death enum count including generated sentinel"), Death->NumEnums(), static_cast<int32>(UE_ARRAY_COUNT(DeathNames)));
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(DeathNames)) && Index < Death->NumEnums(); ++Index)
	{
		TestEqual(TEXT("Death enum member name"), Death->GetNameStringByIndex(Index), FString(DeathNames[Index]));
		TestEqual(TEXT("Death enum numeric value"), Death->GetValueByIndex(Index), static_cast<int64>(Index));
		TestEqual(TEXT("Death enum name resolves to value"), Death->GetValueByNameString(DeathNames[Index]), static_cast<int64>(Index));
	}
	const TCHAR* CollisionNames[] = { TEXT("NoCollision"), TEXT("UseCollisionFromCharacterPart"), TEXT("EDCLyraCharacterCustomizationCollisionMode_MAX") };
	TestEqual(TEXT("Collision enum count including generated sentinel"), Collision->NumEnums(), static_cast<int32>(UE_ARRAY_COUNT(CollisionNames)));
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(CollisionNames)) && Index < Collision->NumEnums(); ++Index)
	{
		TestEqual(TEXT("Collision enum member name"), Collision->GetNameStringByIndex(Index), FString(CollisionNames[Index]));
		TestEqual(TEXT("Collision enum numeric value"), Collision->GetValueByIndex(Index), static_cast<int64>(Index));
		TestEqual(TEXT("Collision enum name resolves to value"), Collision->GetValueByNameString(CollisionNames[Index]), static_cast<int64>(Index));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRiflePresentationDelegatesTest,
	"DreamCatcher.R5.PresentationRegistration.DelegateSignatures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRiflePresentationDelegatesTest::RunTest(const FString& Parameters)
{
	using namespace DCRiflePresentationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	const FExpectedParameter Death[] = { { TEXT("OwningActor"), EParameterKind::Object, TEXT("/Script/Engine.Actor") } };
	const FExpectedParameter Attribute[] =
	{
		{ TEXT("HealthComponent"), EParameterKind::Object, TEXT("/Script/DreamCatcher.DCLyraHealthComponent") },
		{ TEXT("OldValue"), EParameterKind::Float },
		{ TEXT("NewValue"), EParameterKind::Float },
		{ TEXT("Instigator"), EParameterKind::Object, TEXT("/Script/Engine.Actor") },
	};
	const FExpectedParameter Parts[] =
	{
		{ TEXT("ComponentWithChangedParts"), EParameterKind::Object, TEXT("/Script/DreamCatcher.DCLyraPawnComponent_CharacterParts") },
	};
	const FExpectedParameter Team[] =
	{
		{ TEXT("bTeamSet"), EParameterKind::Bool },
		{ TEXT("TeamId"), EParameterKind::Int },
		{ TEXT("DisplayAsset"), EParameterKind::Object, TEXT("/Script/DreamCatcher.DCTeamDisplayAsset"), true },
	};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Delegates)); ++Index)
	{
		UFunction* Signature = Cast<UFunction>(LoadMapped(*this, Delegates[Index]));
		if (!Signature) { continue; }
		TestTrue(TEXT("Delegate signature remains package-level"), Signature->GetOuter() == Signature->GetOutermost());
		switch (Index)
		{
		case 0: CheckParameters(*this, Signature, MakeArrayView(Death)); break;
		case 1: CheckParameters(*this, Signature, MakeArrayView(Attribute)); break;
		case 2: CheckParameters(*this, Signature, MakeArrayView(Parts)); break;
		case 3: CheckParameters(*this, Signature, MakeArrayView(Team)); break;
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRiflePresentationBindingsTest,
	"DreamCatcher.R5.PresentationRegistration.DelegateProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRiflePresentationBindingsTest::RunTest(const FString& Parameters)
{
	using namespace DCRiflePresentationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	struct FPropertySignature
	{
		const TCHAR* OwnerClass;
		const TCHAR* Property;
		int32 DelegateIndex;
	};
	const FPropertySignature Bindings[] =
	{
		{ TEXT("/Script/DreamCatcher.DCLyraHealthComponent"), TEXT("OnDeathStarted"), 0 },
		{ TEXT("/Script/DreamCatcher.DCLyraHealthComponent"), TEXT("OnDeathFinished"), 0 },
		{ TEXT("/Script/DreamCatcher.DCLyraHealthComponent"), TEXT("OnHealthChanged"), 1 },
		{ TEXT("/Script/DreamCatcher.DCLyraHealthComponent"), TEXT("OnMaxHealthChanged"), 1 },
		{ TEXT("/Script/DreamCatcher.DCLyraPawnComponent_CharacterParts"), TEXT("OnCharacterPartsChanged"), 2 },
		{ TEXT("/Script/DreamCatcher.DCLyraAsyncAction_ObserveTeamColors"), TEXT("OnTeamChanged"), 3 },
	};
	for (const FPropertySignature& Binding : Bindings)
	{
		UClass* Owner = FindObject<UClass>(nullptr, Binding.OwnerClass);
		if (!TestNotNull(TEXT("Delegate property owner registered"), Owner)) { continue; }
		const FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(Owner, Binding.Property);
		if (!TestNotNull(FString::Printf(TEXT("Multicast property %s"), Binding.Property), Property)) { continue; }
		UFunction* Signature = FindObject<UFunction>(nullptr, Delegates[Binding.DelegateIndex].Target);
		if (!TestNotNull(TEXT("Expected signature registered"), Signature)) { continue; }
		TestTrue(TEXT("Property points to the exact registered signature"), Property->SignatureFunction.Get() == Signature);
	}
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
