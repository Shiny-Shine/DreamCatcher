// Isolated native indicator/async contracts, not viewport or asset-loading tests.
// No Slate canvas, runtime widget, world, controller, saved asset, or production
// configuration is created/modified. Only owned transient objects/scopes are used.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AsyncMixin.h"
#include "Blueprint/UserWidget.h"
#include "Components/ControllerComponent.h"
#include "Components/SceneComponent.h"
#include "Components/Widget.h"
#include "CoreGlobals.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "SceneView.h"
#include "UI/IndicatorSystem/Lyra/DCLyraIndicatorDescriptor.h"
#include "UI/IndicatorSystem/Lyra/DCLyraIndicatorLayer.h"
#include "UI/IndicatorSystem/Lyra/DCLyraIndicatorLibrary.h"
#include "UI/IndicatorSystem/Lyra/DCLyraIndicatorManagerComponent.h"
#include "UI/IndicatorSystem/Lyra/IDCLyraActorIndicatorWidget.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCIndicatorFoundationTests
{
bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Use a separate unattended DreamCatcher Editor with -DCR5IndicatorIsolatedAutomation, not a commandlet or runtime-subsystem diagnostic session"),
		GIsEditor && !IsRunningCommandlet() && IsInGameThread() && FApp::IsUnattended()
		&& FString(FApp::GetProjectName()) == TEXT("DreamCatcher")
		&& FParse::Param(FCommandLine::Get(), TEXT("DCR5IndicatorIsolatedAutomation"))
		&& !FParse::Param(FCommandLine::Get(), TEXT("DCLyraRuntimeDiagnostics")));
}

void CheckInterfaceEvent(FAutomationTestBase& Test, const FName Name, bool bConst)
{
	UClass* InterfaceClass = UDCLyraIndicatorWidgetInterface::StaticClass();
	UFunction* Event = InterfaceClass->FindFunctionByName(Name);
	if (!Test.TestNotNull(Name.ToString() + TEXT(" is registered"), Event)) { return; }
	Test.TestTrue(TEXT("Event belongs to the ported interface"), Event->GetOuter() == InterfaceClass);
	Test.TestTrue(TEXT("Bind/unbind remains BlueprintNativeEvent"), Event->HasAllFunctionFlags(FUNC_Native | FUNC_Event | FUNC_BlueprintEvent));
	Test.TestNull(TEXT("Interface event returns void"), Event->GetReturnProperty());
	int32 Parameters = 0;
	for (TFieldIterator<FProperty> It(Event, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_Parm)) { continue; }
		++Parameters;
		Test.TestEqual(TEXT("Original interface argument name"), It->GetName(), FString(TEXT("Indicator")));
		const FObjectPropertyBase* Argument = CastField<FObjectPropertyBase>(*It);
		if (Test.TestNotNull(TEXT("Interface argument is an object pointer"), Argument))
		{
			Test.TestTrue(TEXT("Interface uses the original-derived descriptor"), Argument->PropertyClass == UDCLyraIndicatorDescriptor::StaticClass());
		}
		Test.TestFalse(TEXT("Argument is an input pointer, not output/reference/return"), It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm | CPF_ReturnParm));
		const bool bReflectedConst = It->HasAnyPropertyFlags(CPF_ConstParm) || It->HasMetaData(TEXT("NativeConst"));
		Test.TestEqual(TEXT("Original bind/unbind const contract"), bReflectedConst, bConst);
	}
	Test.TestEqual(TEXT("Exactly one descriptor argument"), Parameters, 1);
}

struct FScopedIndicatorListeners
{
	UDCLyraIndicatorManagerComponent* Manager;
	FDelegateHandle Added;
	FDelegateHandle Removed;

	explicit FScopedIndicatorListeners(UDCLyraIndicatorManagerComponent* InManager) : Manager(InManager) {}
	~FScopedIndicatorListeners()
	{
		Manager->OnIndicatorAdded.Remove(Added);
		Manager->OnIndicatorRemoved.Remove(Removed);
	}
};

struct FAsyncObservations
{
	TArray<int32> Order;
	int32 CanceledCalls = 0;
	int32 DestroyedCalls = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCIndicatorRegistrationTest,
	"DreamCatcher.R5.IndicatorFoundation.RegistrationAndInterface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCIndicatorRegistrationTest::RunTest(const FString& Parameters)
{
	using namespace DCIndicatorFoundationTests;
	if (!CheckContext(*this)) { return false; }
	TestTrue(TEXT("Original AsyncMixin runtime module is loaded"), FModuleManager::Get().IsModuleLoaded(TEXT("AsyncMixin")));
	struct FRedirect
	{
		const TCHAR* Source;
		const TCHAR* Target;
		ECoreRedirectFlags Kind;
		UObject* Expected;
	};
	const FRedirect Redirects[] =
	{
		{ TEXT("/Script/LyraGame.IndicatorLayer"), TEXT("/Script/DreamCatcher.DCLyraIndicatorLayer"), ECoreRedirectFlags::Type_Class, UDCLyraIndicatorLayer::StaticClass() },
		{ TEXT("/Script/LyraGame.IndicatorDescriptor"), TEXT("/Script/DreamCatcher.DCLyraIndicatorDescriptor"), ECoreRedirectFlags::Type_Class, UDCLyraIndicatorDescriptor::StaticClass() },
		{ TEXT("/Script/LyraGame.LyraIndicatorManagerComponent"), TEXT("/Script/DreamCatcher.DCLyraIndicatorManagerComponent"), ECoreRedirectFlags::Type_Class, UDCLyraIndicatorManagerComponent::StaticClass() },
		{ TEXT("/Script/LyraGame.IndicatorLibrary"), TEXT("/Script/DreamCatcher.DCLyraIndicatorLibrary"), ECoreRedirectFlags::Type_Class, UDCLyraIndicatorLibrary::StaticClass() },
		{ TEXT("/Script/LyraGame.IndicatorWidgetInterface"), TEXT("/Script/DreamCatcher.DCLyraIndicatorWidgetInterface"), ECoreRedirectFlags::Type_Class, UDCLyraIndicatorWidgetInterface::StaticClass() },
		{ TEXT("/Script/LyraGame.EActorCanvasProjectionMode"), TEXT("/Script/DreamCatcher.EDCLyraActorCanvasProjectionMode"), ECoreRedirectFlags::Type_Enum, StaticEnum<EDCLyraActorCanvasProjectionMode>() },
	};
	static_assert(UE_ARRAY_COUNT(Redirects) == 6, "Keep the approved indicator name boundary.");
	for (const FRedirect& Entry : Redirects)
	{
		if (!TestNotNull(TEXT("Expected native type"), Entry.Expected)
			|| !TestTrue(TEXT("Exact native target is registered"), FindObject<UObject>(nullptr, Entry.Target) == Entry.Expected)) { continue; }
		const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(Entry.Kind, FCoreRedirectObjectName(FString(Entry.Source)));
		if (!TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), FCoreRedirectObjectName(FString(Entry.Target)).ToString())) { continue; }
		FSoftObjectPath Path(Entry.Source);
		if (!TestTrue(TEXT("Fix up source before native load"), Path.FixupCoreRedirects())
			|| !TestEqual(TEXT("Fixed-up path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		TestTrue(TEXT("Native path resolves to the expected type"), Path.TryLoad() == Entry.Expected);
	}
	TestTrue(TEXT("Layer retains UWidget parent"), UDCLyraIndicatorLayer::StaticClass()->GetSuperClass() == UWidget::StaticClass());
	TestTrue(TEXT("Descriptor retains UObject parent"), UDCLyraIndicatorDescriptor::StaticClass()->GetSuperClass() == UObject::StaticClass());
	TestTrue(TEXT("Manager retains UControllerComponent parent"), UDCLyraIndicatorManagerComponent::StaticClass()->GetSuperClass() == UControllerComponent::StaticClass());
	TestTrue(TEXT("Library retains BlueprintFunctionLibrary parent"), UDCLyraIndicatorLibrary::StaticClass()->GetSuperClass() == UBlueprintFunctionLibrary::StaticClass());
	TestTrue(TEXT("Widget interface retains its interface flag"), UDCLyraIndicatorWidgetInterface::StaticClass()->HasAnyClassFlags(CLASS_Interface));
	const UEnum* Modes = StaticEnum<EDCLyraActorCanvasProjectionMode>();
	if (!TestNotNull(TEXT("Projection enum is registered"), Modes)) { return false; }
	const TCHAR* ModeNames[] = { TEXT("ComponentPoint"), TEXT("ComponentBoundingBox"), TEXT("ComponentScreenBoundingBox"), TEXT("ActorBoundingBox"), TEXT("ActorScreenBoundingBox"), TEXT("EDCLyraActorCanvasProjectionMode_MAX") };
	TestEqual(TEXT("Five source projection modes plus sentinel"), Modes->NumEnums(), static_cast<int32>(UE_ARRAY_COUNT(ModeNames)));
	for (int32 Index = 0; Index < Modes->NumEnums() && Index < static_cast<int32>(UE_ARRAY_COUNT(ModeNames)); ++Index)
	{
		TestEqual(TEXT("Projection mode name"), Modes->GetNameStringByIndex(Index), FString(ModeNames[Index]));
		TestEqual(TEXT("Projection mode value"), Modes->GetValueByIndex(Index), static_cast<int64>(Index));
	}
	CheckInterfaceEvent(*this, TEXT("BindIndicator"), false);
	CheckInterfaceEvent(*this, TEXT("UnbindIndicator"), true);
	const UDCLyraIndicatorLayer* LayerDefaults = GetDefault<UDCLyraIndicatorLayer>();
	if (TestNotNull(TEXT("Native layer CDO"), LayerDefaults))
	{
		TestTrue(TEXT("Original layer visibility is HitTestInvisible"), LayerDefaults->GetVisibility() == ESlateVisibility::HitTestInvisible);
	}
	TestNull(TEXT("Manager lookup accepts a null controller"), UDCLyraIndicatorLibrary::GetIndicatorManagerComponent(nullptr));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCIndicatorDescriptorTest,
	"DreamCatcher.R5.IndicatorFoundation.DescriptorDefaultsAndGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCIndicatorDescriptorTest::RunTest(const FString& Parameters)
{
	using namespace DCIndicatorFoundationTests;
	if (!CheckContext(*this)) { return false; }
	TStrongObjectPtr<UDCLyraIndicatorDescriptor> Descriptor(NewObject<UDCLyraIndicatorDescriptor>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Transient descriptor"), Descriptor.Get())) { return false; }
	TestNull(TEXT("No default data object"), Descriptor->GetDataObject());
	TestNull(TEXT("No default component"), Descriptor->GetSceneComponent());
	TestNull(TEXT("No default manager"), Descriptor->GetIndicatorManagerComponent());
	TestTrue(TEXT("Default widget class is empty"), Descriptor->GetIndicatorClass().IsNull());
	TestTrue(TEXT("Default socket is None"), Descriptor->GetComponentSocketName().IsNone());
	TestTrue(TEXT("Default projection is ComponentPoint"), Descriptor->GetProjectionMode() == EDCLyraActorCanvasProjectionMode::ComponentPoint);
	TestTrue(TEXT("Default alignment is centered"), Descriptor->GetHAlign() == HAlign_Center && Descriptor->GetVAlign() == VAlign_Center);
	TestEqual(TEXT("Default priority"), Descriptor->GetPriority(), 0);
	TestEqual(TEXT("Default box anchor"), Descriptor->GetBoundingBoxAnchor(), FVector(0.5, 0.5, 0.5));
	TestEqual(TEXT("Default world offset"), Descriptor->GetWorldPositionOffset(), FVector::ZeroVector);
	TestEqual(TEXT("Default screen offset"), Descriptor->GetScreenSpaceOffset(), FVector2D::ZeroVector);
	TestFalse(TEXT("Default clamping"), Descriptor->GetClampToScreen());
	TestFalse(TEXT("Default clamp arrow"), Descriptor->GetShowClampToScreenArrow());
	TestFalse(TEXT("Default auto removal"), Descriptor->GetAutoRemoveWhenIndicatorComponentIsNull());
	TestFalse(TEXT("No component means not visible"), Descriptor->GetIsVisible());
	TestFalse(TEXT("No implicit removal without opt-in"), Descriptor->CanAutomaticallyRemove());
	FSceneViewProjectionData UnusedView;
	FVector ScreenPosition(7, 8, 9);
	FDCLyraIndicatorProjection Projector;
	TestFalse(TEXT("Projection refuses a missing component"), Projector.Project(*Descriptor.Get(), UnusedView, FVector2f(1280, 720), ScreenPosition));
	TestEqual(TEXT("Failed projection leaves the caller output unchanged"), ScreenPosition, FVector(7, 8, 9));

	TStrongObjectPtr<USceneComponent> Component(NewObject<USceneComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Ownerless transient scene component"), Component.Get())) { return false; }
	if (!TestTrue(TEXT("Component is unregistered/worldless"), !Component->IsRegistered() && Component->GetWorld() == nullptr)) { return false; }
	Descriptor->SetAutoRemoveWhenIndicatorComponentIsNull(true);
	TestTrue(TEXT("Explicit opt-in allows removal with no component"), Descriptor->CanAutomaticallyRemove());
	Descriptor->SetSceneComponent(Component.Get());
	Descriptor->SetDataObject(Component.Get());
	TestTrue(TEXT("Public data reference round-trip"), Descriptor->GetDataObject() == Component.Get());
	TestTrue(TEXT("Component reference round-trip"), Descriptor->GetSceneComponent() == Component.Get());
	TestTrue(TEXT("Valid component satisfies visibility guard"), Descriptor->GetIsVisible());
	TestFalse(TEXT("Valid component blocks automatic removal"), Descriptor->CanAutomaticallyRemove());
	Descriptor->SetDesiredVisibility(false);
	TestFalse(TEXT("Explicit desired visibility is respected"), Descriptor->GetIsVisible());
	Descriptor->SetComponentSocketName(TEXT("IndicatorFixtureSocket"));
	Descriptor->SetProjectionMode(EDCLyraActorCanvasProjectionMode::ActorBoundingBox);
	Descriptor->SetHAlign(HAlign_Right);
	Descriptor->SetVAlign(VAlign_Bottom);
	Descriptor->SetClampToScreen(true);
	Descriptor->SetShowClampToScreenArrow(true);
	Descriptor->SetPriority(7);
	Descriptor->SetWorldPositionOffset(FVector(1, 2, 3));
	Descriptor->SetScreenSpaceOffset(FVector2D(4, 5));
	Descriptor->SetBoundingBoxAnchor(FVector(0.25, 0.75, 0.5));
	Descriptor->SetIndicatorClass(TSoftClassPtr<UUserWidget>(UUserWidget::StaticClass()));
	TestEqual(TEXT("Socket setter"), Descriptor->GetComponentSocketName(), FName(TEXT("IndicatorFixtureSocket")));
	TestTrue(TEXT("Projection/alignment setters"), Descriptor->GetProjectionMode() == EDCLyraActorCanvasProjectionMode::ActorBoundingBox && Descriptor->GetHAlign() == HAlign_Right && Descriptor->GetVAlign() == VAlign_Bottom);
	TestTrue(TEXT("Clamp flags setters"), Descriptor->GetClampToScreen() && Descriptor->GetShowClampToScreenArrow());
	TestEqual(TEXT("Priority setter"), Descriptor->GetPriority(), 7);
	TestEqual(TEXT("World offset setter"), Descriptor->GetWorldPositionOffset(), FVector(1, 2, 3));
	TestEqual(TEXT("Screen offset setter"), Descriptor->GetScreenSpaceOffset(), FVector2D(4, 5));
	TestEqual(TEXT("Box anchor setter"), Descriptor->GetBoundingBoxAnchor(), FVector(0.25, 0.75, 0.5));
	TestTrue(TEXT("Soft class is a reference only, not a created widget"), Descriptor->GetIndicatorClass().Get() == UUserWidget::StaticClass());
	Descriptor->SetSceneComponent(nullptr);
	TestTrue(TEXT("Clearing the component restores opted-in removal"), Descriptor->CanAutomaticallyRemove());
	AddInfo(TEXT("Synthetic transient data only. Projection with a real component/camera/viewport, bounding boxes and on-screen positions are unverified."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCIndicatorManagerTest,
	"DreamCatcher.R5.IndicatorFoundation.ManagerLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCIndicatorManagerTest::RunTest(const FString& Parameters)
{
	using namespace DCIndicatorFoundationTests;
	if (!CheckContext(*this)) { return false; }
	TStrongObjectPtr<UDCLyraIndicatorManagerComponent> Manager(NewObject<UDCLyraIndicatorManagerComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCLyraIndicatorDescriptor> Descriptor(NewObject<UDCLyraIndicatorDescriptor>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Transient manager"), Manager.Get()) || !TestNotNull(TEXT("Transient descriptor"), Descriptor.Get())) { return false; }
	if (!TestTrue(TEXT("Manager has no controller/world and is not registered"), Manager->GetOwner() == nullptr && Manager->GetWorld() == nullptr && !Manager->IsRegistered())) { return false; }
	TestEqual(TEXT("Manager starts empty"), Manager->GetIndicators().Num(), 0);
	int32 AddedCalls = 0;
	int32 RemovedCalls = 0;
	FScopedIndicatorListeners Listeners(Manager.Get());
	Listeners.Added = Manager->OnIndicatorAdded.AddLambda([this, &AddedCalls, ManagerPtr = Manager.Get(), DescriptorPtr = Descriptor.Get()](UDCLyraIndicatorDescriptor* Item)
	{
		++AddedCalls;
		TestTrue(TEXT("Added event descriptor identity"), Item == DescriptorPtr);
		TestTrue(TEXT("Manager assigned before added event"), Item->GetIndicatorManagerComponent() == ManagerPtr);
		TestEqual(TEXT("Added event precedes array insertion"), ManagerPtr->GetIndicators().Num(), 0);
	});
	Listeners.Removed = Manager->OnIndicatorRemoved.AddLambda([this, &RemovedCalls, ManagerPtr = Manager.Get(), DescriptorPtr = Descriptor.Get()](UDCLyraIndicatorDescriptor* Item)
	{
		++RemovedCalls;
		TestTrue(TEXT("Removed event descriptor identity"), Item == DescriptorPtr);
		TestTrue(TEXT("Removed event precedes array removal"), ManagerPtr->GetIndicators().Contains(Item));
	});
	Manager->AddIndicator(Descriptor.Get());
	TestEqual(TEXT("One added event"), AddedCalls, 1);
	TestEqual(TEXT("One descriptor after addition"), Manager->GetIndicators().Num(), 1);
	TestTrue(TEXT("Manager contains the same descriptor"), Manager->GetIndicators().Contains(Descriptor.Get()));
	Descriptor->UnregisterIndicator();
	TestEqual(TEXT("Unregister routes one removal event"), RemovedCalls, 1);
	TestEqual(TEXT("Unregister empties the array"), Manager->GetIndicators().Num(), 0);
	TestTrue(TEXT("Original removal leaves the descriptor manager pointer intact"), Descriptor->GetIndicatorManagerComponent() == Manager.Get());
	Manager->RemoveIndicator(nullptr);
	TestEqual(TEXT("Null removal is a no-op"), RemovedCalls, 1);
	TestFalse(TEXT("No component registration was introduced"), Manager->IsRegistered());
	AddInfo(TEXT("Fresh descriptor, one registration only. No same-descriptor re-registration, controller attachment, widget listeners, rendering, or replication is tested."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCIndicatorAsyncTest,
	"DreamCatcher.R5.IndicatorFoundation.AsyncSequenceAndCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCIndicatorAsyncTest::RunTest(const FString& Parameters)
{
	using namespace DCIndicatorFoundationTests;
	if (!CheckContext(*this)) { return false; }
	TSharedRef<FAsyncObservations> Observed = MakeShared<FAsyncObservations>();
	{
		FAsyncScope Sequence;
		TestFalse(TEXT("New scope has no loading in progress"), Sequence.IsAsyncLoadingInProgress());
		Sequence.AsyncEvent(FSimpleDelegate::CreateLambda([Observed]() { Observed->Order.Add(1); }));
		Sequence.AsyncEvent(FSimpleDelegate::CreateLambda([Observed]() { Observed->Order.Add(2); }));
		TestEqual(TEXT("Queued events wait for start"), Observed->Order.Num(), 0);
		Sequence.StartAsyncLoading();
		TestTrue(TEXT("Asset-free events execute in queue order"), Observed->Order == TArray<int32>({1, 2}));
		TestFalse(TEXT("Started event sequence is complete"), Sequence.IsAsyncLoadingInProgress());
	}
	{
		FAsyncScope Canceled;
		Canceled.AsyncEvent(FSimpleDelegate::CreateLambda([Observed]() { ++Observed->CanceledCalls; }));
		Canceled.CancelAsyncLoading();
		Canceled.StartAsyncLoading();
		TestEqual(TEXT("Cancel discards the queued callback"), Observed->CanceledCalls, 0);
	}
	{
		FAsyncScope Destroyed;
		Destroyed.AsyncEvent(FSimpleDelegate::CreateLambda([Observed]() { ++Observed->DestroyedCalls; }));
		// Destructor must cancel its own pending auto-start, without touching other scopes.
	}
	// Let the isolated Editor advance normally. Never call the global ticker's Tick
	// or clear process-wide state/delegates. Shared observations survive this RunTest.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Observed]()
	{
		TestTrue(TEXT("Completed callbacks do not run twice on later frames"), Observed->Order == TArray<int32>({1, 2}));
		TestEqual(TEXT("Canceled callback stays canceled after normal frame progress"), Observed->CanceledCalls, 0);
		TestEqual(TEXT("Destroyed scope cannot auto-start its callback"), Observed->DestroyedCalls, 0);
	}, 0.05f));
	AddInfo(TEXT("AsyncEvent only; no AsyncLoad, bundles, asset IO, Slate canvas, WidgetPool, or viewport was exercised."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
