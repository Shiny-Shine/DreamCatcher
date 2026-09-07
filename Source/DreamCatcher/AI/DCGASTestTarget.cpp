#include "AI/DCGASTestTarget.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "Components/StaticMeshComponent.h"
#include "DreamCatcher.h"

ADCGASTestTarget::ADCGASTestTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	SetRootComponent(TargetMesh);

	TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetMesh->SetCollisionObjectType(ECC_WorldDynamic);
	TargetMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	TargetMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	TargetMesh->SetGenerateOverlapEvents(false);
	TargetMesh->SetCanEverAffectNavigation(false);

	AbilitySystemComponent = CreateDefaultSubobject<UDCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	HealthSet = CreateDefaultSubobject<UDCHealthSet>(TEXT("HealthSet"));
}

UAbilitySystemComponent* ADCGASTestTarget::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ADCGASTestTarget::GetCurrentHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

void ADCGASTestTarget::BeginPlay()
{
	Super::BeginPlay();

	// 表적 자신이 ASC의 Owner와 Avatar를 모두 담당.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		const float StartingHealth = FMath::Max(InitialHealth, 1.0f);

		AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetMaxHealthAttribute(), StartingHealth);

		AbilitySystemComponent->SetNumericAttributeBase(UDCHealthSet::GetHealthAttribute(), StartingHealth);
	}

	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UDCHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);

	BP_OnHealthChanged(HealthSet->GetHealth(), HealthSet->GetMaxHealth());
}

void ADCGASTestTarget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogDreamCatcher, Log, TEXT("GASTestTarget [%s]: Health %.1f -> %.1f / %.1f"),
	       *GetName(), Data.OldValue, Data.NewValue, HealthSet->GetMaxHealth());

	BP_OnHealthChanged(Data.NewValue, HealthSet->GetMaxHealth());
}

void ADCGASTestTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent && HealthChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UDCHealthSet::GetHealthAttribute()).Remove(
			HealthChangedHandle);

		HealthChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
