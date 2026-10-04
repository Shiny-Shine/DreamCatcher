#include "AI/DCGASTestTarget.h"

#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/DCGameplayEffectContext.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystemGlobals.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "DreamCatcher.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Equipment/DCEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "System/DCAssetManager.h"
#include "System/DCGameData.h"
#include "Teams/DCTeamSubsystem.h"
#include "UObject/ScriptInterface.h"
#include "Weapon/DCRangedWeaponInstance.h"

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

void ADCGASTestTarget::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (!HasAuthority())
	{
		return;
	}

	const FGenericTeamId OldTeamID = TestTeamID;
	TestTeamID = NewTeamID;

	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
}

FGenericTeamId ADCGASTestTarget::GetGenericTeamId() const
{
	return TestTeamID;
}

FOnDCTeamIndexChangedDelegate* ADCGASTestTarget::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

bool ADCGASTestTarget::ConfigureTestTeams(AActor* SourceActor, int32 SourceTeamId, int32 TargetTeamId)
{
	const auto IsValidTeamIndex = [](int32 TeamId)
	{
		return TeamId == INDEX_NONE ||
			(TeamId >= 0 && TeamId < static_cast<int32>(FGenericTeamId::NoTeam.GetId()));
	};

	UWorld* World = GetWorld();

	if (!World ||
		!HasAuthority() ||
		!IsValid(SourceActor) ||
		SourceActor == this ||
		!SourceActor->HasAuthority() ||
		SourceActor->GetWorld() != World ||
		!IsValidTeamIndex(SourceTeamId) ||
		!IsValidTeamIndex(TargetTeamId))
	{
		return false;
	}

	UDCTeamSubsystem* TeamSubsystem = World->GetSubsystem<UDCTeamSubsystem>();
	if (!TeamSubsystem)
	{
		return false;
	}

	if (!TeamSubsystem->ChangeTeamForActor(SourceActor, SourceTeamId))
	{
		return false;
	}

	if (!TeamSubsystem->ChangeTeamForActor(this, TargetTeamId))
	{
		return false;
	}

	const int32 ActualSourceTeam = TeamSubsystem->FindTeamFromObject(SourceActor);
	const int32 ActualTargetTeam = TeamSubsystem->FindTeamFromObject(this);

	UE_LOG(LogDreamCatcher, Log, TEXT("[R3-2] Teams: Source [%s]=%d, Target [%s]=%d"),
		*GetNameSafe(SourceActor), ActualSourceTeam, *GetNameSafe(this), ActualTargetTeam);

	return ActualSourceTeam == SourceTeamId && ActualTargetTeam == TargetTeamId;
}

float ADCGASTestTarget::ApplyWeaponSourceTestDamage(AActor* SourceActor, float SetByCallerDamage)
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority() || !IsValid(SourceActor) || SourceActor == this ||
		!SourceActor->HasAuthority() || SourceActor->GetWorld() != World ||
		!IsValid(TargetMesh) || !TargetMesh->IsRegistered() || !FMath::IsFinite(SetByCallerDamage))
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("[R3-2] WeaponSource test: invalid source, target, authority, or magnitude."));
		return 0.0f;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
	if (!IsValid(SourceASC) || !IsValid(AbilitySystemComponent) ||
		SourceASC->GetAvatarActor() != SourceActor || AbilitySystemComponent->GetAvatarActor() != this ||
		!SourceASC->IsOwnerActorAuthoritative() || !SourceASC->GetSet<UDCCombatSet>() ||
		!AbilitySystemComponent->GetSet<UDCHealthSet>())
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("[R3-2] WeaponSource test: source/target ASC or required attributes are not ready."));
		return 0.0f;
	}

	UDCEquipmentManagerComponent* EquipmentManager = SourceActor->FindComponentByClass<UDCEquipmentManagerComponent>();
	UDCRangedWeaponInstance* WeaponInstance = IsValid(EquipmentManager)
		? Cast<UDCRangedWeaponInstance>(EquipmentManager->GetCurrentWeaponInstance())
		: nullptr;
	if (!IsValid(WeaponInstance) || !WeaponInstance->IsEquipped() || WeaponInstance->GetPawn() != SourceActor)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("[R3-2] WeaponSource test: [%s] has no equipped ranged weapon."),
			*GetNameSafe(SourceActor));
		return 0.0f;
	}

	// Deterministic test trace: actor location to mesh bounds center, independent of the legacy fire path.
	const FVector TraceStart = SourceActor->GetActorLocation();
	const FVector TraceEnd = TargetMesh->GetBounds().Origin;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DCWeaponSourceTest), false);
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.AddIgnoredActor(SourceActor);
	TraceParams.AddIgnoredActors(WeaponInstance->GetSpawnedActors());

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams) ||
		HitResult.GetActor() != this)
	{
		UE_LOG(LogDreamCatcher, Warning, TEXT("[R3-2] WeaponSource test: trace hit [%s] instead of target [%s]."),
			*GetNameSafe(HitResult.GetActor()), *GetNameSafe(this));
		return 0.0f;
	}

	const TSubclassOf<UGameplayEffect> DamageGE =
		UDCAssetManager::GetSubclass(UDCGameData::Get().DamageGameplayEffect_SetByCaller);
	if (!DamageGE)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R3-2] WeaponSource test: GameData has no SetByCaller damage GE."));
		return 0.0f;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FDCGameplayEffectContext* EffectContext = FDCGameplayEffectContext::ExtractEffectContext(ContextHandle);
	if (!EffectContext)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R3-2] WeaponSource test: expected FDCGameplayEffectContext."));
		return 0.0f;
	}

	// ASC::MakeEffectContext already supplies owner/avatar. Add the original weapon source and trace data.
	EffectContext->SetAbilitySource(WeaponInstance, 1.0f);
	ContextHandle.AddSourceObject(WeaponInstance);
	ContextHandle.AddOrigin(TraceStart);
	ContextHandle.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("[R3-2] WeaponSource test: could not create damage spec."));
		return 0.0f;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(DCGameplayTags::SetByCaller_Damage, SetByCallerDamage);

	// These values are diagnostic only. The original execution owns damage math and team filtering.
	const float Distance = static_cast<float>(FVector::Dist(TraceStart, HitResult.ImpactPoint));
	const float DistanceMultiplier = FMath::Max(WeaponInstance->GetDistanceAttenuation(Distance), 0.0f);
	const float MaterialMultiplier = WeaponInstance->GetPhysicalMaterialAttenuation(HitResult.PhysMaterial.Get());
	const FString WeaponName = GetNameSafe(WeaponInstance);
	const FString PhysicalMaterialName = GetNameSafe(HitResult.PhysMaterial.Get());
	const float PreviousHealth = GetCurrentHealth();

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);

	const float CurrentHealth = GetCurrentHealth();
	const float AppliedDamage = FMath::Max(PreviousHealth - CurrentHealth, 0.0f);
	UE_LOG(LogDreamCatcher, Log,
		TEXT("[R3-2] WeaponSource: Weapon=%s Distance=%.1fcm DistanceMultiplier=%.3f PhysMat=%s ")
		TEXT("MaterialMultiplier=%.3f SetByCaller=%.2f AppliedDamage=%.2f TargetHealth=%.2f->%.2f"),
		*WeaponName, Distance, DistanceMultiplier, *PhysicalMaterialName,
		MaterialMultiplier, SetByCallerDamage, AppliedDamage, PreviousHealth, CurrentHealth);

	return AppliedDamage;
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
