#include "Tests/DCMergeReviewTestTypes.h"

#include "Weapon/Lyra/DCLyraRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCMergeReviewTestTypes)

void UDCReviewRangedWeaponFixture::TraceForTest(UDCLyraRangedWeaponInstance* Weapon,
	const FGameplayAbilityActorInfo* ActorInfo, const TArray<ECollisionChannel>& Channels, TArray<FHitResult>& OutHits)
{
	TraceChannels = Channels;
	NextTraceChannel = 0;
	SetCurrentActorInfo(FGameplayAbilitySpecHandle(), ActorInfo);
	FRangedWeaponFiringInput Input;
	Input.WeaponData = Weapon;
	Input.AimDir = FVector::ForwardVector;
	Input.EndAim = Input.AimDir * Weapon->GetMaxDamageRange();
	TraceBulletsInCartridge(Input, OutHits);
	SetCurrentActorInfo(FGameplayAbilitySpecHandle(), nullptr);
}

ECollisionChannel UDCReviewRangedWeaponFixture::DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const
{
	check(TraceChannels.IsValidIndex(NextTraceChannel));
	return TraceChannels[NextTraceChannel++];
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Character/DCPawnData.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/DCInputComponent.h"
#include "Input/DCInputConfig.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace DCMergeReviewTests
{
bool CreateWorld(FAutomationTestBase& Test, FTestWorldWrapper& Fixture)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCMergeReviewIsolatedAutomation")))
	{
		Test.AddError(TEXT("Run in a separate UnrealEditor-Cmd process with -DCMergeReviewIsolatedAutomation."));
		return false;
	}
	if (!Fixture.CreateTestWorld(EWorldType::Game))
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCReviewInputMappingTest,
	"DreamCatcher.IntegrationReview.InputMappingRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCReviewInputMappingTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCMergeReviewTests::CreateWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	APawn* Pawn = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Controller"), Controller) || !TestNotNull(TEXT("Pawn"), Pawn)) { return false; }
	Controller->Possess(Pawn);
	Controller->PlayerInput = NewObject<UEnhancedPlayerInput>(Controller, NAME_None, RF_Transient);
	TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient));
	// Initialize a separate local player's subsystems without loading the user's saved input profile.
	LocalPlayer->PlayerAdded(nullptr, PLATFORMUSERID_NONE);
	LocalPlayer->PlayerController = Controller;
	Controller->Player = LocalPlayer.Get();
	ON_SCOPE_EXIT
	{
		LocalPlayer->PlayerRemoved();
		LocalPlayer->PlayerController = nullptr;
		Controller->Player = nullptr;
	};
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!TestNotNull(TEXT("Enhanced Input subsystem"), Subsystem)) { return false; }
	FObjectPropertyBase* SettingsProperty = FindFProperty<FObjectPropertyBase>(Subsystem->GetClass(), TEXT("UserSettings"));
	if (!TestNotNull(TEXT("Transient settings property"), SettingsProperty)) { return false; }
	UEnhancedInputUserSettings* Settings = NewObject<UEnhancedInputUserSettings>(Subsystem, NAME_None, RF_Transient);
	Settings->Initialize(LocalPlayer.Get());
	SettingsProperty->SetObjectPropertyValue_InContainer(Subsystem, Settings);
	ON_SCOPE_EXIT { Subsystem->ClearAllMappings(); };

	UDCInputConfig* InputConfig = NewObject<UDCInputConfig>(Pawn, NAME_None, RF_Transient);
	UInputAction* AimAction = NewObject<UInputAction>(InputConfig, NAME_None, RF_Transient);
	FDCInputAction& Aim = InputConfig->NativeInputActions.AddDefaulted_GetRef();
	Aim.InputAction = AimAction;
	Aim.InputTag = DCGameplayTags::InputTag_Aim;
	UDCPawnData* Data = NewObject<UDCPawnData>(Pawn, NAME_None, RF_Transient);
	Data->InputConfig = InputConfig;
	UDCPawnExtensionComponent* Extension = NewObject<UDCPawnExtensionComponent>(Pawn, NAME_None, RF_Transient);
	Pawn->AddInstanceComponent(Extension);
	// Supply only the data dependency; normal Pawn/ASC initialization is outside this regression.
	FObjectPropertyBase* DataProperty = FindFProperty<FObjectPropertyBase>(Extension->GetClass(), TEXT("PawnData"));
	if (!TestNotNull(TEXT("Transient PawnData property"), DataProperty)) { return false; }
	DataProperty->SetObjectPropertyValue_InContainer(Extension, Data);
	UDCReviewHeroFixture* Hero = NewObject<UDCReviewHeroFixture>(Pawn, NAME_None, RF_Transient);
	UDCInputComponent* Input = NewObject<UDCInputComponent>(Pawn, NAME_None, RF_Transient);
	UInputMappingContext* Registered = NewObject<UInputMappingContext>(Pawn, NAME_None, RF_Transient);
	UInputMappingContext* Unregistered = NewObject<UInputMappingContext>(Pawn, NAME_None, RF_Transient);
	UInputMappingContext* Previous = NewObject<UInputMappingContext>(Pawn, NAME_None, RF_Transient);
	Registered->MapKey(AimAction, EKeys::RightMouseButton);
	Unregistered->MapKey(AimAction, EKeys::LeftShift);
	TArray<FInputMappingContextAndPriority> Mappings;
	FInputMappingContextAndPriority& RegisteredEntry = Mappings.AddDefaulted_GetRef();
	RegisteredEntry.InputMapping = Registered;
	RegisteredEntry.Priority = 2;
	RegisteredEntry.bRegisterWithSettings = true;
	FInputMappingContextAndPriority& UnregisteredEntry = Mappings.AddDefaulted_GetRef();
	UnregisteredEntry.InputMapping = Unregistered;
	UnregisteredEntry.Priority = 7;
	UnregisteredEntry.bRegisterWithSettings = false;
	Subsystem->AddMappingContext(Previous, 99);
	Hero->InitializeForTest(Input, Mappings);
	int32 Priority = INDEX_NONE;
	TestTrue(TEXT("Unregistered context is active after Hero clears the previous mappings"), Subsystem->HasMappingContext(Unregistered, Priority));
	TestEqual(TEXT("Unregistered priority is retained"), Priority, 7);
	TestTrue(TEXT("Registered context remains active"), Subsystem->HasMappingContext(Registered, Priority));
	TestEqual(TEXT("Registered priority is retained"), Priority, 2);
	TestFalse(TEXT("Previous context was cleared by real initialization"), Subsystem->HasMappingContext(Previous));
	TestTrue(TEXT("Opted-in context is registered with settings"), Settings->IsMappingContextRegistered(Registered));
	TestFalse(TEXT("Opted-out context is not registered with settings"), Settings->IsMappingContextRegistered(Unregistered));
	TestTrue(TEXT("Hero finished input binding"), Hero->IsReadyToBindInputs());

	// Settings may be unavailable: both kinds of mapping must still be activated.
	SettingsProperty->SetObjectPropertyValue_InContainer(Subsystem, nullptr);
	Hero->InitializeForTest(Input, Mappings);
	TestTrue(TEXT("Registered context activates without user settings"), Subsystem->HasMappingContext(Registered));
	TestTrue(TEXT("Unregistered context activates without user settings"), Subsystem->HasMappingContext(Unregistered));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCReviewCartridgeMissTest,
	"DreamCatcher.IntegrationReview.PerBulletMissResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCReviewCartridgeMissTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Fixture;
	if (!DCMergeReviewTests::CreateWorld(*this, Fixture)) { return false; }
	UWorld* World = Fixture.GetTestWorld();
	APawn* Pawn = World->SpawnActor<APawn>();
	AActor* Target = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Firing Pawn"), Pawn) || !TestNotNull(TEXT("Trace target"), Target)) { return false; }
	UBoxComponent* Box = NewObject<UBoxComponent>(Target, NAME_None, RF_Transient);
	Target->AddInstanceComponent(Box);
	Target->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(20.0));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Box->RegisterComponent();
	Target->SetActorLocation(FVector(500.0, 0.0, 0.0));
	UDCAbilitySystemComponent* ASC = NewObject<UDCAbilitySystemComponent>(Pawn, NAME_None, RF_Transient);
	Pawn->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Pawn, Pawn);
	TStrongObjectPtr<UDCLyraRangedWeaponInstance> Weapon(NewObject<UDCLyraRangedWeaponInstance>(Pawn, NAME_None, RF_Transient));
	TStrongObjectPtr<UDCReviewRangedWeaponFixture> Ability(NewObject<UDCReviewRangedWeaponFixture>(Pawn, NAME_None, RF_Transient));
	FIntProperty* BulletCount = FindFProperty<FIntProperty>(Weapon->GetClass(), TEXT("BulletsPerCartridge"));
	if (!TestNotNull(TEXT("Transient bullet count"), BulletCount)) { return false; }
	TestEqual(TEXT("Fixture has no spread"), Weapon->GetCalculatedSpreadAngle(), 0.0f);
	TestEqual(TEXT("Fixture uses one line trace per bullet"), Weapon->GetBulletTraceSweepRadius(), 0.0f);

	auto CheckCartridge = [&](const TCHAR* Label, const TArray<ECollisionChannel>& Channels)
	{
		BulletCount->SetPropertyValue_InContainer(Weapon.Get(), Channels.Num());
		TArray<FHitResult> Hits;
		Ability->TraceForTest(Weapon.Get(), ASC->AbilityActorInfo.Get(), Channels, Hits);
		TestEqual(FString::Printf(TEXT("%s: trace count"), Label), Ability->GetTraceCountForTest(), Channels.Num());
		if (!TestEqual(FString::Printf(TEXT("%s: one result per bullet"), Label), Hits.Num(), Channels.Num())) { return; }
		for (int32 Index = 0; Index < Channels.Num(); ++Index)
		{
			const FString Prefix = FString::Printf(TEXT("%s bullet %d"), Label, Index);
			const bool bShouldHit = Channels[Index] == ECC_Visibility;
			TestEqual(Prefix + TEXT(" blocking state"), Hits[Index].bBlockingHit != 0, bShouldHit);
			if (bShouldHit)
			{
				TestTrue(Prefix + TEXT(" keeps the actual target"), Hits[Index].GetActor() == Target);
				TestTrue(Prefix + TEXT(" keeps the actual impact"), Hits[Index].ImpactPoint.Equals(FVector(480.0, 0.0, 0.0), 0.1));
			}
			else
			{
				TestNull(Prefix + TEXT(" has no target"), Hits[Index].GetActor());
				const FVector End = FVector::ForwardVector * Weapon->GetMaxDamageRange();
				TestTrue(Prefix + TEXT(" tracer reaches this bullet's end"), Hits[Index].ImpactPoint.Equals(End, 0.1));
				TestTrue(Prefix + TEXT(" location reaches this bullet's end"), Hits[Index].Location.Equals(End, 0.1));
			}
		}
	};
	CheckCartridge(TEXT("Single miss"), {ECC_Camera});
	CheckCartridge(TEXT("Single hit"), {ECC_Visibility});
	CheckCartridge(TEXT("All misses"), {ECC_Camera, ECC_Camera, ECC_Camera});
	CheckCartridge(TEXT("Hit then misses"), {ECC_Visibility, ECC_Camera, ECC_Camera});
	CheckCartridge(TEXT("Miss hit miss"), {ECC_Camera, ECC_Visibility, ECC_Camera});
	CheckCartridge(TEXT("All hits"), {ECC_Visibility, ECC_Visibility, ECC_Visibility});
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
