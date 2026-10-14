// Bounded native registration and ownerless ground-cache tests.
// No worlds, Pawns, meshes, AnimBPs, traces, animation updates, or asset saves.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Animation/AnimInstance.h"
#include "Animation/DCAnimInstance.h"
#include "Character/DCCharacterMovementComponent.h"
#include "CoreGlobals.h"
#include "Engine/HitResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DCRifleAnimationRegistrationTests
{
struct FNativeRedirect
{
	const TCHAR* Source;
	const TCHAR* Target;
	ECoreRedirectFlags Kind;
};

static const FNativeRedirect Redirects[] =
{
	{ TEXT("/Script/LyraGame.LyraAnimInstance"), TEXT("/Script/DreamCatcher.DCAnimInstance"), ECoreRedirectFlags::Type_Class },
	{ TEXT("/Script/LyraGame.LyraCharacterMovementComponent"), TEXT("/Script/DreamCatcher.DCCharacterMovementComponent"), ECoreRedirectFlags::Type_Class },
	{ TEXT("/Script/LyraGame.LyraCharacterGroundInfo"), TEXT("/Script/DreamCatcher.DCCharacterGroundInfo"), ECoreRedirectFlags::Type_Struct },
};

static_assert(UE_ARRAY_COUNT(Redirects) == 3, "Keep the approved animation name boundary.");

bool CheckContext(FAutomationTestBase& Test)
{
	return Test.TestTrue(TEXT("Animation registration tests require the DreamCatcher editor"),
		GIsEditor && !IsRunningCommandlet() && FString(FApp::GetProjectName()) == TEXT("DreamCatcher"));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleAnimationNativeTypesTest,
	"DreamCatcher.R5.AnimationRegistration.NativeTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleAnimationNativeTypesTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleAnimationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	for (const FNativeRedirect& Entry : Redirects)
	{
		UObject* Target = nullptr;
		if (Entry.Kind == ECoreRedirectFlags::Type_Class)
		{
			UClass* TargetClass = FindObject<UClass>(nullptr, Entry.Target);
			if (!TestNotNull(FString::Printf(TEXT("Native class already registered: %s"), Entry.Target), TargetClass)) { continue; }
			TestTrue(TEXT("Target is a native class"), TargetClass->HasAnyClassFlags(CLASS_Native));
			Target = TargetClass;
		}
		else
		{
			UScriptStruct* TargetStruct = FindObject<UScriptStruct>(nullptr, Entry.Target);
			if (!TestNotNull(TEXT("Ground-info script struct already registered"), TargetStruct)) { continue; }
			TestTrue(TEXT("Target is the existing native ground-info struct"), TargetStruct == FDCCharacterGroundInfo::StaticStruct());
			Target = TargetStruct;
		}

		const FCoreRedirectObjectName Actual = FCoreRedirects::GetRedirectedName(Entry.Kind, FCoreRedirectObjectName(FString(Entry.Source)));
		const FCoreRedirectObjectName Expected(FString(Entry.Target));
		if (!TestEqual(FString::Printf(TEXT("Exact redirect: %s"), Entry.Source), Actual.ToString(), Expected.ToString())) { continue; }
		FSoftObjectPath Path(Entry.Source);
		if (!TestTrue(TEXT("Fix up the source path before any load"), Path.FixupCoreRedirects())) { continue; }
		if (!TestEqual(TEXT("Fixed-up path is the approved destination"), Path.ToString(), FString(Entry.Target))) { continue; }
		// Only load a fixed-up /Script destination that was already found above.
		TestTrue(FString::Printf(TEXT("Native soft path resolves: %s"), Entry.Source), Path.TryLoad() == Target);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleAnimationReflectedContractTest,
	"DreamCatcher.R5.AnimationRegistration.ReflectedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleAnimationReflectedContractTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleAnimationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	UClass* AnimClass = UDCAnimInstance::StaticClass();
	UClass* MovementClass = UDCCharacterMovementComponent::StaticClass();
	TestTrue(TEXT("AnimInstance retains engine animation ancestry"), AnimClass->IsChildOf(UAnimInstance::StaticClass()));
	TestTrue(TEXT("Movement retains engine character movement ancestry"), MovementClass->IsChildOf(UCharacterMovementComponent::StaticClass()));

	const FFloatProperty* AnimGroundDistance = FindFProperty<FFloatProperty>(AnimClass, TEXT("GroundDistance"));
	if (TestNotNull(TEXT("AnimInstance exposes float GroundDistance"), AnimGroundDistance))
	{
		TestTrue(TEXT("Animation GroundDistance remains BlueprintReadOnly"), AnimGroundDistance->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
		// Read the native CDO only; do not initialize/update animation or bind an ASC.
		const UDCAnimInstance* AnimDefaults = GetDefault<UDCAnimInstance>();
		if (TestNotNull(TEXT("Native animation defaults are available"), AnimDefaults))
		{
			TestEqual(TEXT("Animation ground distance starts uninitialized"), AnimGroundDistance->GetPropertyValue_InContainer(AnimDefaults), -1.0f);
		}
	}
	const FStructProperty* TagMap = FindFProperty<FStructProperty>(AnimClass, TEXT("GameplayTagPropertyMap"));
	if (TestNotNull(TEXT("Animation tag-property map is reflected"), TagMap))
	{
		TestTrue(TEXT("Tag-property map retains its GAS struct type"), TagMap->Struct == FGameplayTagBlueprintPropertyMap::StaticStruct());
		TestTrue(TEXT("Tag-property map remains defaults-only editable"), TagMap->HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance));
	}

	UScriptStruct* GroundStruct = FDCCharacterGroundInfo::StaticStruct();
	const FStructProperty* GroundHit = FindFProperty<FStructProperty>(GroundStruct, TEXT("GroundHitResult"));
	if (TestNotNull(TEXT("Ground info exposes a hit-result struct"), GroundHit))
	{
		TestTrue(TEXT("Ground hit retains the engine FHitResult type"), GroundHit->Struct == FHitResult::StaticStruct());
		TestTrue(TEXT("Ground hit remains BlueprintReadOnly"), GroundHit->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
	}
	const FFloatProperty* InfoGroundDistance = FindFProperty<FFloatProperty>(GroundStruct, TEXT("GroundDistance"));
	if (TestNotNull(TEXT("Ground info exposes float GroundDistance"), InfoGroundDistance))
	{
		TestTrue(TEXT("Ground-info distance remains BlueprintReadOnly"), InfoGroundDistance->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCRifleAnimationOwnerlessGroundCacheTest,
	"DreamCatcher.R5.AnimationRegistration.OwnerlessGroundCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCRifleAnimationOwnerlessGroundCacheTest::RunTest(const FString& Parameters)
{
	using namespace DCRifleAnimationRegistrationTests;
	if (!CheckContext(*this)) { return false; }
	const FDCCharacterGroundInfo Defaults;
	TestEqual(TEXT("Ground-info default update frame"), Defaults.LastUpdateFrame, uint64(0));
	TestEqual(TEXT("Ground-info default distance"), Defaults.GroundDistance, 0.0f);

	TStrongObjectPtr<UDCCharacterMovementComponent> Movement(
		NewObject<UDCCharacterMovementComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Transient movement fixture created"), Movement.Get())) { return false; }
	if (!TestTrue(TEXT("Fixture is ownerless, worldless and unregistered"),
		Movement->GetOwner() == nullptr && Movement->GetWorld() == nullptr && !Movement->IsRegistered())) { return false; }
	TestTrue(TEXT("Fixture is transient and belongs to the transient package"),
		Movement->HasAnyFlags(RF_Transient) && Movement->GetOutermost() == GetTransientPackage());

	// The original !CharacterOwner guard returns the cache without tracing or using a world.
	const FDCCharacterGroundInfo& First = Movement->GetGroundInfo();
	TestEqual(TEXT("Ownerless lookup leaves the update frame unchanged"), First.LastUpdateFrame, Defaults.LastUpdateFrame);
	TestEqual(TEXT("Ownerless lookup leaves the distance unchanged"), First.GroundDistance, Defaults.GroundDistance);
	const FDCCharacterGroundInfo& Second = Movement->GetGroundInfo();
	TestTrue(TEXT("Repeated ownerless lookup returns the same cache"), &First == &Second);
	TestEqual(TEXT("Repeated lookup still does not update the frame"), Second.LastUpdateFrame, Defaults.LastUpdateFrame);
	TestEqual(TEXT("Repeated lookup preserves the distance"), Second.GroundDistance, Defaults.GroundDistance);
	TestFalse(TEXT("Reading the cache did not register the component"), Movement->IsRegistered());
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
