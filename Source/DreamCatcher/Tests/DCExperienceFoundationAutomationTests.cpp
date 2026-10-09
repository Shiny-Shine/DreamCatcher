// DreamCatcher-only transient tests for the source-derived Experience foundations.
// No production Experience, plugin activation, world, saved asset, or settings changes.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AssetRegistry/AssetBundleData.h"
#include "Character/DCPawnData.h"
#include "Components/SceneComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "GameFeatureAction_AddComponents.h"
#include "GameFeaturesSubsystemSettings.h"
#include "GameFramework/Actor.h"
#include "GameModes/Lyra/DCLyraExperienceActionSet.h"
#include "GameModes/Lyra/DCLyraExperienceDefinition.h"
#include "GameModes/Lyra/DCLyraExperienceManager.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace DCExperienceFoundationTests
{
template <typename T>
void CheckActionValidation(FAutomationTestBase& Test, T& Asset, const FString& Label)
{
	FDataValidationContext EmptyContext;
	Test.TestTrue(Label + TEXT(" empty original data is valid"), Asset.IsDataValid(EmptyContext) == EDataValidationResult::Valid);
	Test.TestEqual(Label + TEXT(" empty data errors"), EmptyContext.GetNumErrors(), uint32(0));

	Asset.Actions.Add(nullptr);
	Asset.Actions.Add(nullptr);
	FDataValidationContext NullContext;
	Test.TestTrue(Label + TEXT(" null actions are invalid"), Asset.IsDataValid(NullContext) == EDataValidationResult::Invalid);
	Test.TestEqual(Label + TEXT(" each null action is reported"), NullContext.GetNumErrors(), uint32(2));
	Asset.Actions.Reset();

	// Reuse a concrete engine action instead of a test UClass; do not activate it.
	UGameFeatureAction_AddComponents* Action = NewObject<UGameFeatureAction_AddComponents>(&Asset, NAME_None, RF_Transient);
	Asset.Actions.Add(Action);
	FGameFeatureComponentEntry& Component = Action->ComponentList.AddDefaulted_GetRef();
	FDataValidationContext InvalidChildContext;
	Test.TestTrue(Label + TEXT(" delegates invalid child validation"), Asset.IsDataValid(InvalidChildContext) == EDataValidationResult::Invalid);
	Test.TestEqual(Label + TEXT(" missing actor and component errors propagate"), InvalidChildContext.GetNumErrors(), uint32(2));

	Component.ActorClass = AActor::StaticClass();
	Component.ComponentClass = USceneComponent::StaticClass();
	FDataValidationContext ValidChildContext;
	Test.TestTrue(Label + TEXT(" valid child action accepted"), Asset.IsDataValid(ValidChildContext) == EDataValidationResult::Valid);
	Test.TestEqual(Label + TEXT(" valid child errors"), ValidChildContext.GetNumErrors(), uint32(0));
	Asset.Actions.Reset();
}

#if WITH_EDITORONLY_DATA
template <typename T>
void CheckAssetBundles(FAutomationTestBase& Test, T& Asset, const FString& Label)
{
	// Inspect the inherited reflected bundle data without widening the production API.
	FStructProperty* BundleProperty = FindFProperty<FStructProperty>(Asset.GetClass(), TEXT("AssetBundleData"));
	if (!Test.TestNotNull(Label + TEXT(" reflected bundle property"), BundleProperty)
		|| !Test.TestTrue(Label + TEXT(" expected bundle struct"), BundleProperty->Struct == FAssetBundleData::StaticStruct()))
	{
		return;
	}
	FAssetBundleData* Bundles = BundleProperty->ContainerPtrToValuePtr<FAssetBundleData>(&Asset);
	UGameFeatureAction_AddComponents* Action = NewObject<UGameFeatureAction_AddComponents>(&Asset, NAME_None, RF_Transient);
	Asset.Actions.Add(nullptr); // Original bundle traversal skips null actions.
	Asset.Actions.Add(Action);
	FGameFeatureComponentEntry& Component = Action->ComponentList.AddDefaulted_GetRef();
	Component.ActorClass = AActor::StaticClass();
	Component.ComponentClass = USceneComponent::StaticClass();
	Component.bClientComponent = true;
	Component.bServerComponent = false;
	const FTopLevelAssetPath ComponentPath = Component.ComponentClass.ToSoftObjectPath().GetAssetPath();

	Asset.UpdateAssetBundleData();
	const FAssetBundleEntry* Client = Bundles->FindEntry(UGameFeaturesSubsystemSettings::LoadStateClient);
	Test.TestTrue(Label + TEXT(" client bundle includes the action component"), Client && Client->AssetPaths.Contains(ComponentPath));
	Test.TestNull(Label + TEXT(" server bundle absent when not requested"), Bundles->FindEntry(UGameFeaturesSubsystemSettings::LoadStateServer));

	Component.bClientComponent = false;
	Component.bServerComponent = true;
	Asset.UpdateAssetBundleData();
	const FAssetBundleEntry* Server = Bundles->FindEntry(UGameFeaturesSubsystemSettings::LoadStateServer);
	Test.TestTrue(Label + TEXT(" server bundle includes the action component"), Server && Server->AssetPaths.Contains(ComponentPath));
	Test.TestNull(Label + TEXT(" refreshing removes the previous client bundle"), Bundles->FindEntry(UGameFeaturesSubsystemSettings::LoadStateClient));

	Asset.Actions.Reset();
	Asset.UpdateAssetBundleData();
	Test.TestEqual(Label + TEXT(" removing actions clears bundle entries"), Bundles->Bundles.Num(), 0);
}
#endif // WITH_EDITORONLY_DATA
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCExperienceDataContractTest,
	"DreamCatcher.R5.ExperienceFoundation.DataContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCExperienceDataContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDCLyraExperienceDefinition> Experience(NewObject<UDCLyraExperienceDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCLyraExperienceActionSet> ActionSet(NewObject<UDCLyraExperienceActionSet>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCPawnData> PawnData(NewObject<UDCPawnData>(GetTransientPackage(), NAME_None, RF_Transient));

	TestEqual(TEXT("Experience starts without feature plugins"), Experience->GameFeaturesToEnable.Num(), 0);
	TestEqual(TEXT("Experience starts without actions"), Experience->Actions.Num(), 0);
	TestEqual(TEXT("Experience starts without action sets"), Experience->ActionSets.Num(), 0);
	TestNull(TEXT("Experience has no implicit PawnData"), Experience->DefaultPawnData.Get());
	TestEqual(TEXT("ActionSet starts without feature plugins"), ActionSet->GameFeaturesToEnable.Num(), 0);
	TestEqual(TEXT("ActionSet starts without actions"), ActionSet->Actions.Num(), 0);
	TestTrue(TEXT("Original Experience Const class flag retained"), Experience->GetClass()->HasAnyClassFlags(CLASS_Const));

	Experience->DefaultPawnData = PawnData.Get();
	Experience->ActionSets.Add(ActionSet.Get());
	TestTrue(TEXT("Previously ported PawnData can be referenced"), Experience->DefaultPawnData == PawnData.Get());
	TestTrue(TEXT("Composition references the ported ActionSet type"), Experience->ActionSets[0] == ActionSet.Get());

	// Class renaming also changes native instance PrimaryAssetType; scanning/real BP migration is separate.
	TestTrue(TEXT("Experience native instance uses its new class name as primary type"),
		Experience->GetPrimaryAssetId() == FPrimaryAssetId(FName(TEXT("DCLyraExperienceDefinition")), Experience->GetFName()));
	TestTrue(TEXT("ActionSet native instance uses its new class name as primary type"),
		ActionSet->GetPrimaryAssetId() == FPrimaryAssetId(FName(TEXT("DCLyraExperienceActionSet")), ActionSet->GetFName()));
	TestFalse(TEXT("Native Experience CDO is not itself a primary asset"), GetDefault<UDCLyraExperienceDefinition>()->GetPrimaryAssetId().IsValid());

	DCExperienceFoundationTests::CheckActionValidation(*this, *Experience, TEXT("Experience"));
	DCExperienceFoundationTests::CheckActionValidation(*this, *ActionSet, TEXT("ActionSet"));
	AddInfo(TEXT("Transient native data only. Blueprint inheritance validation, original asset imports and PrimaryAsset scanning are not exercised."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCExperienceAssetBundleTest,
	"DreamCatcher.R5.ExperienceFoundation.ActionBundles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCExperienceAssetBundleTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Editor AssetManager is initialized"), UAssetManager::IsInitialized()))
	{
		return false;
	}
	TStrongObjectPtr<UDCLyraExperienceDefinition> Experience(NewObject<UDCLyraExperienceDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCLyraExperienceActionSet> ActionSet(NewObject<UDCLyraExperienceActionSet>(GetTransientPackage(), NAME_None, RF_Transient));
#if WITH_EDITORONLY_DATA
	DCExperienceFoundationTests::CheckAssetBundles(*this, *Experience, TEXT("Experience"));
	DCExperienceFoundationTests::CheckAssetBundles(*this, *ActionSet, TEXT("ActionSet"));
#endif
	AddInfo(TEXT("Bundle collection only; no asset save, async Experience load, component spawning or GameFeature activation."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCExperiencePluginArbitrationTest,
	"DreamCatcher.R5.ExperienceFoundation.PluginRequestArbitration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCExperiencePluginArbitrationTest::RunTest(const FString& Parameters)
{
	// The source API uses the process EngineSubsystem. Never exercise it in the user's interactive editor.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCR5ExperienceIsolatedAutomation")) || !FApp::IsUnattended())
	{
		AddError(TEXT("Use a separate unattended UnrealEditor-Cmd process with -DCR5ExperienceIsolatedAutomation."));
		return false;
	}
	if (!TestTrue(TEXT("Editor arbitration branch is active"), GIsEditor)
		|| !TestNotNull(TEXT("Engine"), GEngine)
		|| !TestNotNull(TEXT("Source-based Experience engine subsystem"), GEngine->GetEngineSubsystem<UDCLyraExperienceManager>()))
	{
		return false;
	}

	// Synthetic keys are never passed to GameFeaturesSubsystem. Each request is balanced; no global reset.
	const FString Prefix = TEXT("DCExperienceFixture_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString PluginA = Prefix + TEXT("_A");
	const FString PluginB = Prefix + TEXT("_B");
	UDCLyraExperienceManager::NotifyOfPluginActivation(PluginA);
	UDCLyraExperienceManager::NotifyOfPluginActivation(PluginA);
	UDCLyraExperienceManager::NotifyOfPluginActivation(PluginA);
	UDCLyraExperienceManager::NotifyOfPluginActivation(PluginB);
	TestFalse(TEXT("First of three requesters must not deactivate A"), UDCLyraExperienceManager::RequestToDeactivatePlugin(PluginA));
	TestTrue(TEXT("Only requester may deactivate independent B"), UDCLyraExperienceManager::RequestToDeactivatePlugin(PluginB));
	TestFalse(TEXT("Second of three requesters must not deactivate A"), UDCLyraExperienceManager::RequestToDeactivatePlugin(PluginA));
	TestTrue(TEXT("Last requester may deactivate A"), UDCLyraExperienceManager::RequestToDeactivatePlugin(PluginA));
	UDCLyraExperienceManager::NotifyOfPluginActivation(PluginA);
	TestTrue(TEXT("A new balanced cycle starts at one request"), UDCLyraExperienceManager::RequestToDeactivatePlugin(PluginA));

	AddInfo(TEXT("Request-count decisions only; no actual plugin activation/deactivation, multi-PIE sessions, loading screen or runtime GameState."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
