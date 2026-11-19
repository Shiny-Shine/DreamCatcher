#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Character/DCLyraHealthComponent.h"
#include "Character/DCTraversalCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Equipment/DCLyraEquipmentInstance.h"
#include "Equipment/DCLyraEquipmentManagerComponent.h"
#include "Equipment/DCLyraQuickBarComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/DCInventoryItemDefinition.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Inventory/DCInventoryManagerComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCDashDefenseTestTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Weapon/DCLyraWeaponInstance.h"

// Test only the attachment sub-operation independently of montage/ability setup.
struct FDCTraversalCombatTestAccessor
{
	static bool Mount(UDCTraversalCombatComponent* Component)
	{
		return Component->CaptureWeapon() && Component->HolsterCapturedWeapon();
	}
	static void Restore(UDCTraversalCombatComponent* Component) { Component->RestoreWeapon(); }
	// Simple automation tests do not advance the engine frame counter; only expire this local guard.
	static void NextAdmissionFrame(UDCTraversalCombatComponent* Component) { Component->AdmissionAfterFrame = 0; }
};

namespace DCTraversalTests
{
constexpr const TCHAR* MeshPath = TEXT("/Game/DreamCatcher/Meshes/Characters/UEFN_Mannequin/SKM_UEFN_Mannequin.SKM_UEFN_Mannequin");
constexpr const TCHAR* IdlePath = TEXT("/Game/DreamCatcher/Animations/Characters/UEFN_Mannequin/Idle/M_Neutral_Stand_Idle_Loop.M_Neutral_Stand_Idle_Loop");
constexpr const TCHAR* ItemPath = TEXT("/Game/Tests/Equipment/ID_DC_R5_EquipmentSmoke.ID_DC_R5_EquipmentSmoke_C");

template <typename T>
T* AddComponent(AActor* Owner)
{
	T* Component = NewObject<T>(Owner, NAME_None, RF_Transient);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

struct FFixture
{
	FTestWorldWrapper World;
	ACharacter* Pawn = nullptr;
	APlayerController* Controller = nullptr;
	UDCAbilitySystemComponent* ASC = nullptr;
	UDCTraversalCombatComponent* Bridge = nullptr;
	UDCLyraEquipmentManagerComponent* Equipment = nullptr;
	UDCInventoryManagerComponent* Inventory = nullptr;
	UDCLyraQuickBarComponent* QuickBar = nullptr;
	TArray<TStrongObjectPtr<UAnimMontage>> Montages;

	bool Start(FAutomationTestBase& Test, bool bWithWeapon)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCTraversalIsolatedAutomation")))
		{
			Test.AddError(TEXT("Use a separate UnrealEditor-Cmd process with -DCTraversalIsolatedAutomation. These tests are not PIE."));
			return false;
		}
		if (!World.CreateTestWorld(EWorldType::Game)) { World.ForwardErrorMessages(&Test); return false; }
		World.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
		if (!World.BeginPlayInTestWorld()) { World.ForwardErrorMessages(&Test); return false; }
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Controller = World.GetTestWorld()->SpawnActor<APlayerController>(Params);
		Pawn = World.GetTestWorld()->SpawnActor<ACharacter>(Params);
		if (!Test.TestNotNull(TEXT("Native controller"), Controller) || !Test.TestNotNull(TEXT("Native pawn"), Pawn)) { return false; }
		Controller->Possess(Pawn);
		ASC = AddComponent<UDCAbilitySystemComponent>(Pawn);
		ASC->AddAttributeSetSubobject(NewObject<UDCHealthSet>(Pawn, NAME_None, RF_Transient));
		ASC->InitAbilityActorInfo(Pawn, Pawn);
		ASC->SetNumericAttributeBase(UDCHealthSet::GetMaxHealthAttribute(), 100.0f);
		ASC->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), 100.0f);
		AddComponent<UDCLyraHealthComponent>(Pawn)->InitializeWithAbilitySystem(ASC);
		if (!bWithWeapon) { return true; }
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
		if (!Test.TestNotNull(TEXT("Original UEFN mesh"), Mesh)) { return false; }
		Pawn->GetMesh()->SetSkeletalMeshAsset(Mesh);
		Pawn->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
		Equipment = AddComponent<UDCLyraEquipmentManagerComponent>(Pawn);
		Inventory = AddComponent<UDCInventoryManagerComponent>(Controller);
		QuickBar = AddComponent<UDCLyraQuickBarComponent>(Controller);
		UClass* ItemClass = LoadClass<UDCInventoryItemDefinition>(nullptr, ItemPath);
		if (!Test.TestNotNull(TEXT("Existing equipment fixture"), ItemClass)) { return false; }
		UDCInventoryItemInstance* Item = Inventory->AddItemDefinition(ItemClass, 1);
		QuickBar->AddItemToSlot(0, Item);
		QuickBar->SetActiveSlotIndex(0);
		Bridge = NewObject<UDCTraversalCombatComponent>(Pawn, NAME_None, RF_Transient);
		Pawn->AddInstanceComponent(Bridge);
		Bridge->SetupAttachment(Pawn->GetMesh(), TEXT("spine_03"));
		Bridge->RegisterComponent();
		Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		return Test.TestNotNull(TEXT("Equipped fixture actor"), WeaponActor())
			&& Test.TestTrue(TEXT("Coordinator auto-activated"), Bridge->IsActive());
	}

	UDCLyraEquipmentInstance* Weapon() const
	{
		return Equipment ? Equipment->GetFirstInstanceOfType(UDCLyraWeaponInstance::StaticClass()) : nullptr;
	}
	AActor* WeaponActor() const
	{
		const auto* Instance = Weapon();
		const TArray<AActor*> Actors = Instance ? Instance->GetSpawnedActors() : TArray<AActor*>();
		return Actors.Num() == 1 ? Actors[0] : nullptr;
	}
	UAnimMontage* NewMontage()
	{
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, IdlePath);
		UAnimMontage* Montage = Idle ? UAnimMontage::CreateSlotAnimationAsDynamicMontage(Idle, TEXT("DefaultSlot"), 0.0f, 0.1f) : nullptr;
		if (Montage) { Montages.Emplace(Montage); }
		return Montage;
	}
	bool Begin(UAnimMontage* Montage)
	{
		return UDCTraversalCombatComponent::TryBeginTraversalForActor(Pawn, Montage, Pawn->GetCapsuleComponent());
	}
	bool PlayAndConfirm(UAnimMontage* Montage)
	{
		UAnimInstance* Anim = Pawn->GetMesh()->GetAnimInstance();
		if (!Anim || Anim->Montage_Play(Montage) <= 0.0f) { return false; }
		Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		return UDCTraversalCombatComponent::ConfirmTraversalForActor(Pawn, Montage);
	}
	void TickBridge() { Bridge->TickComponent(0.016f, LEVELTICK_All, nullptr); }
	void AdvanceMontages(float DeltaSeconds)
	{
		if (UAnimInstance* Anim = Pawn->GetMesh()->GetAnimInstance())
		{
			Anim->TickMontageOnly(DeltaSeconds);
			// Retire instances invalidated by that update, without advancing time again.
			Anim->TickMontageOnly(0.0f);
		}
	}
	void AdvancePastAbort()
	{
		AdvanceMontages(0.1f);
		FDCTraversalCombatTestAccessor::NextAdmissionFrame(Bridge);
	}
	void ApplyHealthChange(float Amount)
	{
		// Exercise the health set's gameplay-effect callbacks, as actual damage/healing does.
		TStrongObjectPtr<UGameplayEffect> Effect(NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient));
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Amount < 0.0f ? UDCHealthSet::GetDamageAttribute() : UDCHealthSet::GetHealingAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(FMath::Abs(Amount));
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(Pawn, Pawn);
		FGameplayEffectSpec Spec(Effect.Get(), Context, 1.0f);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCTraversalInputSuppressionTest,
	"DreamCatcher.Traversal.InputReleasePolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCTraversalInputSuppressionTest::RunTest(const FString&)
{
	using namespace DCTraversalTests;
	// Reuse the existing native, unsaved fixture; restore its CDO after this test.
	auto* AbilityCDO = GetMutableDefault<UDCDashDefenseAutomationAbility>();
	FEnumProperty* PolicyProperty = FindFProperty<FEnumProperty>(UDCGameplayAbility::StaticClass(), TEXT("ActivationPolicy"));
	if (!TestNotNull(TEXT("Activation policy property"), PolicyProperty)) { return false; }
	TGuardValue<EDCAbilityActivationPolicy> PolicyGuard(*PolicyProperty->ContainerPtrToValuePtr<EDCAbilityActivationPolicy>(AbilityCDO), EDCAbilityActivationPolicy::WhileInputActive);
	FFixture Fixture;
	if (!Fixture.Start(*this, false)) { return false; }
	TestTrue(TEXT("Sample without coordinator retains its original admission path"), UDCTraversalCombatComponent::CanRequestTraversalOrJump(Fixture.Pawn));
	TestTrue(TEXT("Sample without coordinator retains its original playback path"), UDCTraversalCombatComponent::TryBeginTraversalForActor(Fixture.Pawn, nullptr, nullptr));
	const FGameplayTag Fire = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.FireAuto"));
	const FGameplayTag Reload = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.Reload"));
	FGameplayAbilitySpec Grant(UDCDashDefenseAutomationAbility::StaticClass(), 1);
	Grant.GetDynamicSpecSourceTags().AddTag(Fire);
	const FGameplayAbilitySpecHandle Handle = Fixture.ASC->GiveAbility(Grant);
	auto Spec = [&]() { return Fixture.ASC->FindAbilitySpecFromHandle(Handle); };
	Fixture.ASC->AbilityInputTagPressed(Fire);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	if (!TestNotNull(TEXT("Granted ability"), Spec()) || !TestTrue(TEXT("Unscoped input activates normally"), Spec()->IsActive())) { return false; }
	UObject* const SourceA = Fixture.Pawn;
	UObject* const SourceB = Fixture.Controller;
	Fixture.ASC->BeginAbilityInputSuppression(SourceA);
	Fixture.ASC->BeginAbilityInputSuppression(SourceA);
	Fixture.ASC->BeginAbilityInputSuppression(SourceB);
	TestTrue(TEXT("Already-held fire requires release"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Fire));
	Fixture.ASC->AbilityInputTagReleased(Fire);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestFalse(TEXT("Release is processed even while suppressed"), Spec()->InputPressed);
	TestTrue(TEXT("Unrelated active ability was not blanket-canceled"), Spec()->IsActive());
	Fixture.ASC->CancelAbilityHandle(Handle);
	Fixture.ASC->AbilityInputTagPressed(Fire);
	Fixture.ASC->AbilityInputTagPressed(Reload);
	Fixture.ASC->EndAbilityInputSuppression(SourceA);
	Fixture.ASC->EndAbilityInputSuppression(SourceA);
	TestTrue(TEXT("Ending one owner preserves the other owner"), Fixture.ASC->IsAbilityInputSuppressed());
	Fixture.ASC->EndAbilityInputSuppression(SourceB);
	TestFalse(TEXT("All owners ended"), Fixture.ASC->IsAbilityInputSuppressed());
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestFalse(TEXT("Held-input activation cannot resume on scope end"), Spec()->IsActive());
	Fixture.ASC->AbilityInputTagPressed(Fire); // Enhanced Input can keep sending Triggered.
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestFalse(TEXT("Repeated Triggered still requires release"), Spec()->IsActive());
	TestTrue(TEXT("An input first pressed during traversal is also latched"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Reload));
	Fixture.ASC->AbilityInputTagReleased(Fire);
	Fixture.ASC->AbilityInputTagReleased(Reload);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	Fixture.ASC->AbilityInputTagPressed(Fire);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestTrue(TEXT("A fresh press works"), Spec()->IsActive());
	Fixture.ASC->CancelAbilityHandle(Handle);
	Fixture.ASC->BeginAbilityInputSuppression(SourceA);
	APawn* NextAvatar = Fixture.World.GetTestWorld()->SpawnActor<APawn>();
	Fixture.ASC->InitAbilityActorInfo(Fixture.Pawn, NextAvatar);
	TestFalse(TEXT("Suppression does not leak to a new avatar"), Fixture.ASC->IsAbilityInputSuppressed());
	TestFalse(TEXT("Release latches do not leak to a new avatar"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Fire));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCTraversalADSInputCleanupTest,
	"DreamCatcher.Traversal.ADSLogicalInputCleanup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCTraversalADSInputCleanupTest::RunTest(const FString&)
{
	using namespace DCTraversalTests;
	auto* AbilityCDO = GetMutableDefault<UDCDashDefenseAutomationAbility>();
	FEnumProperty* PolicyProperty = FindFProperty<FEnumProperty>(UDCGameplayAbility::StaticClass(), TEXT("ActivationPolicy"));
	if (!TestNotNull(TEXT("Activation policy property"), PolicyProperty)) { return false; }
	TGuardValue<EDCAbilityActivationPolicy> PolicyGuard(*PolicyProperty->ContainerPtrToValuePtr<EDCAbilityActivationPolicy>(AbilityCDO), EDCAbilityActivationPolicy::WhileInputActive);
	FFixture Fixture;
	if (!Fixture.Start(*this, false)) { return false; }
	const FGameplayTag Shoulder = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.ADS.Shoulder"));
	const FGameplayTag Scope = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.ADS.Scope"));
	const FGameplayTag Fire = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.FireAuto"));
	const FGameplayTag PhysicalAim = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Aim"));
	FGameplayAbilitySpec Grant(UDCDashDefenseAutomationAbility::StaticClass(), 1);
	Grant.GetDynamicSpecSourceTags().AddTag(Shoulder);
	Grant.GetDynamicSpecSourceTags().AddTag(Scope);
	const FGameplayAbilitySpecHandle Handle = Fixture.ASC->GiveAbility(Grant);
	auto Spec = [&]() { return Fixture.ASC->FindAbilitySpecFromHandle(Handle); };
	if (!TestNotNull(TEXT("Logical ADS test grant"), Spec())) { return false; }

	Fixture.ASC->AbilityInputTagPressed(Shoulder);
	Fixture.ASC->AbilityInputTagPressed(Scope);
	Fixture.ASC->AbilityInputTagPressed(Fire);
	Fixture.ASC->AbilityInputTagPressed(PhysicalAim);
	Fixture.ASC->BeginAbilityInputSuppression(Fixture.Pawn);
	Fixture.ASC->BeginAbilityInputSuppression(Fixture.Controller);
	TestTrue(TEXT("Shoulder was latched before cancellation"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Shoulder));
	TestTrue(TEXT("Scope was latched before cancellation"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Scope));
	Fixture.ASC->CancelAimInputAndState();
	Fixture.ASC->CancelAimInputAndState();
	TestFalse(TEXT("Canceled Shoulder logical press no longer waits for a release"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Shoulder));
	TestFalse(TEXT("Canceled Scope logical press no longer waits for a release"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Scope));
	TestTrue(TEXT("Fire still requires its physical release"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Fire));
	TestTrue(TEXT("Legacy physical Aim still requires its release"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(PhysicalAim));
	Fixture.ASC->EndAbilityInputSuppression(Fixture.Pawn);
	TestTrue(TEXT("ADS cancellation preserves the other suppression owner"), Fixture.ASC->IsAbilityInputSuppressed());
	Fixture.ASC->EndAbilityInputSuppression(Fixture.Controller);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestFalse(TEXT("Canceled pending ADS does not auto-activate when traversal ends"), Spec()->IsActive());

	// A second traversal must not re-latch a stale logical press from the first one.
	Fixture.ASC->BeginAbilityInputSuppression(Fixture.Pawn);
	TestFalse(TEXT("A later scope does not revive canceled Shoulder input"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Shoulder));
	TestFalse(TEXT("A later scope does not revive canceled Scope input"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Scope));
	Fixture.ASC->EndAbilityInputSuppression(Fixture.Pawn);
	Fixture.ASC->AbilityInputTagPressed(Shoulder);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestTrue(TEXT("A newly routed Shoulder press can activate"), Spec()->IsActive());
	Fixture.ASC->CancelAimInputAndState();
	TestFalse(TEXT("Active logical ADS still cancels normally"), Spec()->IsActive());
	Fixture.ASC->AbilityInputTagPressed(Scope);
	Fixture.ASC->ProcessAbilityInput(0.016f, false);
	TestTrue(TEXT("A newly routed Scope press can activate"), Spec()->IsActive());
	Fixture.ASC->CancelAimInputAndState();
	TestTrue(TEXT("ADS recovery never clears the held-fire latch"), Fixture.ASC->IsAbilityInputTagAwaitingRelease(Fire));
	Fixture.ASC->AbilityInputTagReleased(Fire);
	Fixture.ASC->AbilityInputTagReleased(PhysicalAim);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCTraversalAttachmentTest,
	"DreamCatcher.Traversal.EquipmentAttachmentLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCTraversalAttachmentTest::RunTest(const FString&)
{
	using namespace DCTraversalTests;
	FFixture Fixture;
	if (!Fixture.Start(*this, true)) { return false; }
	AActor* FirstActor = Fixture.WeaponActor();
	UDCLyraEquipmentInstance* FirstInstance = Fixture.Weapon();
	USceneComponent* Root = FirstActor->GetRootComponent();
	USceneComponent* Parent = Root->GetAttachParent();
	const FName Socket = Root->GetAttachSocketName();
	const FTransform Relative = Root->GetRelativeTransform();
	UDCInventoryItemInstance* Item = Fixture.QuickBar->GetActiveSlotItem();
	const FGameplayTag Stat = FGameplayTag::RequestGameplayTag(TEXT("Test.R2.CostCharge"));
	const int32 StatBefore = Item->GetStatTagStackCount(Stat);
	TestTrue(TEXT("Capture and holster"), FDCTraversalCombatTestAccessor::Mount(Fixture.Bridge));
	TestTrue(TEXT("Same actor on back mount"), Root->GetAttachParent() == Fixture.Bridge);
	TestTrue(TEXT("Same equipment instance"), Fixture.Weapon() == FirstInstance);
	TestEqual(TEXT("Item stats unchanged"), Item->GetStatTagStackCount(Stat), StatBefore);
	FDCTraversalCombatTestAccessor::Restore(Fixture.Bridge);
	FDCTraversalCombatTestAccessor::Restore(Fixture.Bridge);
	TestTrue(TEXT("Exact parent restored"), Root->GetAttachParent() == Parent);
	TestEqual(TEXT("Exact socket restored"), Root->GetAttachSocketName(), Socket);
	TestTrue(TEXT("Exact relative transform restored"), Root->GetRelativeTransform().Equals(Relative));
	TestTrue(TEXT("Holster before external detach"), FDCTraversalCombatTestAccessor::Mount(Fixture.Bridge));
	Root->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	const FTransform DetachedWorld = Root->GetComponentTransform();
	FDCTraversalCombatTestAccessor::Restore(Fixture.Bridge);
	TestNull(TEXT("Externally detached weapon is not reclaimed"), Root->GetAttachParent());
	TestTrue(TEXT("External world transform preserved"), Root->GetComponentTransform().Equals(DetachedWorld));
	Root->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform, Socket);
	Root->SetRelativeTransform(Relative);
	TestTrue(TEXT("Holster before replacement"), FDCTraversalCombatTestAccessor::Mount(Fixture.Bridge));
	Fixture.QuickBar->RemoveItemFromSlot(0);
	UClass* ItemClass = LoadClass<UDCInventoryItemDefinition>(nullptr, ItemPath);
	Fixture.QuickBar->AddItemToSlot(1, Fixture.Inventory->AddItemDefinition(ItemClass, 1));
	Fixture.QuickBar->SetActiveSlotIndex(1);
	AActor* Replacement = Fixture.WeaponActor();
	if (!TestNotNull(TEXT("Replacement actor"), Replacement)) { return false; }
	FDCTraversalCombatTestAccessor::Restore(Fixture.Bridge);
	TestTrue(TEXT("Old actor remains destroyed"), !IsValid(FirstActor) || FirstActor->IsActorBeingDestroyed());
	TestTrue(TEXT("Replacement actor is untouched"), Replacement->GetRootComponent()->GetAttachParent() == Parent);
	TestTrue(TEXT("Holster before direct destruction"), FDCTraversalCombatTestAccessor::Mount(Fixture.Bridge));
	Replacement->Destroy();
	FDCTraversalCombatTestAccessor::Restore(Fixture.Bridge);
	TestTrue(TEXT("Destroyed actor is not recreated"), !IsValid(Replacement) || Replacement->IsActorBeingDestroyed());
	TestEqual(TEXT("Original inventory item still retains its stat"), Item->GetStatTagStackCount(Stat), StatBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCTraversalLifetimeTest,
	"DreamCatcher.Traversal.MontageAndCancellationLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCTraversalLifetimeTest::RunTest(const FString&)
{
	using namespace DCTraversalTests;
	FFixture Fixture;
	if (!Fixture.Start(*this, true)) { return false; }
	UAnimInstance* Anim = Fixture.Pawn->GetMesh()->GetAnimInstance();
	if (!TestNotNull(TEXT("Native animation instance"), Anim)) { return false; }
	UAnimMontage* First = Fixture.NewMontage();
	if (!TestNotNull(TEXT("Transient original-sequence montage"), First)) { return false; }
	TestFalse(TEXT("Invalid selection does not begin"), Fixture.Begin(nullptr));
	TestFalse(TEXT("Invalid selection did not suppress input"), Fixture.ASC->IsAbilityInputSuppressed());
	TestTrue(TEXT("Valid selection prepares"), Fixture.Begin(First));
	// Simulate a failed original PlayMontage after CMC start-state changes.
	Fixture.Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	TestFalse(TEXT("Missing playback fails confirmation"), UDCTraversalCombatComponent::ConfirmTraversalForActor(Fixture.Pawn, First));
	TestFalse(TEXT("Failed start releases its scope"), Fixture.ASC->IsAbilityInputSuppressed());
	TestTrue(TEXT("Failed start restores the weapon"), Fixture.WeaponActor()->GetRootComponent()->GetAttachParent() == Fixture.Pawn->GetMesh());
	TestTrue(TEXT("Failed start does not leave Flying movement"), Fixture.Pawn->GetCharacterMovement()->MovementMode != MOVE_Flying);
	TestFalse(TEXT("Immediate restart waits for queued old cleanup"), UDCTraversalCombatComponent::CanRequestTraversalOrJump(Fixture.Pawn));
	Fixture.AdvancePastAbort();
	TestTrue(TEXT("Prepare first playback"), Fixture.Begin(First));
	if (!TestTrue(TEXT("Start and confirm first instance"), Fixture.PlayAndConfirm(First))) { return false; }
	Fixture.Pawn->GetCharacterMovement()->SetMovementMode(UDCTraversalCombatComponent::ResolveTraversalEndMovementMode(Fixture.Pawn, MOVE_Walking));
	UDCTraversalCombatComponent::NotifyTraversalMovementCleanup(Fixture.Pawn);
	TestTrue(TEXT("Blend-out movement cleanup does not restore the weapon early"), Fixture.WeaponActor()->GetRootComponent()->GetAttachParent() == Fixture.Bridge);
	TestTrue(TEXT("Combat remains suppressed until the instance ends"), Fixture.ASC->IsAbilityInputSuppressed());
	Anim->Montage_Stop(0.0f, First);
	Fixture.AdvanceMontages(0.05f);
	Fixture.TickBridge();
	TestFalse(TEXT("Instance completion releases combat"), Fixture.ASC->IsAbilityInputSuppressed());
	TestTrue(TEXT("Instance completion restores the same actor"), Fixture.WeaponActor()->GetRootComponent()->GetAttachParent() == Fixture.Pawn->GetMesh());
	UAnimMontage* Second = Fixture.NewMontage();
	if (!TestTrue(TEXT("Prepare second playback"), Fixture.Begin(Second)) || !TestTrue(TEXT("Confirm second playback"), Fixture.PlayAndConfirm(Second))) { return false; }
	Anim->OnMontageEnded.Broadcast(First, true);
	Fixture.TickBridge();
	TestTrue(TEXT("A stale end notification does not release a new instance"), UDCTraversalCombatComponent::IsTraversalCombatActive(Fixture.Pawn));
	const FGameplayTag Traversing = FGameplayTag::RequestGameplayTag(TEXT("Status.Traversing"));
	const FGameplayTagContainer FireType(FGameplayTag::RequestGameplayTag(TEXT("Ability.Type.Action.WeaponFire")));
	UObject* const OtherOwner = Fixture.Controller;
	Fixture.ASC->BeginAbilityInputSuppression(OtherOwner);
	Fixture.ASC->BlockAbilitiesWithTags(FireType);
	FGameplayEffectSpecHandle OtherSpec = Fixture.ASC->MakeOutgoingSpec(UDCDashDefenseAutomationEffect::StaticClass(), 1.0f, Fixture.ASC->MakeEffectContext());
	OtherSpec.Data->SetDuration(60.0f, true);
	OtherSpec.Data->DynamicGrantedTags.AddTag(Traversing);
	const FActiveGameplayEffectHandle OtherEffect = Fixture.ASC->ApplyGameplayEffectSpecToSelf(*OtherSpec.Data.Get());
	Fixture.Bridge->AbortTraversal();
	Fixture.Bridge->AbortTraversal();
	TestTrue(TEXT("Other input suppression survives duplicate cleanup"), Fixture.ASC->IsAbilityInputSuppressed());
	TestTrue(TEXT("Other ability block survives duplicate cleanup"), Fixture.ASC->AreAbilityTagsBlocked(FireType));
	TestEqual(TEXT("Only our traversal effect was removed"), Fixture.ASC->GetGameplayTagCount(Traversing), 1);
	Fixture.ASC->EndAbilityInputSuppression(OtherOwner);
	Fixture.ASC->UnBlockAbilitiesWithTags(FireType);
	Fixture.ASC->RemoveActiveGameplayEffect(OtherEffect);
	TestFalse(TEXT("No residual input scope"), Fixture.ASC->IsAbilityInputSuppressed());
	TestFalse(TEXT("No residual ability block"), Fixture.ASC->AreAbilityTagsBlocked(FireType));
	Fixture.AdvancePastAbort();
	Fixture.Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Custom, 7);
	TestFalse(TEXT("External custom movement must bypass the original setter"), UDCTraversalCombatComponent::ShouldRestoreTraversalMovementMode(Fixture.Pawn));
	TestEqual(TEXT("External custom submode is preserved"), Fixture.Pawn->GetCharacterMovement()->CustomMovementMode, static_cast<uint8>(7));
	Fixture.Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UAnimMontage* DeathMontage = Fixture.NewMontage();
	if (!TestTrue(TEXT("Prepare death case"), Fixture.Begin(DeathMontage)) || !TestTrue(TEXT("Confirm death case"), Fixture.PlayAndConfirm(DeathMontage))) { return false; }
	Fixture.ApplyHealthChange(-100.0f);
	TestFalse(TEXT("HP zero cancels traversal"), UDCTraversalCombatComponent::IsTraversalCombatActive(Fixture.Pawn));
	TestEqual(TEXT("Death cannot restore walking"), Fixture.Pawn->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	TestFalse(TEXT("Death clears our input scope"), Fixture.ASC->IsAbilityInputSuppressed());
	Fixture.ApplyHealthChange(100.0f);
	Fixture.Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking); // Explicit fixture reset, not runtime resurrection.
	Fixture.AdvancePastAbort();
	UAnimMontage* Last = Fixture.NewMontage();
	if (!TestTrue(TEXT("Prepare unpossess case"), Fixture.Begin(Last)) || !TestTrue(TEXT("Confirm unpossess case"), Fixture.PlayAndConfirm(Last))) { return false; }
	Fixture.Controller->UnPossess();
	TestFalse(TEXT("Unpossess cancels traversal"), UDCTraversalCombatComponent::IsTraversalCombatActive(Fixture.Pawn));
	TestFalse(TEXT("Unpossess releases input scope"), Fixture.ASC->IsAbilityInputSuppressed());
	TestTrue(TEXT("Unpossess restores existing equipment"), Fixture.WeaponActor()->GetRootComponent()->GetAttachParent() == Fixture.Pawn->GetMesh());
	return true;
}

#endif
