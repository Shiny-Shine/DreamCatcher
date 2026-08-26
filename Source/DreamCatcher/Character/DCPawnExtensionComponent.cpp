#include "Character/DCPawnExtensionComponent.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCPawnData.h"
#include "DreamCatcher.h"
#include "GameFramework/Pawn.h"

UDCPawnExtensionComponent::UDCPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = nullptr;
}

UDCPawnExtensionComponent* UDCPawnExtensionComponent::FindPawnExtensionComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UDCPawnExtensionComponent>() : nullptr;
}

void UDCPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	const APawn* Pawn = Cast<APawn>(GetOwner());

	ensureAlwaysMsgf(Pawn, TEXT("DCPawnExtensionComponent can only be added to a Pawn. ""Owner: [%s]"),
	                 *GetNameSafe(GetOwner()));
}

void UDCPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void UDCPawnExtensionComponent::InitializeAbilitySystem(UDCAbilitySystemComponent* InAbilitySystemComponent,
                                                        AActor* InOwnerActor)
{
	check(InAbilitySystemComponent);
	check(InOwnerActor);

	APawn* Pawn = CastChecked<APawn>(GetOwner());

	// 같은 ASC와 Pawn이 이미 연결되어 있다면 AbilitySet을 중복 부여하지 않음.
	if (AbilitySystemComponent == InAbilitySystemComponent && InAbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		return;
	}

	// 기존 ASC가 있었다면 먼저 정리.
	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	// 같은 ASC에 이전 Pawn이 Avatar로 남아 있다면 이전 PawnExtension을 먼저 해제.
	AActor* ExistingAvatar = InAbilitySystemComponent->GetAvatarActor();

	if (ExistingAvatar && ExistingAvatar != Pawn)
	{
		if (UDCPawnExtensionComponent* OtherExtension = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtension->UninitializeAbilitySystem();
		}
		else
		{
			InAbilitySystemComponent->SetAvatarActor(nullptr);
		}
	}

	AbilitySystemComponent = InAbilitySystemComponent;

	/*
	 * ASC Owner  = PlayerState
	 * ASC Avatar = 현재 Character
	 */
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	// Ability 부여는 Authority에서만 수행.
	if (Pawn->HasAuthority())
	{
		if (!PawnData)
		{
			UE_LOG(LogDreamCatcher, Warning,
			       TEXT("PawnExtension [%s] has no PawnData. ""ASC was initialized, but no Pawn AbilitySet was granted."
			       ), *GetNameSafe(this));
		}
		else
		{
			PawnDataGrantedHandles.Reserve(PawnData->AbilitySets.Num());

			for (const UDCAbilitySet* AbilitySet : PawnData->AbilitySets)
			{
				if (!AbilitySet)
				{
					continue;
				}

				FDCAbilitySet_GrantedHandles& GrantedHandles = PawnDataGrantedHandles.Emplace_GetRef();

				AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles);
			}
		}
	}

	UE_LOG(LogDreamCatcher, Log,
	       TEXT("PawnExtension [%s] initialized ASC [%s]. ""Owner [%s], Avatar [%s], PawnData [%s]"),
	       *GetNameSafe(this),
	       *GetNameSafe(AbilitySystemComponent),
	       *GetNameSafe(InOwnerActor),
	       *GetNameSafe(Pawn),
	       *GetNameSafe(PawnData)
	);

	OnAbilitySystemInitialized.Broadcast();
}

void UDCPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());

	// PawnData가 부여했던 Ability, Effect, Attribute만 제거.
	for (FDCAbilitySet_GrantedHandles& GrantedHandles : PawnDataGrantedHandles)
	{
		GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}

	PawnDataGrantedHandles.Reset();

	// 현재 Pawn이 아직 ASC의 Avatar인 경우에만 Pawn 관련 실행 상태를 정리.
	if (Pawn && AbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		AbilitySystemComponent->CancelAbilities();
		AbilitySystemComponent->ClearAbilityInput();

		if (AbilitySystemComponent->GetOwnerActor())
		{
			// PlayerState Owner 정보는 유지하고 Avatar만 제거.
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			AbilitySystemComponent->ClearActorInfo();
		}
	}

	OnAbilitySystemUninitialized.Broadcast();

	UE_LOG(LogDreamCatcher, Log, TEXT("PawnExtension [%s] uninitialized ASC [%s] ""from Pawn [%s]."),
	       *GetNameSafe(this), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(Pawn));

	AbilitySystemComponent = nullptr;
}

void UDCPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void UDCPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}
