// Read-only registration tests for the approved R5 native name cohort.
// Only already-registered native types and fixed-up /Script paths are inspected.
// No CDO/ability instances, Blueprint assets, worlds, gameplay calls, or saves.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "CoreGlobals.h"
#include "Engine/MemberReference.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCRifleNativeRegistrationTests
{
struct FNativeTypeRedirect
{
	const TCHAR* Source;
	const TCHAR* Target;
};

static const FNativeTypeRedirect ClassRedirects[] =
{
	{ TEXT("/Script/LyraGame.LyraInventoryItemDefinition"), TEXT("/Script/DreamCatcher.DCInventoryItemDefinition") },
	{ TEXT("/Script/LyraGame.LyraInventoryItemInstance"), TEXT("/Script/DreamCatcher.DCInventoryItemInstance") },
	{ TEXT("/Script/LyraGame.LyraInventoryItemFragment"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
	{ TEXT("/Script/LyraGame.InventoryFragment_EquippableItem"), TEXT("/Script/DreamCatcher.DCInventoryFragment_EquippableItem") },
	{ TEXT("/Script/LyraGame.InventoryFragment_PickupIcon"), TEXT("/Script/DreamCatcher.DCInventoryFragment_PickupIcon") },
	{ TEXT("/Script/LyraGame.InventoryFragment_QuickBarIcon"), TEXT("/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon") },
	{ TEXT("/Script/LyraGame.InventoryFragment_ReticleConfig"), TEXT("/Script/DreamCatcher.DCInventoryFragment_ReticleConfig") },
	{ TEXT("/Script/LyraGame.InventoryFragment_SetStats"), TEXT("/Script/DreamCatcher.DCInventoryFragment_SetStats") },
	{ TEXT("/Script/LyraGame.LyraEquipmentDefinition"), TEXT("/Script/DreamCatcher.DCLyraEquipmentDefinition") },
	{ TEXT("/Script/LyraGame.LyraEquipmentInstance"), TEXT("/Script/DreamCatcher.DCLyraEquipmentInstance") },
	{ TEXT("/Script/LyraGame.LyraEquipmentManagerComponent"), TEXT("/Script/DreamCatcher.DCLyraEquipmentManagerComponent") },
	{ TEXT("/Script/LyraGame.LyraQuickBarComponent"), TEXT("/Script/DreamCatcher.DCLyraQuickBarComponent") },
	{ TEXT("/Script/LyraGame.LyraGameplayAbility_FromEquipment"), TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_FromEquipment") },
	{ TEXT("/Script/LyraGame.LyraWeaponInstance"), TEXT("/Script/DreamCatcher.DCLyraWeaponInstance") },
	{ TEXT("/Script/LyraGame.LyraRangedWeaponInstance"), TEXT("/Script/DreamCatcher.DCLyraRangedWeaponInstance") },
	{ TEXT("/Script/LyraGame.LyraGameplayAbility_RangedWeapon"), TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_RangedWeapon") },
	{ TEXT("/Script/LyraGame.LyraAbilityCost_ItemTagStack"), TEXT("/Script/DreamCatcher.DCLyraAbilityCost_ItemTagStack") },
	{ TEXT("/Script/LyraGame.LyraReticleWidgetBase"), TEXT("/Script/DreamCatcher.DCLyraReticleWidgetBase") },
	{ TEXT("/Script/LyraGame.LyraAbilitySet"), TEXT("/Script/DreamCatcher.DCAbilitySet") },
	{ TEXT("/Script/LyraGame.LyraAbilityCost"), TEXT("/Script/DreamCatcher.DCAbilityCost") },
	{ TEXT("/Script/LyraGame.LyraAbilitySystemComponent"), TEXT("/Script/DreamCatcher.DCAbilitySystemComponent") },
};

static const FNativeTypeRedirect StructRedirects[] =
{
	{ TEXT("/Script/LyraGame.GameplayTagStack"), TEXT("/Script/DreamCatcher.GameplayTagStack") },
	{ TEXT("/Script/LyraGame.GameplayTagStackContainer"), TEXT("/Script/DreamCatcher.GameplayTagStackContainer") },
	{ TEXT("/Script/LyraGame.LyraEquipmentActorToSpawn"), TEXT("/Script/DreamCatcher.DCLyraEquipmentActorToSpawn") },
	{ TEXT("/Script/LyraGame.LyraEquipmentList"), TEXT("/Script/DreamCatcher.DCLyraEquipmentList") },
	{ TEXT("/Script/LyraGame.LyraAppliedEquipmentEntry"), TEXT("/Script/DreamCatcher.DCLyraAppliedEquipmentEntry") },
	{ TEXT("/Script/LyraGame.LyraAbilitySet_GameplayAbility"), TEXT("/Script/DreamCatcher.DCAbilitySet_GameplayAbility") },
	{ TEXT("/Script/LyraGame.LyraAbilitySet_GameplayEffect"), TEXT("/Script/DreamCatcher.DCAbilitySet_GameplayEffect") },
	{ TEXT("/Script/LyraGame.LyraAbilitySet_AttributeSet"), TEXT("/Script/DreamCatcher.DCAbilitySet_AttributeSet") },
	{ TEXT("/Script/LyraGame.LyraAbilitySet_GrantedHandles"), TEXT("/Script/DreamCatcher.DCAbilitySet_GrantedHandles") },
};

static_assert(UE_ARRAY_COUNT(ClassRedirects) == 21, "Keep the approved class boundary.");
static_assert(UE_ARRAY_COUNT(StructRedirects) == 9, "Keep the approved struct boundary.");

bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Read-only registration tests require the DreamCatcher editor"),
		GIsEditor && !IsRunningCommandlet() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher"));
}

UClass* FindTargetClass(FAutomationTestBase& Test, const TCHAR* Path)
{
	UClass* Result = FindObject<UClass>(nullptr, Path);
	if (!Test.TestNotNull(FString::Printf(TEXT("Native class already registered: %s"), Path), Result)) { return nullptr; }
	Test.TestTrue(FString::Printf(TEXT("Class is native: %s"), Path), Result->HasAnyClassFlags(CLASS_Native));
	return Result;
}

bool CheckRedirect(FAutomationTestBase& Test, ECoreRedirectFlags Kind, const FNativeTypeRedirect& Entry)
{
	const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(Kind, FCoreRedirectObjectName(FString(Entry.Source)));
	const FCoreRedirectObjectName Expected(FString(Entry.Target));
	return Test.TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), Expected.ToString());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeClassesTest,
	"DreamCatcher.R5.NativeRegistration.Classes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeClassesTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	for (const FNativeTypeRedirect& Entry : ClassRedirects)
	{
		UClass* Target = FindTargetClass(*this, Entry.Target);
		if (!Target || !CheckRedirect(*this, ECoreRedirectFlags::Type_Class, Entry)) { continue; }
		FSoftClassPath Path(Entry.Source);
		if (!TestTrue(FString::Printf(TEXT("Fix up before native class load: %s"), Entry.Source), Path.FixupCoreRedirects())) { continue; }
		if (!TestEqual(TEXT("Fixed-up class path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(FString::Printf(TEXT("Native class path resolves: %s"), Entry.Source), Path.TryLoadClass<UObject>() == Target);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeStructsTest,
	"DreamCatcher.R5.NativeRegistration.Structs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeStructsTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	for (const FNativeTypeRedirect& Entry : StructRedirects)
	{
		UScriptStruct* Target = FindObject<UScriptStruct>(nullptr, Entry.Target);
		if (!TestNotNull(FString::Printf(TEXT("Native script struct already registered: %s"), Entry.Target), Target)) { continue; }
		if (!CheckRedirect(*this, ECoreRedirectFlags::Type_Struct, Entry)) { continue; }
		FSoftObjectPath Path(Entry.Source);
		if (!TestTrue(FString::Printf(TEXT("Fix up before native struct load: %s"), Entry.Source), Path.FixupCoreRedirects())) { continue; }
		if (!TestEqual(TEXT("Fixed-up struct path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(FString::Printf(TEXT("Script struct path resolves: %s"), Entry.Source), Path.TryLoad() == Target);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeGetterAliasesTest,
	"DreamCatcher.R5.NativeRegistration.ASCGetterAliases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeGetterAliasesTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	UClass* AbilityClass = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCGameplayAbility"));
	UClass* ASCClass = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCAbilitySystemComponent"));
	if (!AbilityClass || !ASCClass) { return false; }
	UFunction* Getter = AbilityClass->FindFunctionByName(TEXT("GetDCAbilitySystemComponentFromActorInfo"));
	if (!TestNotNull(TEXT("Existing ASC getter found without invoking it"), Getter)) { return false; }
	TestTrue(TEXT("ASC getter retains const/BlueprintCallable contract"), Getter->HasAllFunctionFlags(FUNC_Const | FUNC_BlueprintCallable));
	const FObjectPropertyBase* ReturnProperty = CastField<FObjectPropertyBase>(Getter->GetReturnProperty());
	if (TestNotNull(TEXT("ASC getter has an object return parameter"), ReturnProperty))
	{
		TestTrue(TEXT("ASC return is marked ReturnParm"), ReturnProperty->HasAnyPropertyFlags(CPF_ReturnParm));
		TestTrue(TEXT("ASC getter returns the ported ASC type"), ReturnProperty->PropertyClass == ASCClass);
	}
	int32 InputParameterCount = 0;
	int32 ReturnParameterCount = 0;
	for (TFieldIterator<FProperty> It(Getter, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Parm))
		{
			if (It->HasAnyPropertyFlags(CPF_ReturnParm)) { ++ReturnParameterCount; }
			else { ++InputParameterCount; }
		}
	}
	TestEqual(TEXT("ASC getter takes no explicit inputs"), InputParameterCount, 0);
	TestEqual(TEXT("ASC getter has exactly one return value"), ReturnParameterCount, 1);

	const FNativeTypeRedirect Aliases[] =
	{
		{ TEXT("/Script/LyraGame.LyraGameplayAbility.GetLyraAbilitySystemComponentFromActorInfo"),
			TEXT("/Script/DreamCatcher.DCGameplayAbility.GetDCAbilitySystemComponentFromActorInfo") },
		{ TEXT("/Script/DreamCatcher.DCGameplayAbility.GetLyraAbilitySystemComponentFromActorInfo"),
			TEXT("/Script/DreamCatcher.DCGameplayAbility.GetDCAbilitySystemComponentFromActorInfo") },
	};
	for (const FNativeTypeRedirect& Entry : Aliases)
	{
		if (!CheckRedirect(*this, ECoreRedirectFlags::Type_Function, Entry)) { continue; }
		const FCoreRedirectObjectName Resolved = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Function, FCoreRedirectObjectName(FString(Entry.Source)));
		TestTrue(TEXT("Both names locate the same native getter"), AbilityClass->FindFunctionByName(Resolved.ObjectName) == Getter);
	}

	UClass* Scopes[] =
	{
		AbilityClass,
		FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_FromEquipment")),
		FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_RangedWeapon")),
	};
	for (UClass* Scope : Scopes)
	{
		if (!Scope) { continue; }
		FMemberReference LegacyReference;
		LegacyReference.SetExternalMember(TEXT("GetLyraAbilitySystemComponentFromActorInfo"), Scope);
		// Only this local reference can be fixed up; no Blueprint node or saved asset is involved.
		TestTrue(FString::Printf(TEXT("Legacy getter resolves through native inheritance: %s"), *Scope->GetName()),
			LegacyReference.ResolveMember<UFunction>(Scope, true) == Getter);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleNativeTypeLinksTest,
	"DreamCatcher.R5.NativeRegistration.TypeLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleNativeTypeLinksTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleNativeRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	const FNativeTypeRedirect Inheritance[] =
	{
		{ TEXT("/Script/DreamCatcher.DCInventoryFragment_EquippableItem"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
		{ TEXT("/Script/DreamCatcher.DCInventoryFragment_PickupIcon"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
		{ TEXT("/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
		{ TEXT("/Script/DreamCatcher.DCInventoryFragment_ReticleConfig"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
		{ TEXT("/Script/DreamCatcher.DCInventoryFragment_SetStats"), TEXT("/Script/DreamCatcher.DCInventoryItemFragment") },
		{ TEXT("/Script/DreamCatcher.DCLyraWeaponInstance"), TEXT("/Script/DreamCatcher.DCLyraEquipmentInstance") },
		{ TEXT("/Script/DreamCatcher.DCLyraRangedWeaponInstance"), TEXT("/Script/DreamCatcher.DCLyraWeaponInstance") },
		{ TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_FromEquipment"), TEXT("/Script/DreamCatcher.DCGameplayAbility") },
		{ TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_RangedWeapon"), TEXT("/Script/DreamCatcher.DCLyraGameplayAbility_FromEquipment") },
		{ TEXT("/Script/DreamCatcher.DCLyraAbilityCost_ItemTagStack"), TEXT("/Script/DreamCatcher.DCAbilityCost") },
		{ TEXT("/Script/DreamCatcher.DCAbilitySystemComponent"), TEXT("/Script/GameplayAbilities.AbilitySystemComponent") },
		{ TEXT("/Script/DreamCatcher.DCLyraReticleWidgetBase"), TEXT("/Script/UMG.UserWidget") },
	};
	for (const FNativeTypeRedirect& Link : Inheritance)
	{
		UClass* Child = FindTargetClass(*this, Link.Source);
		UClass* Parent = FindTargetClass(*this, Link.Target);
		if (Child && Parent) { TestTrue(FString::Printf(TEXT("Native inheritance: %s"), Link.Source), Child->IsChildOf(Parent)); }
	}

	UClass* Definition = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCLyraEquipmentDefinition"));
	UClass* Instance = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCLyraEquipmentInstance"));
	UClass* AbilitySet = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCAbilitySet"));
	UClass* Equippable = FindTargetClass(*this, TEXT("/Script/DreamCatcher.DCInventoryFragment_EquippableItem"));
	UScriptStruct* ActorEntry = FindObject<UScriptStruct>(nullptr, TEXT("/Script/DreamCatcher.DCLyraEquipmentActorToSpawn"));
	if (!Definition || !Instance || !AbilitySet || !Equippable || !TestNotNull(TEXT("Equipment actor entry struct registered"), ActorEntry)) { return false; }
	const FClassProperty* InstanceType = FindFProperty<FClassProperty>(Definition, TEXT("InstanceType"));
	if (TestNotNull(TEXT("Equipment InstanceType property"), InstanceType))
	{
		TestTrue(TEXT("Equipment definition names the original-based instance hierarchy"), InstanceType->MetaClass == Instance);
	}
	const FClassProperty* DefinitionType = FindFProperty<FClassProperty>(Equippable, TEXT("EquipmentDefinition"));
	if (TestNotNull(TEXT("Equippable fragment definition property"), DefinitionType))
	{
		TestTrue(TEXT("Inventory fragment links to original-based equipment definition"), DefinitionType->MetaClass == Definition);
	}
	const FArrayProperty* Grants = FindFProperty<FArrayProperty>(Definition, TEXT("AbilitySetsToGrant"));
	if (TestNotNull(TEXT("AbilitySetsToGrant array"), Grants))
	{
		const FObjectPropertyBase* GrantType = CastField<FObjectPropertyBase>(Grants->Inner);
		if (TestNotNull(TEXT("Grant entry is an object reference"), GrantType))
		{
			TestTrue(TEXT("Grant entry uses the ported AbilitySet"), GrantType->PropertyClass == AbilitySet);
		}
	}
	const FArrayProperty* Actors = FindFProperty<FArrayProperty>(Definition, TEXT("ActorsToSpawn"));
	if (TestNotNull(TEXT("ActorsToSpawn array"), Actors))
	{
		const FStructProperty* ActorType = CastField<FStructProperty>(Actors->Inner);
		if (TestNotNull(TEXT("Actor entry is a script struct"), ActorType))
		{
			TestTrue(TEXT("Actor entry uses the mapped spawn struct"), ActorType->Struct == ActorEntry);
		}
	}
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
