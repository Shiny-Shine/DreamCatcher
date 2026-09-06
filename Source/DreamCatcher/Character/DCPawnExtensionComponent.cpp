#include "Character/DCPawnExtensionComponent.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCPawnData.h"
#include "DreamCatcher.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"

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

	// 연결 해제 콜백 안에서 다시 초기화하여 ASC 참조가 바뀌는 것을 막음.
	if (bUninitializingAbilitySystem)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("PawnExtension [%s]: initialization requested during uninitialization."),
		       *GetNameSafe(this));

		return;
	}

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

	// ASC Owner  = PlayerState ASC Avatar = 현재 Character
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
	if (!AbilitySystemComponent || bUninitializingAbilitySystem)
	{
		return;
	}

	bUninitializingAbilitySystem = true;

	// 중간에 함수가 종료되더라도 재진입 방지 상태는 반드시 복구.
	ON_SCOPE_EXIT
	{
		bUninitializingAbilitySystem = false;
	};

	// 중요: ASC의 Avatar와 Pawn AbilitySet이 살아 있는 동안 장비부터 정리.
	OnAbilitySystemUninitializing.Broadcast();

	APawn* Pawn = Cast<APawn>(GetOwner());

	// PawnData의 Ability를 회수하기 전에 조준 입력과 상태를 정리.
	if (Pawn && AbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		AbilitySystemComponent->CancelAimInputAndState();
	}

	// 이 PawnData가 부여했던 GAS 항목만 회수.
	for (FDCAbilitySet_GrantedHandles& GrantedHandles : PawnDataGrantedHandles)
	{
		GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}

	PawnDataGrantedHandles.Reset();

	// 다른 Pawn이 이미 사용 중인 ASC의 Avatar를 지우지 않도록 확인.
	if (Pawn && AbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		AbilitySystemComponent->CancelAbilities();
		AbilitySystemComponent->ClearAbilityInput();

		if (AbilitySystemComponent->GetOwnerActor())
		{
			// PlayerState Owner는 유지하고 현재 Pawn Avatar만 제거.
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			AbilitySystemComponent->ClearActorInfo();
		}
	}

	// 기존 HealthComponent 등의 연결 해제 처리는 그대로 유지.
	OnAbilitySystemUninitialized.Broadcast();

	UE_LOG(LogDreamCatcher, Log, TEXT("PawnExtension [%s] uninitialized ASC [%s] from Pawn [%s]."),
	       *GetNameSafe(this), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(Pawn));

	AbilitySystemComponent = nullptr;
}

void UDCPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	// 연결 해제 중인 ASC를 준비 완료 상태로 전달 X.
	if (AbilitySystemComponent && !bUninitializingAbilitySystem)
	{
		Delegate.ExecuteIfBound();
	}
}

void UDCPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}

void UDCPawnExtensionComponent::OnAbilitySystemUninitializing_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	// 동일한 객체가 같은 수명 이벤트에 중복 등록되는 것을 방지.
	if (!OnAbilitySystemUninitializing.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitializing.Add(Delegate);
	}
}

void UDCPawnExtensionComponent::UnregisterAbilitySystemDelegates(UObject* Listener)
{
	if (!Listener)
	{
		return;
	}

	OnAbilitySystemInitialized.RemoveAll(Listener);
	OnAbilitySystemUninitializing.RemoveAll(Listener);
	OnAbilitySystemUninitialized.RemoveAll(Listener);
}
