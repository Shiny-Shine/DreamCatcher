#include "Components/DCHealthComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Character/DCPawnExtensionComponent.h"
#include "DreamCatcher.h"
#include "GameplayEffect.h"

UDCHealthComponent::UDCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
}

void UDCHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	/**
	 * 플레이어 Character에는 PawnExtension이 있음.
	 *
	 * ASC가 이미 초기화되어 있다면 RegisterAndCall이
	 * HandleAbilitySystemInitialized를 즉시 한 번 호출.
	 */
	if (UDCPawnExtensionComponent* PawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemInitialized));

		PawnExtension->OnAbilitySystemUninitialized_Register(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUninitialized));
	}

	// ASC가 아직 없거나 PawnExtension이 없는 기존 적은 Legacy 체력으로 시작.
	if (!IsUsingAbilitySystem())
	{
		CurrentHealth = MaxHealth;
		bDeathBroadcasted = false;

		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}
}

void UDCHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeFromAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void UDCHealthComponent::InitializeWithAbilitySystem(UDCAbilitySystemComponent* InAbilitySystemComponent)
{
	if (AbilitySystemComponent == InAbilitySystemComponent && HealthSet)
	{
		return;
	}

	UninitializeFromAbilitySystem();

	if (!InAbilitySystemComponent)
	{
		UE_LOG(LogDreamCatcher, Error,
		       TEXT("HealthComponent [%s] cannot initialize ""with a null AbilitySystemComponent."),
		       *GetNameSafe(this));

		return;
	}

	const UDCHealthSet* FoundHealthSet = InAbilitySystemComponent->GetSet<UDCHealthSet>();

	if (!FoundHealthSet)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("HealthComponent [%s] cannot find ""DCHealthSet on ASC [%s]."),
		       *GetNameSafe(this), *GetNameSafe(InAbilitySystemComponent));

		return;
	}

	AbilitySystemComponent = InAbilitySystemComponent;

	HealthSet = FoundHealthSet;

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthAttributeChanged);

	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthAttributeChanged);

	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	// 이미 적용된 초기화 Effect의 값을 캐시에 반영.
	CurrentHealth = HealthSet->GetHealth();
	MaxHealth = HealthSet->GetMaxHealth();

	// 이미 죽은 ASC에 연결되었다고 해서 연결 순간 OnDeath를 다시 발생시키지는 않음.
	bDeathBroadcasted = CurrentHealth <= 0.0f;

	BroadcastCurrentHealth();

	UE_LOG(LogDreamCatcher, Log, TEXT("HealthComponent [%s] initialized with ""ASC [%s]. Health: %.1f / %.1f"),
	       *GetNameSafe(this), *GetNameSafe(AbilitySystemComponent), GetCurrentHealth(), GetMaxHealth());
}

void UDCHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		// 연결 해제 전에 마지막 Attribute 값을 캐시에 보관.
		CurrentHealth = HealthSet->GetHealth();
		MaxHealth = HealthSet->GetMaxHealth();

		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UDCHealthComponent::HandleAbilitySystemInitialized()
{
	const UDCPawnExtensionComponent* PawnExtension = UDCPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());

	if (!PawnExtension)
	{
		return;
	}

	InitializeWithAbilitySystem(PawnExtension->GetDCAbilitySystemComponent());
}

void UDCHealthComponent::HandleAbilitySystemUninitialized()
{
	UninitializeFromAbilitySystem();
}

float UDCHealthComponent::GetCurrentHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : CurrentHealth;
}

float UDCHealthComponent::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : MaxHealth;
}

bool UDCHealthComponent::IsDead() const
{
	return GetCurrentHealth() <= 0.0f;
}

void UDCHealthComponent::StartDeath()
{
	if (DeathState != EDCDeathState::NotDead)
	{
		return;
	}

	DeathState = EDCDeathState::DeathStarted;

	bDeathBroadcasted = true;

	if (AbilitySystemComponent)
	{
		// State.Dead는 AnimBP, HUD, Ability 차단 규칙이 공통으로 읽을 사망 상태.
		AbilitySystemComponent->SetLooseGameplayTagCount(DCGameplayTags::State_Dead, 1);

		// 기존 Ability 입력 처리도 즉시 중단.
		AbilitySystemComponent->SetLooseGameplayTagCount(DCGameplayTags::Gameplay_AbilityInputBlocked, 1);

		AbilitySystemComponent->ClearAbilityInput();
	}

	UE_LOG(LogDreamCatcher, Log, TEXT("HealthComponent [%s] started death ""for Actor [%s]."), *GetNameSafe(this),
	       *GetNameSafe(GetOwner()));

	// 기존 Character와 GameMode가 이 Delegate를 구독. 기존 이동 비활성화, BP_OnDeath, 레벨 재시작이 그대로 실행.
	OnDeath.Broadcast(GetOwner());

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UDCHealthComponent::FinishDeath()
{
	if (DeathState != EDCDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EDCDeathState::DeathFinished;

	OnDeathFinished.Broadcast(GetOwner());

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UDCHealthComponent::ClearGameplayTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent
		->SetLooseGameplayTagCount(
			DCGameplayTags::State_Dead,
			0
		);

	AbilitySystemComponent
		->SetLooseGameplayTagCount(
			DCGameplayTags
			::Gameplay_AbilityInputBlocked,
			0
		);
}

// 어떤 GameplayEffect, SetByCaller 태그, 얼마의 값을 누가 발생시켰는지를 받아,
// 실제 GAS Effect Spec을 만들어 현재 HealthComponent의 ASC에 적용하는 공통 함수
bool UDCHealthComponent::ApplySetByCallerGameplayEffect(
	TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const FGameplayTag& SetByCallerTag, float Magnitude,
	AActor* EffectCauser)
{
	if (!IsUsingAbilitySystem())
	{
		return false;
	}

	// Attribute 변경은 Authority에서만 실행.
	// 현재 싱글플레이에서는 서버와 로컬이 동일하지만, 구조는 GAS 권한 규칙을 따름.
	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return false;
	}

	if (!GameplayEffectClass)
	{
		UE_LOG(LogDreamCatcher, Error,
		       TEXT("HealthComponent [%s] cannot apply ""SetByCaller [%s]: GameplayEffect class ""is not assigned."),
		       *GetNameSafe(this), *SetByCallerTag.ToString());

		return false;
	}

	if (!SetByCallerTag.IsValid())
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("HealthComponent [%s] received an ""invalid SetByCaller tag."),
		       *GetNameSafe(this));

		return false;
	}

	// DamageCauser가 GAS Actor라면 해당 Actor의 ASC를 Effect Source로 사용.
	// 현재 Legacy 적처럼 ASC가 없다면 대상 ASC를 임시 Source로 사용하고, SetByCaller 값으로 실제 데미지를 전달.
	UAbilitySystemComponent* SourceAbilitySystemComponent = nullptr;

	if (EffectCauser)
	{
		SourceAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(EffectCauser, true);
	}

	if (!SourceAbilitySystemComponent)
	{
		SourceAbilitySystemComponent = AbilitySystemComponent;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();

	if (EffectCauser)
	{
		EffectContext.AddInstigator(EffectCauser, EffectCauser);

		EffectContext.AddSourceObject(EffectCauser);
	}

	FGameplayEffectSpecHandle EffectSpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(
		GameplayEffectClass, 1.0f, EffectContext);

	if (!EffectSpecHandle.IsValid())
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("HealthComponent [%s] failed to create ""GameplayEffect Spec from [%s]."),
		       *GetNameSafe(this), *GetNameSafe(GameplayEffectClass));

		return false;
	}

	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();

	if (!EffectSpec)
	{
		return false;
	}

	EffectSpec->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);

	SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*EffectSpec, AbilitySystemComponent);

	return true;
}

void UDCHealthComponent::SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth)
{
	const float ClampedMaxHealth = FMath::Max(NewMaxHealth, 1.0f);

	if (IsUsingAbilitySystem())
	{
		AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetMaxHealthAttribute(), ClampedMaxHealth);

		if (bResetCurrentHealth)
		{
			AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), ClampedMaxHealth);
		}

		BroadcastCurrentHealth();
		return;
	}

	MaxHealth = ClampedMaxHealth;

	CurrentHealth = bResetCurrentHealth ? MaxHealth : FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	if (CurrentHealth > 0.0f)
	{
		bDeathBroadcasted = false;
	}

	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth
	);
}

float UDCHealthComponent::ApplyDamage(float Damage, AActor* DamageCauser)
{
	if (Damage <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float PreviousHealth = GetCurrentHealth();

	const bool bUsingAbilitySystem = IsUsingAbilitySystem();

	if (bUsingAbilitySystem)
	{
		const bool bEffectApplied = ApplySetByCallerGameplayEffect(
			DamageGameplayEffectClass,
			DCGameplayTags::SetByCaller_Damage,
			Damage,
			DamageCauser
		);

		if (!bEffectApplied)
		{
			return 0.0f;
		}
	}
	else
	{
		// 아직 GAS로 전환하지 않은 기존 적은 Legacy 체력 경로를 유지.
		CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	}

	const float AppliedDamage = PreviousHealth - GetCurrentHealth();

	// GAS 경로는 DCHealthSet의 Delegate가 HUD와 죽음 이벤트를 발생.
	// Legacy 경로만 여기서 직접 이벤트를 보냄.
	if (!bUsingAbilitySystem && AppliedDamage > 0.0f)
	{
		BroadcastCurrentHealth();
		BroadcastDeathIfNeeded();
	}

	return FMath::Max(AppliedDamage, 0.0f);
}

float UDCHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float PreviousHealth = GetCurrentHealth();

	const bool bUsingAbilitySystem = IsUsingAbilitySystem();

	if (bUsingAbilitySystem)
	{
		const bool bEffectApplied = ApplySetByCallerGameplayEffect(
			HealGameplayEffectClass,
			DCGameplayTags::SetByCaller_Healing,
			Amount,
			GetOwner()
		);

		if (!bEffectApplied)
		{
			return 0.0f;
		}
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	}

	const float AppliedHealing = GetCurrentHealth() - PreviousHealth;

	if (!bUsingAbilitySystem && AppliedHealing > 0.0f)
	{
		bDeathBroadcasted = false;
		BroadcastCurrentHealth();
	}

	return FMath::Max(AppliedHealing, 0.0f);
}

void UDCHealthComponent::ResetToFull()
{
	if (IsUsingAbilitySystem())
	{
		AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), GetMaxHealth());
	}
	else
	{
		CurrentHealth = MaxHealth;
	}

	DeathState = EDCDeathState::NotDead;
	bDeathBroadcasted = false;

	ClearGameplayTags();
	BroadcastCurrentHealth();
}

void UDCHealthComponent::HandleHealthAttributeChanged(
	AActor* /* EffectInstigator */,
	AActor* /* EffectCauser */,
	const FGameplayEffectSpec* /* EffectSpec */,
	float /* EffectMagnitude */,
	float /* OldValue */,
	float NewValue
)
{
	CurrentHealth = NewValue;

	if (HealthSet)
	{
		MaxHealth = HealthSet->GetMaxHealth();
	}

	if (CurrentHealth > 0.0f)
	{
		bDeathBroadcasted = false;
	}

	OnHealthChanged.Broadcast(GetCurrentHealth(), GetMaxHealth());
}

void UDCHealthComponent::HandleMaxHealthAttributeChanged(
	AActor* /* EffectInstigator */,
	AActor* /* EffectCauser */,
	const FGameplayEffectSpec* /* EffectSpec */,
	float /* EffectMagnitude */,
	float /* OldValue */,
	float NewValue
)
{
	MaxHealth = NewValue;

	if (HealthSet)
	{
		CurrentHealth = HealthSet->GetHealth();
	}

	OnHealthChanged.Broadcast(GetCurrentHealth(), GetMaxHealth());
}

void UDCHealthComponent::HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser,
                                           const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
                                           float /* OldValue */, float /* NewValue */)
{
	// Legacy Actor이거나 ASC가 없는 경우에는 기존 방식으로 사망을 시작.
	if (!AbilitySystemComponent)
	{
		StartDeath();
		return;
	}

	// 사망 Event는 Authority에서만 발생시킴. 클라이언트는 이후 복제된 Tag와 상태를 관찰하게 됨.
	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	FGameplayEventData EventPayload;
	EventPayload.EventTag = DCGameplayTags::GameplayEvent_Death;

	EventPayload.Instigator = EffectInstigator ? EffectInstigator : EffectCauser;

	EventPayload.Target = GetOwner();
	EventPayload.EventMagnitude = EffectMagnitude;

	if (EffectSpec)
	{
		EventPayload.OptionalObject = EffectSpec->Def;

		EventPayload.ContextHandle = EffectSpec->GetEffectContext();

		EventPayload.InstigatorTags = *EffectSpec->CapturedSourceTags.GetAggregatedTags();

		EventPayload.TargetTags = *EffectSpec->CapturedTargetTags.GetAggregatedTags();
	}

	const int32 TriggeredAbilityCount = AbilitySystemComponent->HandleGameplayEvent(
		EventPayload.EventTag, &EventPayload);

	if (TriggeredAbilityCount <= 0)
	{
		// Death Ability가 누락되어도 플레이가 멈추지 않도록 기존 사망 처리로 대체.
		UE_LOG(LogDreamCatcher, Error,
		       TEXT("GameplayEvent.Death did not activate ""an Ability for [%s]. ""Starting death through fallback."),
		       *GetNameSafe(GetOwner()));

		StartDeath();
	}
}

void UDCHealthComponent::BroadcastCurrentHealth()
{
	CurrentHealth = GetCurrentHealth();
	MaxHealth = GetMaxHealth();

	if (CurrentHealth > 0.0f)
	{
		bDeathBroadcasted = false;
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UDCHealthComponent::BroadcastDeathIfNeeded()
{
	if (!IsDead())
	{
		return;
	}

	StartDeath();
}