#include "DCTraversalCombatComponent.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/DCLyraHealthComponent.h"
#include "Character/DCPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/DCLyraEquipmentInstance.h"
#include "Equipment/DCLyraEquipmentManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "NativeGameplayTags.h"
#include "System/DCAssetManager.h"
#include "System/DCGameData.h"
#include "Weapon/DCLyraWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCTraversalCombatComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_Traversing, "Status.Traversing");
DEFINE_LOG_CATEGORY_STATIC(LogDCTraversalCombat, Log, All);

namespace DCTraversalCombat
{
FGameplayTag ActionTag(const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name)); }

FGameplayTagContainer CancelTypes()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(ActionTag(TEXT("Ability.Type.Action.WeaponFire")));
	Tags.AddTag(ActionTag(TEXT("Ability.Type.Action.Reload")));
	Tags.AddTag(ActionTag(TEXT("Ability.Type.Action.ADS")));
	Tags.AddTag(ActionTag(TEXT("Ability.Type.Action.Jump")));
	return Tags;
}

bool IsAimSpec(const FGameplayAbilitySpec& Spec)
{
	const FGameplayTagContainer& Tags = Spec.GetDynamicSpecSourceTags();
	return Tags.HasTagExact(DCGameplayTags::InputTag_Aim)
		|| Tags.HasTagExact(DCGameplayTags::InputTag_Weapon_ADS_Shoulder)
		|| Tags.HasTagExact(DCGameplayTags::InputTag_Weapon_ADS_Scope);
}

bool IsWeaponMontage(const UAnimMontage* Montage)
{
	return Montage && (Montage->IsValidSlot(TEXT("UpperBody"))
		|| Montage->IsValidSlot(TEXT("UpperBodyAdditive"))
		|| Montage->IsValidSlot(TEXT("FullBodyAdditivePreAim")));
}

UDCTraversalCombatComponent* Coordinator(const AActor* Actor)
{
	auto* Component = UDCTraversalCombatComponent::FindTraversalCombatComponent(Actor);
	return Component && Actor->GetWorld() && Actor->GetWorld()->IsGameWorld()
		&& Actor->GetNetMode() == NM_Standalone ? Component : nullptr;
}
}

UDCTraversalCombatComponent::UDCTraversalCombatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	bAutoActivate = true;
	SetRelativeLocation(FVector(0.0, -15.0, 0.0));
	SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
}

UDCTraversalCombatComponent* UDCTraversalCombatComponent::FindTraversalCombatComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UDCTraversalCombatComponent>() : nullptr;
}

bool UDCTraversalCombatComponent::CanRequestTraversalOrJump(const AActor* Actor)
{
	const auto* Component = DCTraversalCombat::Coordinator(Actor);
	return !Component || Component->CanRequest();
}

bool UDCTraversalCombatComponent::TryBeginTraversalForActor(AActor* Actor, UAnimMontage* Montage, UPrimitiveComponent* Obstacle)
{
	auto* Component = DCTraversalCombat::Coordinator(Actor);
	return !Component || Component->BeginTraversal(Montage, Obstacle);
}

bool UDCTraversalCombatComponent::ConfirmTraversalForActor(AActor* Actor, UAnimMontage* Montage)
{
	auto* Component = DCTraversalCombat::Coordinator(Actor);
	return !Component || Component->ConfirmTraversal(Montage);
}

TEnumAsByte<EMovementMode> UDCTraversalCombatComponent::ResolveTraversalEndMovementMode(AActor* Actor, TEnumAsByte<EMovementMode> DesiredMode)
{
	const auto* Component = FindTraversalCombatComponent(Actor);
	const auto* Character = Cast<ACharacter>(Actor);
	if (!Component || !Component->IsStandaloneGame() || !Character) { return DesiredMode; }
	if (Component->bEndingPlay || Component->IsOwnerDead()) { return MOVE_None; }
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement) { return MOVE_None; }
	return ShouldRestoreTraversalMovementMode(Actor) ? DesiredMode : Movement->MovementMode;
}

bool UDCTraversalCombatComponent::ShouldRestoreTraversalMovementMode(AActor* Actor)
{
	const auto* Component = FindTraversalCombatComponent(Actor);
	const auto* Character = Cast<ACharacter>(Actor);
	if (!Component || !Component->IsStandaloneGame() || !Character) { return true; }
	if (Component->bEndingPlay || Component->IsOwnerDead()) { return true; }
	if (Component->bMovementCleanedUp) { return false; }
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement || Movement->MovementMode != MOVE_Flying) { return false; }
	if (UAnimInstance* Anim = Component->ActiveAnimInstance.Get())
	{
		const FAnimMontageInstance* RootMotion = Anim->GetRootMotionMontageInstance();
		if (Component->MontageInstanceID != INDEX_NONE && RootMotion
			&& RootMotion->GetInstanceID() != Component->MontageInstanceID)
		{
			return false;
		}
	}
	return true;
}

void UDCTraversalCombatComponent::NotifyTraversalMovementCleanup(AActor* Actor)
{
	if (auto* Component = FindTraversalCombatComponent(Actor); Component && Component->IsStandaloneGame())
	{
		Component->bMovementCleanedUp = true;
		Component->RestoreObstacleIgnore();
	}
}

bool UDCTraversalCombatComponent::IsTraversalCombatActive(const AActor* Actor)
{
	const auto* Component = FindTraversalCombatComponent(Actor);
	return Component && Component->bTraversalActive;
}

bool UDCTraversalCombatComponent::IsStandaloneGame() const
{
	return GetWorld() && GetWorld()->IsGameWorld() && GetOwner() && GetOwner()->GetNetMode() == NM_Standalone;
}

bool UDCTraversalCombatComponent::IsOwnerAlive() const
{
	const AActor* Owner = GetOwner();
	const auto* Health = UDCLyraHealthComponent::FindHealthComponent(Owner);
	return IsValid(Owner) && !Owner->IsActorBeingDestroyed() && Health
		&& Health->GetMaxHealth() > 0.0f && Health->GetHealth() > 0.0f && !Health->IsDeadOrDying();
}

bool UDCTraversalCombatComponent::IsOwnerDead() const
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) { return true; }
	const auto* Health = UDCLyraHealthComponent::FindHealthComponent(Owner);
	// Losing the ASC/attribute binding during unpossess is not itself a death.
	return Health && (Health->IsDeadOrDying() || (Health->GetMaxHealth() > 0.0f && Health->GetHealth() <= 0.0f));
}

UDCAbilitySystemComponent* UDCTraversalCombatComponent::FindOwnerASC() const
{
	if (const auto* Extension = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		return Extension->IsAbilitySystemUninitializing() ? nullptr : Extension->GetDCAbilitySystemComponent();
	}
	return Cast<UDCAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner(), true));
}

bool UDCTraversalCombatComponent::CanRequest() const
{
	// Let the old proxy's queued cleanup drain before another traversal can reuse its BP state.
	if (AdmissionAfterFrame != 0 && GFrameCounter <= AdmissionAfterFrame) { return false; }
	if (UAnimInstance* Retired = RetiredAnimInstance.Get(); Retired && RetiredMontageInstanceID != INDEX_NONE
		&& Retired->GetMontageInstanceForID(RetiredMontageInstanceID)) { return false; }
	const auto* Character = Cast<ACharacter>(GetOwner());
	const auto* ASC = FindOwnerASC();
	if (!IsActive() || bTraversalActive || bFinishing || bEndingPlay || !IsOwnerAlive() || !Character
		|| !Character->GetController() || Character->IsMoveInputIgnored() || !ASC
		|| ASC->GetAvatarActor() != Character || ASC->IsAbilityInputSuppressed()
		|| ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked)) { return false; }
	const auto* Movement = Character->GetCharacterMovement();
	if (!Movement || (!Movement->IsMovingOnGround() && !Movement->IsFalling())) { return false; }
	const FGameplayTag Dash = DCTraversalCombat::ActionTag(TEXT("Ability.Type.Action.Dash"));
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasTag(Dash)) { return false; }
	}
	return true;
}

void UDCTraversalCombatComponent::BeginPlay()
{
	bEndingPlay = false;
	Super::BeginPlay();
	if (auto* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleControllerChanged);
	}
	BoundHealth = UDCLyraHealthComponent::FindHealthComponent(GetOwner());
	if (auto* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
		Health->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleDeathStarted);
	}
	BoundPawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	if (auto* Extension = BoundPawnExtension.Get())
	{
		Extension->OnAbilitySystemUninitializing_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUninitializing));
	}
	SetComponentTickEnabled(false);
}

bool UDCTraversalCombatComponent::CaptureWeapon()
{
	WeaponAttachments.Reset();
	ActiveWeapon.Reset();
	const auto* Character = Cast<ACharacter>(GetOwner());
	if (!Character || GetAttachParent() != Character->GetMesh()
		|| GetAttachSocketName().IsNone() || !Character->GetMesh()->DoesSocketExist(GetAttachSocketName())) { return false; }
	auto* Manager = GetOwner()->FindComponentByClass<UDCLyraEquipmentManagerComponent>();
	if (!Manager) { return false; }
	ActiveWeapon = Manager->GetFirstInstanceOfType(UDCLyraWeaponInstance::StaticClass());
	if (!ActiveWeapon.IsValid()) { return true; } // Unarmed traversal still uses combat/input guards.
	const TArray<AActor*> Actors = ActiveWeapon->GetSpawnedActors();
	if (Actors.IsEmpty()) { return false; }
	for (AActor* Actor : Actors)
	{
		USceneComponent* Root = IsValid(Actor) ? Actor->GetRootComponent() : nullptr;
		const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Root);
		if (!Root || Actor->IsActorBeingDestroyed() || Actor == GetOwner() || !Root->GetAttachParent()
			|| Root->GetAttachParent() == this || Root->GetMobility() != EComponentMobility::Movable
			|| (Primitive && Primitive->IsSimulatingPhysics())) { WeaponAttachments.Reset(); return false; }
		FDCTraversalWeaponAttachment& Saved = WeaponAttachments.AddDefaulted_GetRef();
		Saved.Actor = Actor;
		Saved.Root = Root;
		Saved.Parent = Root->GetAttachParent();
		Saved.Socket = Root->GetAttachSocketName();
		Saved.RelativeTransform = Root->GetRelativeTransform();
	}
	return true;
}

bool UDCTraversalCombatComponent::IsCapturedWeaponStillEquipped() const
{
	if (!ActiveWeapon.IsValid() || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()) { return false; }
	const auto* Manager = GetOwner()->FindComponentByClass<UDCLyraEquipmentManagerComponent>();
	return Manager && Manager->GetEquipmentInstancesOfType(UDCLyraEquipmentInstance::StaticClass()).Contains(ActiveWeapon.Get());
}

bool UDCTraversalCombatComponent::HolsterCapturedWeapon()
{
	auto* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UDCLyraEquipmentManagerComponent>() : nullptr;
	if (!Manager || Manager->GetFirstInstanceOfType(UDCLyraWeaponInstance::StaticClass()) != ActiveWeapon.Get()) { return false; }
	if (WeaponAttachments.IsEmpty()) { return !ActiveWeapon.IsValid(); }
	if (!IsCapturedWeaponStillEquipped()) { return false; }
	const TWeakObjectPtr<UDCLyraEquipmentInstance> ExpectedWeapon = ActiveWeapon;
	const uint64 ExpectedScope = ScopeSerial;
	const bool bWasActive = bTraversalActive;
	const TArray<FDCTraversalWeaponAttachment> SavedAttachments = WeaponAttachments;
	for (const FDCTraversalWeaponAttachment& Saved : SavedAttachments)
	{
		if (ActiveWeapon != ExpectedWeapon || ScopeSerial != ExpectedScope || (bWasActive && !bTraversalActive)
			|| !IsCapturedWeaponStillEquipped()) { return false; }
		AActor* Actor = Saved.Actor.Get();
		USceneComponent* Root = Saved.Root.Get();
		if (!Actor || Actor->IsActorBeingDestroyed() || !Root || Actor->GetRootComponent() != Root
			|| Root->GetAttachParent() != Saved.Parent.Get() || Root->GetAttachSocketName() != Saved.Socket
			|| !Root->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform)) { return false; }
		if (ActiveWeapon != ExpectedWeapon || ScopeSerial != ExpectedScope || (bWasActive && !bTraversalActive)
			|| Root->GetAttachParent() != this) { return false; }
		Root->SetRelativeTransform(Saved.RelativeTransform);
	}
	return true;
}

void UDCTraversalCombatComponent::RestoreWeapon()
{
	if (IsCapturedWeaponStillEquipped())
	{
		const TArray<FDCTraversalWeaponAttachment> SavedAttachments = WeaponAttachments;
		for (const FDCTraversalWeaponAttachment& Saved : SavedAttachments)
		{
			if (!IsCapturedWeaponStillEquipped()) { break; }
			AActor* Actor = Saved.Actor.Get();
			USceneComponent* Root = Saved.Root.Get();
			USceneComponent* Parent = Saved.Parent.Get();
			const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Root);
			if (!Actor || Actor->IsActorBeingDestroyed() || !Root || !Parent || Actor->GetRootComponent() != Root
				|| Root->GetAttachParent() != this || !Root->GetAttachSocketName().IsNone()
				|| (Primitive && Primitive->IsSimulatingPhysics())
				|| (!Saved.Socket.IsNone() && !Parent->DoesSocketExist(Saved.Socket))) { continue; }
			if (Root->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform, Saved.Socket)
				&& Root->GetAttachParent() == Parent && Root->GetAttachSocketName() == Saved.Socket
				&& IsCapturedWeaponStillEquipped())
			{
				Root->SetRelativeTransform(Saved.RelativeTransform);
			}
		}
	}
	WeaponAttachments.Reset();
	ActiveWeapon.Reset();
}

bool UDCTraversalCombatComponent::BeginTraversal(UAnimMontage* Montage, UPrimitiveComponent* Obstacle)
{
	if (!CanRequest() || !Montage || Montage->GetPlayLength() <= 0.0f || !IsValid(Obstacle)) { return false; }
	auto* Character = CastChecked<ACharacter>(GetOwner());
	UDCAbilitySystemComponent* ASC = FindOwnerASC();
	UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
	if (!Anim || Anim->GetActiveInstanceForMontage(Montage) || !CaptureWeapon()) { return false; }
	const FGameplayTagContainer CancelTypes = DCTraversalCombat::CancelTypes();
	TArray<FGameplayAbilitySpecHandle> CombatHandles;
	BlockedAbilityTypes = CancelTypes;
	BlockedAbilityTypes.AddTag(DCTraversalCombat::ActionTag(TEXT("Ability.Type.Action.Dash")));
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability) { continue; }
		if (DCTraversalCombat::IsAimSpec(Spec)) { BlockedAbilityTypes.AppendTags(Spec.Ability->GetAssetTags()); }
		if (Spec.Ability->GetAssetTags().HasAny(CancelTypes) || DCTraversalCombat::IsAimSpec(Spec))
		{
			CombatHandles.Add(Spec.Handle);
			if (Spec.IsActive())
			{
				for (const UGameplayAbility* Instance : Spec.GetAbilityInstances())
				{
					if (Instance && !Instance->CanBeCanceled()) { WeaponAttachments.Reset(); ActiveWeapon.Reset(); return false; }
				}
			}
		}
	}
	const TSubclassOf<UGameplayEffect> TagEffect = UDCAssetManager::GetSubclass(UDCGameData::Get().DynamicTagGameplayEffect);
	FGameplayEffectSpecHandle StateSpec = TagEffect ? ASC->MakeOutgoingSpec(TagEffect, 1.0f, ASC->MakeEffectContext()) : FGameplayEffectSpecHandle();
	if (!StateSpec.IsValid()) { WeaponAttachments.Reset(); ActiveWeapon.Reset(); return false; }
	StateSpec.Data->DynamicGrantedTags.AddTag(TAG_Status_Traversing);
	ActiveASC = ASC;
	ActiveAnimInstance = Anim;
	ActiveMontage = Montage;
	TraversalObstacle = Obstacle;
	bObstacleWasIgnored = Character->GetCapsuleComponent()->GetMoveIgnoreComponents().Contains(Obstacle);
	bMovementClaimed = false;
	bMovementCleanedUp = false;
	MontageInstanceID = INDEX_NONE;
	RetiredAnimInstance.Reset();
	RetiredMontageInstanceID = INDEX_NONE;
	AdmissionAfterFrame = 0;
	PreparedFrame = GFrameCounter;
	const uint64 ThisScope = ++ScopeSerial;
	bTraversalActive = true;
	SetComponentTickEnabled(true);
	ASC->BeginAbilityInputSuppression(this);
	ASC->BlockAbilitiesWithTags(BlockedAbilityTypes);
	bOwnsAbilityBlocks = true;
	const FActiveGameplayEffectHandle AppliedState = ASC->ApplyGameplayEffectSpecToSelf(*StateSpec.Data.Get());
	// Tag callbacks can cancel this scope while the effect is being applied.
	if (!bTraversalActive || ScopeSerial != ThisScope)
	{
		if (AppliedState.IsValid()) { ASC->RemoveActiveGameplayEffect(AppliedState); }
		return false;
	}
	TraversalStateEffect = AppliedState;
	if (!TraversalStateEffect.IsValid()) { AbortTraversal(); return false; }
	for (const FGameplayAbilitySpecHandle& Handle : CombatHandles)
	{
		// Clear the ability's input bit before cancellation callbacks; the scope retains the release latch.
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle)) { Spec->InputPressed = false; }
	}
	ASC->CancelAimInputAndState();
	if (!bTraversalActive || ScopeSerial != ThisScope) { return false; }
	ASC->CancelAbilities(&CancelTypes);
	if (!bTraversalActive || ScopeSerial != ThisScope) { return false; }
	if (!IsOwnerAlive() || ASC->GetAvatarActor() != Character) { AbortTraversal(); return false; }
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && (Spec.Ability->GetAssetTags().HasAny(CancelTypes) || DCTraversalCombat::IsAimSpec(Spec)))
		{
			AbortTraversal(); return false;
		}
	}
	// Weapon presentation montages can be played outside GAS. Stop only their slots.
	TArray<UAnimMontage*> WeaponMontages;
	for (const FAnimMontageInstance* Instance : Anim->MontageInstances)
	{
		if (Instance && DCTraversalCombat::IsWeaponMontage(Instance->Montage)) { WeaponMontages.AddUnique(Instance->Montage); }
	}
	for (UAnimMontage* WeaponMontage : WeaponMontages)
	{
		Anim->Montage_Stop(0.1f, WeaponMontage);
		if (!bTraversalActive || ScopeSerial != ThisScope) { return false; }
	}
	const bool bHolstered = HolsterCapturedWeapon();
	if (!bTraversalActive || ScopeSerial != ThisScope) { return false; }
	if (!IsOwnerAlive() || !bHolstered) { AbortTraversal(); return false; }
	return true;
}

bool UDCTraversalCombatComponent::ConfirmTraversal(UAnimMontage* Montage)
{
	if (!bTraversalActive) { return false; }
	bMovementClaimed = true; // Caller has just run the original CMC start-state chain.
	UAnimInstance* Anim = ActiveAnimInstance.Get();
	FAnimMontageInstance* Instance = Anim && Montage == ActiveMontage.Get() ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
	if (!IsOwnerAlive() || !ActiveASC.IsValid() || ActiveASC->GetAvatarActor() != GetOwner() || !Instance)
	{
		AbortTraversal(); return false;
	}
	MontageInstanceID = Instance->GetInstanceID();
	return true;
}

void UDCTraversalCombatComponent::RestoreObstacleIgnore()
{
	if (auto* Character = Cast<ACharacter>(GetOwner()); Character && TraversalObstacle.IsValid())
	{
		Character->GetCapsuleComponent()->IgnoreComponentWhenMoving(TraversalObstacle.Get(), bObstacleWasIgnored);
	}
}

void UDCTraversalCombatComponent::CleanupMovement()
{
	if (!bMovementClaimed || bMovementCleanedUp) { return; }
	if (!bEndingPlay && IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed())
	{
		OnTraversalCleanupRequested.Broadcast();
	}
	// Fail-safe for an unbound cleanup hook; normal GASP completion uses its original selector.
	if (!bMovementCleanedUp)
	{
		if (auto* Character = Cast<ACharacter>(GetOwner()))
		{
			auto* Movement = Character->GetCharacterMovement();
			if (Movement && IsOwnerDead()) { Movement->DisableMovement(); }
			else if (Movement && Movement->MovementMode == MOVE_Flying) { Movement->SetMovementMode(MOVE_Falling); }
		}
		bMovementCleanedUp = true;
	}
	RestoreObstacleIgnore();
}

void UDCTraversalCombatComponent::StopTrackedMontage()
{
	UAnimInstance* Anim = ActiveAnimInstance.Get();
	UAnimMontage* Montage = ActiveMontage.Get();
	FAnimMontageInstance* Instance = Anim && Montage ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
	FAnimMontageInstance* Tracked = Anim && MontageInstanceID != INDEX_NONE ? Anim->GetMontageInstanceForID(MontageInstanceID) : Instance;
	if (Tracked && Tracked->Montage == Montage)
	{
		RetiredAnimInstance = Anim;
		RetiredMontageInstanceID = Tracked->GetInstanceID();
	}
	if (Instance && (MontageInstanceID == INDEX_NONE || Instance->GetInstanceID() == MontageInstanceID))
	{
		Anim->Montage_Stop(0.0f, Montage);
	}
}

void UDCTraversalCombatComponent::FinishTraversal(bool bAbort)
{
	if (!bTraversalActive || bFinishing) { return; }
	TGuardValue<bool> FinishingGuard(bFinishing, true);
	if (bAbort)
	{
		AdmissionAfterFrame = GFrameCounter + 1;
		StopTrackedMontage();
	}
	CleanupMovement();
	RestoreWeapon();
	if (auto* ASC = ActiveASC.Get(); ASC && IsValid(ASC->GetOwner()) && !ASC->GetOwner()->IsActorBeingDestroyed())
	{
		if (TraversalStateEffect.IsValid()) { ASC->RemoveActiveGameplayEffect(TraversalStateEffect); }
		if (bOwnsAbilityBlocks) { ASC->UnBlockAbilitiesWithTags(BlockedAbilityTypes); }
		ASC->EndAbilityInputSuppression(this);
	}
	TraversalStateEffect.Invalidate();
	bOwnsAbilityBlocks = false;
	BlockedAbilityTypes.Reset();
	ActiveASC.Reset();
	ActiveAnimInstance.Reset();
	ActiveMontage.Reset();
	MontageInstanceID = INDEX_NONE;
	bTraversalActive = false;
	SetComponentTickEnabled(false);
	// Keep the obstacle snapshot until the next begin: a queued original cleanup can still arrive.
}

void UDCTraversalCombatComponent::AbortTraversal() { FinishTraversal(true); }

void UDCTraversalCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bTraversalActive) { return; }
	const auto* Pawn = Cast<APawn>(GetOwner());
	const auto* Character = Cast<ACharacter>(Pawn);
	if (!IsStandaloneGame() || !IsOwnerAlive() || !Pawn || !Pawn->GetController() || !ActiveASC.IsValid()
		|| ActiveASC->GetAvatarActor() != GetOwner() || !ActiveAnimInstance.IsValid()) { AbortTraversal(); return; }
	if (!Character || Character->GetMesh()->GetAnimInstance() != ActiveAnimInstance.Get()
		|| Character->GetMesh()->IsSimulatingPhysics()
		|| (bMovementClaimed && !bMovementCleanedUp && Character->GetCharacterMovement()->MovementMode != MOVE_Flying))
	{
		AbortTraversal(); return;
	}
	if (MontageInstanceID == INDEX_NONE)
	{
		if (GFrameCounter > PreparedFrame + 1)
		{
			UE_LOG(LogDCTraversalCombat, Warning, TEXT("Traversal playback was not confirmed; rolling back %s."), *GetNameSafe(GetOwner()));
			bMovementClaimed = true;
			AbortTraversal();
		}
		return;
	}
	// Wait for the specific instance, including its blend-out. No per-montage delegate is replaced.
	if (!ActiveAnimInstance->GetMontageInstanceForID(MontageInstanceID)) { FinishTraversal(false); }
}

void UDCTraversalCombatComponent::HandleHealthChanged(UDCLyraHealthComponent*, float, float NewValue, AActor*)
{
	if (NewValue <= 0.0f) { AbortTraversal(); }
}
void UDCTraversalCombatComponent::HandleDeathStarted(AActor*) { AbortTraversal(); }
void UDCTraversalCombatComponent::HandleControllerChanged(APawn*, AController* OldController, AController* NewController)
{
	if (OldController != NewController) { AbortTraversal(); }
}
void UDCTraversalCombatComponent::HandleAbilitySystemUninitializing() { AbortTraversal(); }

void UDCTraversalCombatComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	if (!bTraversalActive) { SetComponentTickEnabled(false); }
}

void UDCTraversalCombatComponent::Deactivate()
{
	AbortTraversal();
	Super::Deactivate();
}

void UDCTraversalCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	AbortTraversal();
	if (auto* Pawn = Cast<APawn>(GetOwner())) { Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged); }
	if (auto* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
	}
	if (auto* Extension = BoundPawnExtension.Get()) { Extension->UnregisterAbilitySystemDelegates(this); }
	Super::EndPlay(EndPlayReason);
}
