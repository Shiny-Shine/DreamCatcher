// Native/CDO-only tests for the original-derived HUD widget foundations.
// Never instantiate the abstract widgets or initialize/activate them; no CDO writes,
// player/world creation, viewport, device events, user-selector UI, or asset saves.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "CoreGlobals.h"
#include "Engine/EngineBaseTypes.h"
#include "GameplayTagContainer.h"
#include "Input/UIActionBindingHandle.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UI/Foundation/DCLyraControllerDisconnectedScreen.h"
#include "UI/Lyra/DCLyraActivatableWidget.h"
#include "UI/Lyra/DCLyraHUDLayout.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/EnumProperty.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCHUDLayoutFoundationTests
{
bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("HUD layout tests require the DreamCatcher editor on the game thread"),
		GIsEditor && !IsRunningCommandlet() && IsInGameThread() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher"));
}

bool CheckCDO(FAutomationTestBase& Test, const UObject* Defaults)
{
	return Test.TestNotNull(TEXT("Native CDO is available"), Defaults)
		&& Test.TestTrue(TEXT("Only a class default object is inspected"), Defaults->HasAnyFlags(RF_ClassDefaultObject));
}

void CheckEnumDefault(FAutomationTestBase& Test, const UObject* Defaults, const FName Name, UEnum* ExpectedEnum, uint64 ExpectedValue)
{
	if (!CheckCDO(Test, Defaults)) { return; }
	const FEnumProperty* Property = FindFProperty<FEnumProperty>(Defaults->GetClass(), Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is a reflected enum property"), Property)) { return; }
	Test.TestTrue(Name.ToString() + TEXT(" uses the original enum type"), Property->GetEnum() == ExpectedEnum);
	Test.TestTrue(Name.ToString() + TEXT(" remains defaults-only editable"), Property->HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance));
	const void* Value = Property->ContainerPtrToValuePtr<void>(Defaults);
	Test.TestEqual(Name.ToString() + TEXT(" retains its constructor default"), Property->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(Value), ExpectedValue);
}

void CheckTagDefault(FAutomationTestBase& Test, const UObject* Defaults, const FName Name, const TCHAR* ExpectedTagName)
{
	if (!CheckCDO(Test, Defaults)) { return; }
	const FStructProperty* Property = FindFProperty<FStructProperty>(Defaults->GetClass(), Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is reflected"), Property)
		|| !Test.TestTrue(Name.ToString() + TEXT(" is a gameplay tag container"), Property->Struct == FGameplayTagContainer::StaticStruct())) { return; }
	const FGameplayTagContainer* Tags = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Defaults);
	const FGameplayTag Expected = FGameplayTag::RequestGameplayTag(ExpectedTagName, false);
	Test.TestTrue(TEXT("Original platform tag is registered without editing Config"), Expected.IsValid());
	Test.TestEqual(Name.ToString() + TEXT(" contains one default platform trait"), Tags->Num(), 1);
	Test.TestTrue(Name.ToString() + TEXT(" retains the exact source spelling"), Tags->HasTagExact(Expected));
}

void CheckObjectDefault(FAutomationTestBase& Test, const UObject* Defaults, const FName Name, UClass* ExpectedClass, bool bBindWidget)
{
	if (!CheckCDO(Test, Defaults)) { return; }
	const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Defaults->GetClass(), Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is an object property"), Property)) { return; }
	Test.TestTrue(Name.ToString() + TEXT(" retains its widget type"), Property->PropertyClass == ExpectedClass);
	Test.TestNull(Name.ToString() + TEXT(" has no default widget instance"), Property->GetObjectPropertyValue_InContainer(Defaults));
	if (bBindWidget)
	{
		Test.TestTrue(Name.ToString() + TEXT(" retains the required BindWidget metadata"), Property->HasMetaData(TEXT("BindWidget")));
		Test.TestFalse(Name.ToString() + TEXT(" was not made optional"), Property->HasMetaData(TEXT("BindWidgetOptional")));
	}
	else
	{
		Test.TestTrue(Name.ToString() + TEXT(" remains transient"), Property->HasAnyPropertyFlags(CPF_Transient));
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDLayoutRegistrationTest,
	"DreamCatcher.R5.HUDLayoutFoundation.RegistrationAndEnum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDLayoutRegistrationTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDLayoutFoundationTests;
	if (!CheckContext(*this)) { return false; }
	struct FRedirect
	{
		const TCHAR* Source;
		const TCHAR* Target;
		ECoreRedirectFlags Kind;
		UObject* Expected;
	};
	const FRedirect Redirects[] =
	{
		{ TEXT("/Script/LyraGame.LyraActivatableWidget"), TEXT("/Script/DreamCatcher.DCLyraActivatableWidget"), ECoreRedirectFlags::Type_Class, UDCLyraActivatableWidget::StaticClass() },
		{ TEXT("/Script/LyraGame.LyraHUDLayout"), TEXT("/Script/DreamCatcher.DCLyraHUDLayout"), ECoreRedirectFlags::Type_Class, UDCLyraHUDLayout::StaticClass() },
		{ TEXT("/Script/LyraGame.LyraControllerDisconnectedScreen"), TEXT("/Script/DreamCatcher.DCLyraControllerDisconnectedScreen"), ECoreRedirectFlags::Type_Class, UDCLyraControllerDisconnectedScreen::StaticClass() },
		{ TEXT("/Script/LyraGame.ELyraWidgetInputMode"), TEXT("/Script/DreamCatcher.EDCLyraWidgetInputMode"), ECoreRedirectFlags::Type_Enum, StaticEnum<EDCLyraWidgetInputMode>() },
	};
	static_assert(UE_ARRAY_COUNT(Redirects) == 4, "Keep the approved HUD layout redirect boundary.");
	for (const FRedirect& Entry : Redirects)
	{
		if (!TestNotNull(TEXT("Expected native type"), Entry.Expected)
			|| !TestTrue(TEXT("Target is already registered"), FindObject<UObject>(nullptr, Entry.Target) == Entry.Expected)) { continue; }
		const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(Entry.Kind, FCoreRedirectObjectName(FString(Entry.Source)));
		if (!TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), FCoreRedirectObjectName(FString(Entry.Target)).ToString())) { continue; }
		FSoftObjectPath Path(Entry.Source);
		if (!TestTrue(TEXT("Fix up the source before native loading"), Path.FixupCoreRedirects())
			|| !TestEqual(TEXT("Fixed-up path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(TEXT("Native path resolves to the expected type"), Path.TryLoad() == Entry.Expected);
		if (const UClass* Class = Cast<UClass>(Entry.Expected))
		{
			TestTrue(TEXT("Widget foundation stays native and abstract"), Class->HasAllClassFlags(CLASS_Native | CLASS_Abstract));
		}
	}
	TestTrue(TEXT("Activatable widget retains its CommonUI parent"), UDCLyraActivatableWidget::StaticClass()->GetSuperClass() == UCommonActivatableWidget::StaticClass());
	TestTrue(TEXT("HUD layout derives from the ported input-policy widget"), UDCLyraHUDLayout::StaticClass()->GetSuperClass() == UDCLyraActivatableWidget::StaticClass());
	TestTrue(TEXT("Disconnected screen retains its original CommonUI parent"), UDCLyraControllerDisconnectedScreen::StaticClass()->GetSuperClass() == UCommonActivatableWidget::StaticClass());
	const UEnum* InputMode = StaticEnum<EDCLyraWidgetInputMode>();
	if (!TestNotNull(TEXT("Input-mode enum registered"), InputMode)) { return false; }
	const TCHAR* Names[] = { TEXT("Default"), TEXT("GameAndMenu"), TEXT("Game"), TEXT("Menu"), TEXT("EDCLyraWidgetInputMode_MAX") };
	TestEqual(TEXT("Four source enum members plus generated sentinel"), InputMode->NumEnums(), static_cast<int32>(UE_ARRAY_COUNT(Names)));
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Names)) && Index < InputMode->NumEnums(); ++Index)
	{
		TestEqual(TEXT("Input-mode enum member name"), InputMode->GetNameStringByIndex(Index), FString(Names[Index]));
		TestEqual(TEXT("Input-mode enum numeric value"), InputMode->GetValueByIndex(Index), static_cast<int64>(Index));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDLayoutDefaultsTest,
	"DreamCatcher.R5.HUDLayoutFoundation.ReflectedDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDLayoutDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDLayoutFoundationTests;
	if (!CheckContext(*this)) { return false; }
	// Reading abstract class CDOs is intentional; never create ordinary abstract instances.
	const UDCLyraActivatableWidget* Activatable = GetDefault<UDCLyraActivatableWidget>();
	const UDCLyraHUDLayout* Layout = GetDefault<UDCLyraHUDLayout>();
	const UDCLyraControllerDisconnectedScreen* Disconnect = GetDefault<UDCLyraControllerDisconnectedScreen>();
	if (!CheckCDO(*this, Activatable) || !CheckCDO(*this, Layout) || !CheckCDO(*this, Disconnect)) { return false; }
	CheckEnumDefault(*this, Activatable, TEXT("InputConfig"), StaticEnum<EDCLyraWidgetInputMode>(), static_cast<uint64>(EDCLyraWidgetInputMode::Default));
	CheckEnumDefault(*this, Activatable, TEXT("GameMouseCaptureMode"), StaticEnum<EMouseCaptureMode>(), static_cast<uint64>(EMouseCaptureMode::CapturePermanently));
	CheckEnumDefault(*this, Layout, TEXT("InputConfig"), StaticEnum<EDCLyraWidgetInputMode>(), static_cast<uint64>(EDCLyraWidgetInputMode::Default));
	CheckEnumDefault(*this, Layout, TEXT("GameMouseCaptureMode"), StaticEnum<EMouseCaptureMode>(), static_cast<uint64>(EMouseCaptureMode::CapturePermanently));

	const FSoftClassProperty* Escape = FindFProperty<FSoftClassProperty>(Layout->GetClass(), TEXT("EscapeMenuClass"));
	if (TestNotNull(TEXT("Escape menu is a reflected soft class"), Escape)
		&& TestTrue(TEXT("Escape menu retains its CommonUI class constraint"), Escape->MetaClass == UCommonActivatableWidget::StaticClass()))
	{
		TestTrue(TEXT("Escape menu is not assigned by this code port"), Escape->ContainerPtrToValuePtr<TSoftClassPtr<UCommonActivatableWidget>>(Layout)->IsNull());
	}
	const FClassProperty* Screen = FindFProperty<FClassProperty>(Layout->GetClass(), TEXT("ControllerDisconnectedScreen"));
	if (TestNotNull(TEXT("Disconnect screen is a reflected class"), Screen))
	{
		TestTrue(TEXT("Disconnect class points to the ported screen base"), Screen->MetaClass == UDCLyraControllerDisconnectedScreen::StaticClass());
		TestNull(TEXT("Disconnect screen class is not assigned by the native default"), Screen->GetObjectPropertyValue_InContainer(Layout));
	}
	CheckObjectDefault(*this, Layout, TEXT("SpawnedControllerDisconnectScreen"), UCommonActivatableWidget::StaticClass(), false);
	CheckObjectDefault(*this, Disconnect, TEXT("HBox_SwitchUser"), UHorizontalBox::StaticClass(), true);
	CheckObjectDefault(*this, Disconnect, TEXT("Button_ChangeUser"), UCommonButtonBase::StaticClass(), true);
	CheckTagDefault(*this, Layout, TEXT("PlatformRequiresControllerDisconnectScreen"), TEXT("Platform.Trait.Input.PrimarlyController"));
	CheckTagDefault(*this, Disconnect, TEXT("PlatformSupportsUserChangeTags"), TEXT("Platform.Trait.Input.HasStrictControllerPairing"));
	TestTrue(TEXT("Menu layer tag retained, without asserting a live UI layer"), FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Menu"), false).IsValid());
	TestTrue(TEXT("Escape tag retained, without asserting a key mapping"), FGameplayTag::RequestGameplayTag(TEXT("UI.Action.Escape"), false).IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDLayoutDefaultInputTest,
	"DreamCatcher.R5.HUDLayoutFoundation.DefaultInputPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDLayoutDefaultInputTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDLayoutFoundationTests;
	if (!CheckContext(*this)) { return false; }
	const UDCLyraActivatableWidget* Activatable = GetDefault<UDCLyraActivatableWidget>();
	const UDCLyraHUDLayout* Layout = GetDefault<UDCLyraHUDLayout>();
	if (!CheckCDO(*this, Activatable) || !CheckCDO(*this, Layout)) { return false; }
	// This original const getter only computes a value. Never activate a widget,
	// call NativeOnInitialized, or mutate a CDO to exercise another input mode.
	TestFalse(TEXT("Default policy leaves input routing unspecified"), Activatable->GetDesiredInputConfig().IsSet());
	TestFalse(TEXT("HUD layout inherits the unspecified default policy"), Layout->GetDesiredInputConfig().IsSet());
	TestFalse(TEXT("Repeated policy read stays unspecified"), Layout->GetDesiredInputConfig().IsSet());
	CheckEnumDefault(*this, Activatable, TEXT("InputConfig"), StaticEnum<EDCLyraWidgetInputMode>(), static_cast<uint64>(EDCLyraWidgetInputMode::Default));
	CheckEnumDefault(*this, Layout, TEXT("InputConfig"), StaticEnum<EDCLyraWidgetInputMode>(), static_cast<uint64>(EDCLyraWidgetInputMode::Default));
	AddInfo(TEXT("Default CDO policy only. Live input-mode switching, focus validation, Escape binding, menus, device callbacks, platform user selection, Blueprint binding, PIE and replication remain unverified."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
