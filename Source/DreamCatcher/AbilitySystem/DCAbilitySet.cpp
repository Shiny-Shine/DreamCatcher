#include "AbilitySystem/DCAbilitySet.h"

#include "AbilitySystem/Abilities/DCGameplayAbility.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"

void FDCAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FDCAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FDCAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* AttributeSet)
{
	if (IsValid(AttributeSet))
	{
		GrantedAttributeSets.Add(AttributeSet);
	}
}

void FDCAbilitySet_GrantedHandles::TakeFromAbilitySystem(UDCAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Ability와 Effect 부여·제거는 Authority에서만 수행함.
	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* AttributeSet : GrantedAttributeSets)
	{
		if (IsValid(AttributeSet))
		{
			AbilitySystemComponent->RemoveSpawnedAttribute(AttributeSet);
		}
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

UDCAbilitySet::UDCAbilitySet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDCAbilitySet::GiveToAbilitySystem(
	UDCAbilitySystemComponent* AbilitySystemComponent,
	FDCAbilitySet_GrantedHandles* OutGrantedHandles,
	UObject* SourceObject
) const
{
	check(AbilitySystemComponent);

	// GAS 항목 부여는 Authority에서만 수행.
	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	// AttributeSet을 먼저 생성.
	// Ability나 Effect가 활성화될 때 Attribute가 이미 존재하도록Ability보다 먼저 처리.
	for (int32 Index = 0; Index < GrantedAttributes.Num(); ++Index)
	{
		const FDCAbilitySet_AttributeSet& AttributeToGrant = GrantedAttributes[Index];

		if (!AttributeToGrant.AttributeSet)
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySet [%s]: GrantedAttributes[%d] is invalid."), *GetNameSafe(this), Index);
			continue;
		}

		UAttributeSet* NewAttributeSet =NewObject<UAttributeSet>(AbilitySystemComponent->GetOwner(), AttributeToGrant.AttributeSet);

		AbilitySystemComponent->AddAttributeSetSubobject(NewAttributeSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewAttributeSet);
		}
	}

	// Gameplay Ability를 부여합니다.
	for (int32 Index = 0; Index < GrantedGameplayAbilities.Num(); ++Index)
	{
		const FDCAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[Index];

		if (!AbilityToGrant.Ability)
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySet [%s]: GrantedGameplayAbilities[%d] is invalid."), *GetNameSafe(this), Index);
			continue;
		}

		UDCGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UDCGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);

		// 어떤 장비나 시스템이 Ability를 부여했는지 기록.
		AbilitySpec.SourceObject = SourceObject;
		
		// Ability Spec에 InputTag를 넣음.
		// 이후 ASC가 입력 태그와 동일한 Ability를 찾아활성화할 때 사용. 
		if (AbilityToGrant.InputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);
		}

		const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilityHandle);
		}
	}

	// Gameplay Effect를 ASC 자신에게 적용.
	for (int32 Index = 0; Index < GrantedGameplayEffects.Num(); ++Index)
	{
		const FDCAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[Index];

		if (!EffectToGrant.GameplayEffect)
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySet [%s]: GrantedGameplayEffects[%d] is invalid."), *GetNameSafe(this), Index);
			continue;
		}

		const UGameplayEffect* GameplayEffectCDO = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();

		if (SourceObject)
		{
			EffectContext.AddSourceObject(SourceObject);
		}

		const FActiveGameplayEffectHandle EffectHandle =
			AbilitySystemComponent->ApplyGameplayEffectToSelf(
				GameplayEffectCDO,
				EffectToGrant.EffectLevel,
				EffectContext
			);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(EffectHandle);
		}
	}
}
