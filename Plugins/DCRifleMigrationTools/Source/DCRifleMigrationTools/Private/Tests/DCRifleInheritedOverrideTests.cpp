// Transient Blueprint fixture: no asset loads, disk saves, native Advanced Copy, or world spawn.
#include "DCRifleInheritedOverrides.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SkeletalMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleInheritedOverrideTest,
	"DreamCatcher.MigrationTools.ExplicitCopy.InheritedOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCRifleInheritedOverrideTest::RunTest(const FString& Parameters)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCRifleCopyDiagnostics")))
	{
		AddError(TEXT("Run this transient fixture in a separate unattended process with -DCRifleCopyDiagnostics."));
		return false;
	}
	const FString Root = TEXT("/Temp/DCRifleOverrideTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const auto MakeBlueprint = [&Root](const FString& Side, const TCHAR* Name, UClass* Parent)
	{
		UPackage* Package = CreatePackage(*(Root / Side / Name));
		// Transient is a UObject flag, not an EPackageFlags value in UE 5.8.
		Package->SetFlags(RF_Transient);
		return FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(Name), BPTYPE_Normal);
	};
	TStrongObjectPtr<UBlueprint> SourceParent(MakeBlueprint(TEXT("Source"), TEXT("BP_Parent"), AActor::StaticClass()));
	TStrongObjectPtr<UBlueprint> TargetParent(MakeBlueprint(TEXT("Target"), TEXT("BP_Parent"), AActor::StaticClass()));
	if (!TestNotNull(TEXT("Source parent"), SourceParent.Get()) || !TestNotNull(TEXT("Target parent"), TargetParent.Get()))
	{
		return false;
	}
	USCS_Node* SourceNode = SourceParent->SimpleConstructionScript->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("WeaponMesh"));
	USCS_Node* TargetNode = TargetParent->SimpleConstructionScript->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("WeaponMesh"));
	SourceParent->SimpleConstructionScript->AddNode(SourceNode);
	TargetParent->SimpleConstructionScript->AddNode(TargetNode);
	TargetNode->VariableGuid = SourceNode->VariableGuid;
	FKismetEditorUtilities::CompileBlueprint(SourceParent.Get());
	FKismetEditorUtilities::CompileBlueprint(TargetParent.Get());
	TStrongObjectPtr<UBlueprint> SourceChild(MakeBlueprint(TEXT("Source"), TEXT("BP_Child"), SourceParent->GeneratedClass));
	TStrongObjectPtr<UBlueprint> TargetChild(MakeBlueprint(TEXT("Target"), TEXT("BP_Child"), TargetParent->GeneratedClass));
	if (!TestNotNull(TEXT("Source child"), SourceChild.Get()) || !TestNotNull(TEXT("Target child"), TargetChild.Get()))
	{
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(SourceChild.Get());
	FKismetEditorUtilities::CompileBlueprint(TargetChild.Get());
	UInheritableComponentHandler* SourceHandler = SourceChild->GetInheritableComponentHandler(true);
	if (!TestNotNull(TEXT("Source handler"), SourceHandler)) { return false; }
	USkeletalMeshComponent* Original = Cast<USkeletalMeshComponent>(SourceHandler->CreateOverridenComponentTemplate(FComponentKey(SourceNode)));
	if (!TestNotNull(TEXT("Source override"), Original)) { return false; }
	Original->ComponentTags.Add(TEXT("KeepOriginalOverride"));
	Original->SetRelativeLocation(FVector(17.0, 25.0, 31.0));
	Original->SetVisibility(false);
	Original->SetCastShadow(false);
	FKismetEditorUtilities::CompileBlueprint(SourceChild.Get());

	TMap<FString, FString> Map;
	Map.Add(SourceParent->GetOutermost()->GetName(), TargetParent->GetOutermost()->GetName());
	Map.Add(SourceChild->GetOutermost()->GetName(), TargetChild->GetOutermost()->GetName());
	DCRifleMigration::FInheritedOverrideState State;
	TArray<FString> Messages;
	if (!TestTrue(TEXT("Capture source override"), State.Capture(Map, Messages)))
	{
		for (const FString& Message : Messages) { AddInfo(Message); }
		return false;
	}
	TestEqual(TEXT("One child override captured"), State.Num(), 1);
	int32 Compared = 0;
	TestFalse(TEXT("Missing target override is detected"), State.Compare(Map, Messages, Compared));
	Messages.Reset();
	if (!TestTrue(TEXT("Restore values and compile"), State.RestoreAndCompile(Map, Messages)))
	{
		for (const FString& Message : Messages) { AddInfo(Message); }
		return false;
	}
	TestTrue(TEXT("Post-compile properties match original"), State.Compare(Map, Messages, Compared));
	TestEqual(TEXT("One override compared"), Compared, 1);
	UInheritableComponentHandler* TargetHandler = TargetChild->GetInheritableComponentHandler(false);
	UActorComponent* Repaired = TargetHandler ? TargetHandler->GetOverridenComponentTemplate(FComponentKey(TargetNode)) : nullptr;
	if (!TestNotNull(TEXT("Repaired override exists"), Repaired)) { return false; }
	TestTrue(TEXT("Original component tag preserved"), Repaired->ComponentTags.Contains(TEXT("KeepOriginalOverride")));
	USkeletalMeshComponent* RepairedMesh = Cast<USkeletalMeshComponent>(Repaired);
	if (!TestNotNull(TEXT("Component type preserved"), RepairedMesh)) { return false; }
	TestTrue(TEXT("Transform preserved"), RepairedMesh->GetRelativeLocation().Equals(FVector(17.0, 25.0, 31.0)));
	Repaired->ComponentTags.Add(TEXT("DeliberateMismatch"));
	TestFalse(TEXT("Later property corruption is detected"), State.Compare(Map, Messages, Compared));
	UInheritableComponentHandler* OriginalHandler = SourceChild->GetInheritableComponentHandler(false);
	UActorComponent* OriginalAfter = OriginalHandler ? OriginalHandler->GetOverridenComponentTemplate(FComponentKey(SourceNode)) : nullptr;
	if (TestNotNull(TEXT("Source override still exists"), OriginalAfter))
	{
		TestFalse(TEXT("Changing copy did not change source"), OriginalAfter->ComponentTags.Contains(TEXT("DeliberateMismatch")));
	}
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
