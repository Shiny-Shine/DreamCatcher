#include "DCRifleIntegrationTarget.h"

#include "AbilitySystem/Abilities/DCLyraGameplayAbility_Death.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayAbilitySpec.h"
#include "Physics/DCLyraCollisionChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCRifleIntegrationTarget)

DEFINE_LOG_CATEGORY_STATIC(LogDCRifleTarget, Log, All);

ADCRifleIntegrationTarget::ADCRifleIntegrationTarget()
{
	IntegrationHealthComponent = CreateDefaultSubobject<UDCLyraHealthComponent>(TEXT("RifleTargetHealth"));
	// Only the integration target participates in the original weapon trace channels.
	TargetMesh->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon, ECR_Block);
	TargetMesh->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Capsule, ECR_Block);
	TargetMesh->SetCollisionResponseToChannel(DCLyra_TraceChannel_Weapon_Multi, ECR_Block);
}

void ADCRifleIntegrationTarget::BeginPlay()
{
	Super::BeginPlay(); // The existing target owns/initializes its ASC and HealthSet.
	if (!AbilitySystemComponent || !DeathAbilityClass || DeathAbilityClass->HasAnyClassFlags(CLASS_Abstract)
		|| IntegrationTeamId == FGenericTeamId::NoTeam.GetId())
	{
		TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogDCRifleTarget, Error, TEXT("[R6] Integration target requires its ASC, a concrete original-derived Death Ability and a valid team."));
		return;
	}
	IntegrationHealthComponent->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleIntegrationDeathStarted);
	IntegrationHealthComponent->OnDeathFinished.AddUniqueDynamic(this, &ThisClass::HandleIntegrationDeathFinished);
	if (HasAuthority())
	{
		SetGenericTeamId(FGenericTeamId(IntegrationTeamId));
		GrantedDeathAbility = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, INDEX_NONE, this));
		if (!GrantedDeathAbility.IsValid())
		{
			TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UE_LOG(LogDCRifleTarget, Error, TEXT("[R6] Could not grant the integration target's Death Ability."));
			return;
		}
	}
	// Original HealthComponent sends GameplayEvent.Death; the granted original ability
	// owns StartDeath/FinishDeath and its Blueprint's delay/FX. No direct HP subtraction.
	IntegrationHealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	bHealthInitialized = true;
	UE_LOG(LogDCRifleTarget, Log, TEXT("[R6] Target ready: %s Health=%.1f Team=%d DeathAbility=%s"),
		*GetName(), GetCurrentHealth(), static_cast<int32>(GetGenericTeamId().GetId()), *GetNameSafe(DeathAbilityClass.Get()));
}

void ADCRifleIntegrationTarget::HandleIntegrationDeathStarted(AActor* OwningActor)
{
	if (OwningActor != this) { return; }
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UE_LOG(LogDCRifleTarget, Log, TEXT("[R6] Target death started: %s Health=%.1f"), *GetName(), GetCurrentHealth());
}

void ADCRifleIntegrationTarget::HandleIntegrationDeathFinished(AActor* OwningActor)
{
	if (OwningActor != this) { return; }
	UE_LOG(LogDCRifleTarget, Log, TEXT("[R6] Target death finished: %s"), *GetName());
	if (HasAuthority())
	{
		// Defer destruction until the original Death Ability has returned from EndAbility.
		SetLifeSpan(0.1f);
	}
}

void ADCRifleIntegrationTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IntegrationHealthComponent->OnDeathStarted.RemoveAll(this);
	IntegrationHealthComponent->OnDeathFinished.RemoveAll(this);
	if (HasAuthority() && AbilitySystemComponent && GrantedDeathAbility.IsValid())
	{
		AbilitySystemComponent->ClearAbility(GrantedDeathAbility);
		GrantedDeathAbility = FGameplayAbilitySpecHandle();
	}
	if (bHealthInitialized)
	{
		IntegrationHealthComponent->UninitializeFromAbilitySystem();
		bHealthInitialized = false;
	}
	Super::EndPlay(EndPlayReason);
}
