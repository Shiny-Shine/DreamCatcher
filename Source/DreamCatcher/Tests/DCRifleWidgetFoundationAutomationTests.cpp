// Read-only native/CDO contracts for the source-derived rifle widget parents.
// No ordinary abstract instances, CDO edits, widgets/players/worlds, Tick/event
// invocation, tag listeners, equipment changes, Blueprint Compile, or asset saves.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "CommonUserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "CoreGlobals.h"
#include "GameplayTagContainer.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UI/Lyra/DCLyraTaggedWidget.h"
#include "UI/Weapons/Lyra/DCLyraWeaponUserInterface.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/EnumProperty.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "Weapon/Lyra/DCLyraWeaponInstance.h"

namespace DCRifleWidgetFoundationTests
{
bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Rifle widget contracts require the DreamCatcher editor on the game thread"),
		GIsEditor && !IsRunningCommandlet() && IsInGameThread() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher"));
}

void CheckVisibilityDefault(FAutomationTestBase& Test, const UDCLyraTaggedWidget* Defaults, const FName Name, ESlateVisibility Expected)
{
	const FEnumProperty* Property = FindFProperty<FEnumProperty>(Defaults->GetClass(), Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is a reflected enum"), Property)) { return; }
	Test.TestTrue(Name.ToString() + TEXT(" retains SlateVisibility"), Property->GetEnum() == StaticEnum<ESlateVisibility>());
	Test.TestTrue(Name.ToString() + TEXT(" remains editable"), Property->HasAnyPropertyFlags(CPF_Edit));
	Test.TestEqual(Name.ToString() + TEXT(" retains the original default"),
		Property->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Defaults)),
		static_cast<uint64>(Expected));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleWidgetRegistrationTest,
	"DreamCatcher.R5.RifleWidgetFoundation.RegistrationAndInheritance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleWidgetRegistrationTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleWidgetFoundationTests;
	if (!CheckContext(*this)) { return false; }
	struct FRedirect
	{
		const TCHAR* Source;
		const TCHAR* Target;
		UClass* Expected;
	};
	const FRedirect Redirects[] =
	{
		{ TEXT("/Script/LyraGame.LyraTaggedWidget"), TEXT("/Script/DreamCatcher.DCLyraTaggedWidget"), UDCLyraTaggedWidget::StaticClass() },
		{ TEXT("/Script/LyraGame.LyraWeaponUserInterface"), TEXT("/Script/DreamCatcher.DCLyraWeaponUserInterface"), UDCLyraWeaponUserInterface::StaticClass() },
	};
	static_assert(UE_ARRAY_COUNT(Redirects) == 2, "Keep the approved rifle widget parent boundary.");
	for (const FRedirect& Entry : Redirects)
	{
		if (!TestNotNull(TEXT("Native widget class exists"), Entry.Expected)
			|| !TestTrue(TEXT("Exact native target already registered"), FindObject<UClass>(nullptr, Entry.Target) == Entry.Expected)) { continue; }
		TestTrue(TEXT("Widget parent is native"), Entry.Expected->HasAnyClassFlags(CLASS_Native));
		TestTrue(TEXT("Original direct CommonUserWidget parent preserved"), Entry.Expected->GetSuperClass() == UCommonUserWidget::StaticClass());
		const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(FString(Entry.Source)));
		if (!TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), FCoreRedirectObjectName(FString(Entry.Target)).ToString())) { continue; }
		FSoftClassPath Path(Entry.Source);
		if (!TestTrue(TEXT("Fix up source before native loading"), Path.FixupCoreRedirects())
			|| !TestEqual(TEXT("Fixed-up path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(TEXT("Fixed-up class path resolves correctly"), Path.TryLoadClass<UCommonUserWidget>() == Entry.Expected);
	}
	TestTrue(TEXT("TaggedWidget remains abstract"), UDCLyraTaggedWidget::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleTaggedWidgetDefaultsTest,
	"DreamCatcher.R5.RifleWidgetFoundation.TaggedWidgetDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleTaggedWidgetDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleWidgetFoundationTests;
	if (!CheckContext(*this)) { return false; }
	const UDCLyraTaggedWidget* Defaults = GetDefault<UDCLyraTaggedWidget>();
	if (!TestNotNull(TEXT("Abstract TaggedWidget native CDO"), Defaults)
		|| !TestTrue(TEXT("Only the CDO is inspected"), Defaults->HasAnyFlags(RF_ClassDefaultObject))) { return false; }
	const FStructProperty* HiddenTags = FindFProperty<FStructProperty>(Defaults->GetClass(), TEXT("HiddenByTags"));
	if (TestNotNull(TEXT("HiddenByTags is reflected"), HiddenTags)
		&& TestTrue(TEXT("HiddenByTags retains its gameplay-tag container type"), HiddenTags->Struct == FGameplayTagContainer::StaticStruct()))
	{
		TestTrue(TEXT("HiddenByTags remains editable and BlueprintReadOnly"), HiddenTags->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly));
		TestTrue(TEXT("Default hidden-tag list is empty"), HiddenTags->ContainerPtrToValuePtr<FGameplayTagContainer>(Defaults)->IsEmpty());
	}
	CheckVisibilityDefault(*this, Defaults, TEXT("ShownVisibility"), ESlateVisibility::Visible);
	CheckVisibilityDefault(*this, Defaults, TEXT("HiddenVisibility"), ESlateVisibility::Collapsed);
	AddInfo(TEXT("Reflected defaults only. The source tag-listener TODO and hard-coded false hidden-tag check remain unchanged; automatic tag hiding is not implemented or tested."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleWeaponWidgetContractTest,
	"DreamCatcher.R5.RifleWidgetFoundation.WeaponEventAndDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleWeaponWidgetContractTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleWidgetFoundationTests;
	if (!CheckContext(*this)) { return false; }
	UClass* WidgetClass = UDCLyraWeaponUserInterface::StaticClass();
	UClass* WeaponClass = UDCLyraWeaponInstance::StaticClass();
	const UDCLyraWeaponUserInterface* Defaults = GetDefault<UDCLyraWeaponUserInterface>();
	if (!TestNotNull(TEXT("Weapon UI native CDO"), Defaults)
		|| !TestTrue(TEXT("Only the weapon UI CDO is inspected"), Defaults->HasAnyFlags(RF_ClassDefaultObject))) { return false; }
	const FObjectPropertyBase* Current = FindFProperty<FObjectPropertyBase>(WidgetClass, TEXT("CurrentInstance"));
	if (TestNotNull(TEXT("CurrentInstance is reflected"), Current))
	{
		TestTrue(TEXT("CurrentInstance uses the existing original-derived weapon type"), Current->PropertyClass == WeaponClass);
		TestTrue(TEXT("CurrentInstance remains transient"), Current->HasAnyPropertyFlags(CPF_Transient));
		TestNull(TEXT("No default equipped weapon"), Current->GetObjectPropertyValue_InContainer(Defaults));
	}
	UFunction* Changed = WidgetClass->FindFunctionByName(TEXT("OnWeaponChanged"));
	if (!TestNotNull(TEXT("Original OnWeaponChanged event retained"), Changed)) { return false; }
	TestTrue(TEXT("Event is declared on the ported weapon UI class"), Changed->GetOuter() == WidgetClass);
	TestTrue(TEXT("Event remains Blueprint implementable"), Changed->HasAllFunctionFlags(FUNC_Event | FUNC_BlueprintEvent));
	TestFalse(TEXT("Event has not acquired a native implementation"), Changed->HasAnyFunctionFlags(FUNC_Native));
	TestNull(TEXT("Original event returns void"), Changed->GetReturnProperty());
	const TCHAR* Names[] = { TEXT("OldWeapon"), TEXT("NewWeapon") };
	const uint64 Qualifiers = static_cast<uint64>(CPF_Parm | CPF_ReturnParm | CPF_OutParm | CPF_ReferenceParm | CPF_ConstParm);
	int32 Index = 0;
	for (TFieldIterator<FProperty> It(Changed, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_Parm)) { continue; }
		if (!TestTrue(TEXT("No unexpected event parameters"), Index < static_cast<int32>(UE_ARRAY_COUNT(Names)))) { break; }
		TestEqual(FString::Printf(TEXT("Event parameter %d"), Index), It->GetName(), FString(Names[Index]));
		const FObjectPropertyBase* Parameter = CastField<FObjectPropertyBase>(*It);
		if (TestNotNull(TEXT("Event parameter is an object pointer"), Parameter))
		{
			TestTrue(TEXT("Event parameter retains the weapon instance type"), Parameter->PropertyClass == WeaponClass);
		}
		TestEqual(TEXT("Original input pointer qualifiers retained"), static_cast<uint64>(It->GetPropertyFlags()) & Qualifiers, static_cast<uint64>(CPF_Parm));
		++Index;
	}
	TestEqual(TEXT("Original event has exactly two parameters"), Index, 2);
	TestEqual(TEXT("UFunction parameter count agrees"), static_cast<int32>(Changed->NumParms), 2);
	AddInfo(TEXT("No event or Tick invoked. Valid-instigator filtering, equipment changes, missing-weapon/null notification behavior, empty Rebuild hook, widget display and networking remain runtime-unverified."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
