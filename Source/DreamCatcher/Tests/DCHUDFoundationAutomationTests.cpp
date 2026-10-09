// Isolated tests for the original-derived HUD receiver and AddWidgets action.
// No saved assets, production HUD selection, LocalPlayers, widgets, viewport, or PIE.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AssetRegistry/AssetBundleData.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Components/GameFrameworkComponentManager.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFeatures/DCGameFeatureAction_AddWidget.h"
#include "GameFeatures/GameFeatureAction_WorldActionBase.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "UI/Lyra/DCLyraHUD.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCHUDFoundationTests
{
bool CheckContext(FAutomationTestBase& Test, bool bNeedsWorld = false)
{
	if (!Test.TestTrue(TEXT("HUD foundation tests require the DreamCatcher editor"),
		GIsEditor && !IsRunningCommandlet() && IsInGameThread() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher")))
	{
		return false;
	}
	return !bNeedsWorld || Test.TestTrue(TEXT("Lifecycle tests require a separate unattended process with -DCR5HUDIsolatedAutomation and no runtime-subsystem diagnostic opt-in"),
		FApp::IsUnattended() && FParse::Param(FCommandLine::Get(), TEXT("DCR5HUDIsolatedAutomation"))
		&& !FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")));
}

bool StartWorld(FAutomationTestBase& Test, FTestWorldWrapper& Fixture)
{
	if (!CheckContext(Test, true)) { return false; }
	if (!Fixture.CreateTestWorld(EWorldType::Game))
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	UWorld* World = Fixture.GetTestWorld();
	UGameInstance* Instance = World->GetGameInstance();
	if (!Test.TestNotNull(TEXT("Fixture GameInstance exists"), Instance)
		|| !Test.TestTrue(TEXT("Fixture GameInstance resolves this world"), Instance->GetWorld() == World)
		|| !Test.TestEqual(TEXT("Fixture has no LocalPlayers"), Instance->GetNumLocalPlayers(), 0)
		|| !Test.TestNull(TEXT("Fixture has no viewport"), Instance->GetGameViewportClient())) { return false; }
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	if (!Fixture.BeginPlayInTestWorld())
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	return true;
}

ADCLyraHUD* SpawnHUD(UWorld* World)
{
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ADCLyraHUD>(Params);
}

template <typename TEntry>
TArray<TEntry>* FindEntries(FAutomationTestBase& Test, UDCGameFeatureAction_AddWidgets& Action, const FName Name)
{
	if (!Test.TestTrue(TEXT("Only a transient non-CDO action may be edited"),
		Action.HasAnyFlags(RF_Transient) && !Action.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))) { return nullptr; }
	FArrayProperty* Array = FindFProperty<FArrayProperty>(Action.GetClass(), Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is a reflected array"), Array)) { return nullptr; }
	FStructProperty* Entry = CastField<FStructProperty>(Array->Inner);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" has struct entries"), Entry)
		|| !Test.TestTrue(Name.ToString() + TEXT(" retains its original entry type"), Entry->Struct == TEntry::StaticStruct())) { return nullptr; }
	return Array->ContainerPtrToValuePtr<TArray<TEntry>>(&Action);
}

// Owned action cleanup precedes fixture/world teardown on every early return.
struct FScopedActionActivation
{
	UDCGameFeatureAction_AddWidgets* Action;
	FName WorldHandle;
	bool bActive = false;

	FScopedActionActivation(UDCGameFeatureAction_AddWidgets* InAction, FName InWorldHandle)
		: Action(InAction), WorldHandle(InWorldHandle) {}
	~FScopedActionActivation() { if (bActive) { Deactivate(); } }

	void Activate()
	{
		check(!bActive);
		FGameFeatureActivatingContext Context;
		Context.SetRequiredWorldContextHandle(WorldHandle);
		bActive = true;
		Action->OnGameFeatureActivating(Context);
	}

	int32 Deactivate()
	{
		check(bActive);
		FGameFeatureDeactivatingContext Context(TEXT("DCHUDFoundationFixture"), [](FStringView) {});
		Context.SetRequiredWorldContextHandle(WorldHandle);
		Action->OnGameFeatureDeactivating(Context);
		bActive = false;
		return Context.GetNumPausers();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDRegistrationTest,
	"DreamCatcher.R5.HUDFoundation.RegistrationAndDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDRegistrationTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDFoundationTests;
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
		{ TEXT("/Script/LyraGame.LyraHUD"), TEXT("/Script/DreamCatcher.DCLyraHUD"), ECoreRedirectFlags::Type_Class, ADCLyraHUD::StaticClass() },
		{ TEXT("/Script/LyraGame.GameFeatureAction_WorldActionBase"), TEXT("/Script/DreamCatcher.GameFeatureAction_WorldActionBase"), ECoreRedirectFlags::Type_Class, UGameFeatureAction_WorldActionBase::StaticClass() },
		{ TEXT("/Script/LyraGame.GameFeatureAction_AddWidgets"), TEXT("/Script/DreamCatcher.DCGameFeatureAction_AddWidgets"), ECoreRedirectFlags::Type_Class, UDCGameFeatureAction_AddWidgets::StaticClass() },
		{ TEXT("/Script/LyraGame.LyraHUDLayoutRequest"), TEXT("/Script/DreamCatcher.DCLyraHUDLayoutRequest"), ECoreRedirectFlags::Type_Struct, FDCLyraHUDLayoutRequest::StaticStruct() },
		{ TEXT("/Script/LyraGame.LyraHUDElementEntry"), TEXT("/Script/DreamCatcher.DCLyraHUDElementEntry"), ECoreRedirectFlags::Type_Struct, FDCLyraHUDElementEntry::StaticStruct() },
	};
	static_assert(UE_ARRAY_COUNT(Redirects) == 5, "Keep the approved HUD redirect boundary.");
	for (const FRedirect& Entry : Redirects)
	{
		if (!TestTrue(TEXT("Native target is already registered"), FindObject<UObject>(nullptr, Entry.Target) == Entry.Expected)) { continue; }
		const FCoreRedirectObjectName Resolved = FCoreRedirects::GetRedirectedName(Entry.Kind, FCoreRedirectObjectName(FString(Entry.Source)));
		if (!TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Resolved.ToString(), FCoreRedirectObjectName(FString(Entry.Target)).ToString())) { continue; }
		FSoftObjectPath Path(Entry.Source);
		if (!TestTrue(TEXT("Fix up source path before native loading"), Path.FixupCoreRedirects())
			|| !TestEqual(TEXT("Fixed-up path is the approved target"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(TEXT("Fixed-up native path resolves to the expected type"), Path.TryLoad() == Entry.Expected);
	}
	TestTrue(TEXT("Original HUD derives from the engine HUD"), ADCLyraHUD::StaticClass()->IsChildOf(AHUD::StaticClass()));
	TestFalse(TEXT("Original HUD does not start with ticking enabled"), GetDefault<ADCLyraHUD>()->PrimaryActorTick.bStartWithTickEnabled);
	TestTrue(TEXT("AddWidgets reuses the existing world action base"), UDCGameFeatureAction_AddWidgets::StaticClass()->GetSuperClass() == UGameFeatureAction_WorldActionBase::StaticClass());
	TestTrue(TEXT("World action base remains abstract"), UGameFeatureAction_WorldActionBase::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	const FDCLyraHUDLayoutRequest Layout;
	const FDCLyraHUDElementEntry Widget;
	TestTrue(TEXT("Default layout has no class or layer"), Layout.LayoutClass.IsNull() && !Layout.LayerID.IsValid());
	TestTrue(TEXT("Default element has no class or slot"), Widget.WidgetClass.IsNull() && !Widget.SlotID.IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDActionDataTest,
	"DreamCatcher.R5.HUDFoundation.ActionDataAndBundles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDActionDataTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDFoundationTests;
	if (!CheckContext(*this)) { return false; }
	TStrongObjectPtr<UDCGameFeatureAction_AddWidgets> Action(NewObject<UDCGameFeatureAction_AddWidgets>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Transient action created"), Action.Get())) { return false; }
	TArray<FDCLyraHUDLayoutRequest>* Layouts = FindEntries<FDCLyraHUDLayoutRequest>(*this, *Action.Get(), TEXT("Layout"));
	TArray<FDCLyraHUDElementEntry>* Widgets = FindEntries<FDCLyraHUDElementEntry>(*this, *Action.Get(), TEXT("Widgets"));
	if (!Layouts || !Widgets) { return false; }
	TestEqual(TEXT("No default layout"), Layouts->Num(), 0);
	TestEqual(TEXT("No default widget"), Widgets->Num(), 0);
	FDataValidationContext Empty;
	TestTrue(TEXT("Original empty action is valid"), Action->IsDataValid(Empty) == EDataValidationResult::Valid);
	TestEqual(TEXT("Empty action has no validation errors"), Empty.GetNumErrors(), uint32(0));
	Layouts->AddDefaulted();
	Widgets->AddDefaulted();
	FDataValidationContext Missing;
	TestTrue(TEXT("Missing classes and tags are invalid"), Action->IsDataValid(Missing) == EDataValidationResult::Invalid);
	TestEqual(TEXT("Original validation reports four missing fields"), Missing.GetNumErrors(), uint32(4));

	// Test only non-null class / non-empty tag validation, not existence of a runtime UI layer.
	// These native classes are never instantiated; no synthetic Blueprint or widget is saved.
	const FGameplayTag ExistingTag = FGameplayTag::RequestGameplayTag(TEXT("HUD.Slot.LeftSideTouchInputs"), false);
	if (!TestTrue(TEXT("Existing test tag is registered without adding tags"), ExistingTag.IsValid())) { return false; }
	(*Layouts)[0].LayoutClass = UCommonActivatableWidget::StaticClass();
	(*Layouts)[0].LayerID = ExistingTag;
	(*Widgets)[0].WidgetClass = UUserWidget::StaticClass();
	(*Widgets)[0].SlotID = ExistingTag;
	FDataValidationContext Valid;
	TestTrue(TEXT("Original populated action validation passes"), Action->IsDataValid(Valid) == EDataValidationResult::Valid);
	TestEqual(TEXT("Populated action has no validation errors"), Valid.GetNumErrors(), uint32(0));
#if WITH_EDITORONLY_DATA
	FAssetBundleData Bundles;
	Action->AddAdditionalAssetBundleData(Bundles);
	const FAssetBundleEntry* Client = Bundles.FindEntry(UGameFeaturesSubsystemSettings::LoadStateClient);
	if (TestNotNull(TEXT("Action adds a client bundle"), Client))
	{
		TestTrue(TEXT("Client bundle contains the element class"), Client->AssetPaths.Contains((*Widgets)[0].WidgetClass.ToSoftObjectPath().GetAssetPath()));
	}
	TestNull(TEXT("Action does not request a server bundle"), Bundles.FindEntry(UGameFeaturesSubsystemSettings::LoadStateServer));
	// This direct hook covers Widgets. Layout's AssetBundles metadata is checked separately;
	// loading a real Experience/Client bundle is outside this fixture.
	const FProperty* LayoutClass = FindFProperty<FProperty>(FDCLyraHUDLayoutRequest::StaticStruct(), TEXT("LayoutClass"));
	const FProperty* WidgetClass = FindFProperty<FProperty>(FDCLyraHUDElementEntry::StaticStruct(), TEXT("WidgetClass"));
	if (TestNotNull(TEXT("Layout soft class is reflected"), LayoutClass)) { TestEqual(TEXT("Layout retains Client bundle metadata"), LayoutClass->GetMetaData(TEXT("AssetBundles")), FString(TEXT("Client"))); }
	if (TestNotNull(TEXT("Widget soft class is reflected"), WidgetClass)) { TestEqual(TEXT("Widget retains Client bundle metadata"), WidgetClass->GetMetaData(TEXT("AssetBundles")), FString(TEXT("Client"))); }
	Layouts->Reset();
	Widgets->Reset();
	FAssetBundleData EmptyBundles;
	Action->AddAdditionalAssetBundleData(EmptyBundles);
	TestEqual(TEXT("Empty action adds no bundle entries"), EmptyBundles.Bundles.Num(), 0);
#endif
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDReceiverLifecycleTest,
	"DreamCatcher.R5.HUDFoundation.HUDReceiverLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDReceiverLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDFoundationTests;
	FTestWorldWrapper Fixture;
	if (!StartWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	UGameFrameworkComponentManager* Manager = World->GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>();
	if (!TestNotNull(TEXT("Fixture component manager"), Manager)) { return false; }
	TArray<FName> Events;
	TSharedPtr<FComponentRequestHandle> Observer = Manager->AddExtensionHandler(TSoftClassPtr<AActor>(ADCLyraHUD::StaticClass()),
		UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateLambda([World, &Events](AActor* Actor, FName Event)
		{
			if (Actor && Actor->GetWorld() == World) { Events.Add(Event); }
		}));
	if (!TestTrue(TEXT("Owned event observer registered"), Observer.IsValid())) { return false; }
	ADCLyraHUD* HUD = SpawnHUD(World);
	if (!TestNotNull(TEXT("Isolated HUD spawned"), HUD)) { return false; }
	TestNull(TEXT("HUD has no owning controller or UI context"), HUD->GetOwningPlayerController());
	TestTrue(TEXT("HUD entered BeginPlay"), HUD->HasActorBegunPlay());
	TestFalse(TEXT("HUD starts without ticking"), HUD->IsActorTickEnabled());
	const int32 Added = Events.Find(UGameFrameworkComponentManager::NAME_ReceiverAdded);
	const int32 Ready = Events.Find(UGameFrameworkComponentManager::NAME_GameActorReady);
	TestTrue(TEXT("ReceiverAdded precedes GameActorReady"), Added != INDEX_NONE && Ready > Added);
	TestEqual(TEXT("Exactly one receiver registration"), Events.FilterByPredicate([](FName Event) { return Event == UGameFrameworkComponentManager::NAME_ReceiverAdded; }).Num(), 1);
	TestEqual(TEXT("Exactly one ready event"), Events.FilterByPredicate([](FName Event) { return Event == UGameFrameworkComponentManager::NAME_GameActorReady; }).Num(), 1);
	TestTrue(TEXT("Destroy isolated HUD"), HUD->Destroy());
	TestEqual(TEXT("EndPlay removes the receiver once"), Events.FilterByPredicate([](FName Event) { return Event == UGameFrameworkComponentManager::NAME_ReceiverRemoved; }).Num(), 1);
	TestTrue(TEXT("Receiver removal follows ready"), Events.Find(UGameFrameworkComponentManager::NAME_ReceiverRemoved) > Ready);
	Observer.Reset();
	Fixture.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHUDEmptyActionLifecycleTest,
	"DreamCatcher.R5.HUDFoundation.EmptyActionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHUDEmptyActionLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace DCHUDFoundationTests;
	FTestWorldWrapper Fixture;
	if (!StartWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World);
	if (!TestNotNull(TEXT("Fixture context exists"), WorldContext)
		|| !TestFalse(TEXT("Fixture context can be restricted by its handle"), WorldContext->ContextHandle.IsNone())) { return false; }
	TStrongObjectPtr<UDCGameFeatureAction_AddWidgets> Action(NewObject<UDCGameFeatureAction_AddWidgets>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Empty transient action created"), Action.Get())) { return false; }
	FScopedActionActivation Activation(Action.Get(), WorldContext->ContextHandle);
	TestFalse(TEXT("New action has no world-start subscription"), FWorldDelegates::OnStartGameInstance.IsBoundToObject(Action.Get()));
	for (int32 Cycle = 0; Cycle < 2; ++Cycle)
	{
		Activation.Activate();
		TestTrue(TEXT("World action subscribes on activation"), FWorldDelegates::OnStartGameInstance.IsBoundToObject(Action.Get()));
		ADCLyraHUD* HUD = SpawnHUD(World);
		if (!TestNotNull(TEXT("HUD for empty action lifecycle"), HUD)) { return false; }
		TestNull(TEXT("No player-owned widgets can be created"), HUD->GetOwningPlayerController());
		UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(HUD, UGameFrameworkComponentManager::NAME_GameActorReady);
		if (Cycle == 1) { TestTrue(TEXT("Second cycle removes HUD before action deactivation"), HUD->Destroy()); }
		TestEqual(TEXT("Empty action deactivates without async pausers"), Activation.Deactivate(), 0);
		TestFalse(TEXT("Deactivation removes the action's world-start subscription"), FWorldDelegates::OnStartGameInstance.IsBoundToObject(Action.Get()));
		if (Cycle == 0) { TestTrue(TEXT("First cycle removes HUD after action deactivation"), HUD->Destroy()); }
	}
	TestEqual(TEXT("Still no LocalPlayers after lifecycle checks"), World->GetGameInstance()->GetNumLocalPlayers(), 0);
	Fixture.ForwardErrorMessages(this);
	AddInfo(TEXT("Empty native action hooks only; no GameFeature plugin activation, actual widgets, UI rendering, PIE, or replication was tested."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
