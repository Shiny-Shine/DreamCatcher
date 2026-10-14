#include "AbilitySystem/Abilities/DCGameplayAbility_BossLineShot.h"
#include "AbilitySystem/Abilities/DCGameplayAbility_BossGroundStrike.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/DCBossCharacter.h"
#include "AI/Boss/DCBossData.h"
#include "AI/Boss/DCBossPhaseComponent.h"
#include "AI/Boss/DCBossTelegraph.h"
#include "Engine/World.h"

UDCGameplayAbility_BossLineShot::UDCGameplayAbility_BossLineShot(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DamageAmount = 15.f;
	AttackRange = 2500.f;
	WindupTime = 1.2f;
	RecoveryTime = .65f;
}

bool UDCGameplayAbility_BossLineShot::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* RelevantTags) const
{
	const ADCBossCharacter* B = ActorInfo ? Cast<ADCBossCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const FGameplayAbilitySpec* Other = B ? B->GetAbilitySystemComponent()->FindAbilitySpecFromClass(B->GetGroundAbilityClass()) : nullptr;
	return B && (!Other || !Other->IsActive()) && B->CanRunBossCombat() && B->GetBossPhaseComponent()->GetBossRunState() == EDCBossRunState::Fighting
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, RelevantTags);
}

void UDCGameplayAbility_BossLineShot::BP_OnAttackIntent_Implementation(FVector TraceStart, FVector AimPoint, float DelaySeconds)
{
	ADCBossCharacter* Boss = Cast<ADCBossCharacter>(GetAvatarActorFromActorInfo());
	if (!Boss || !Boss->GetBossData()) { return; }
	const FVector End = TraceStart + (AimPoint - TraceStart).GetSafeNormal() * AttackRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCBossLineWarning), false, Boss);
	const bool bHit = Boss->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, End, ECC_Visibility, Params);
	Warning = ADCBossTelegraph::Line(Boss, TraceStart, bHit ? Hit.ImpactPoint : End,
		Boss->GetBossData()->WarningMaterial, DelaySeconds + .5f);
}

void UDCGameplayAbility_BossLineShot::BP_OnAttackShot_Implementation(FVector TraceStart, FVector TraceEnd, AActor*)
{
	if (Warning.IsValid()) { Warning->Destroy(); }
	Warning.Reset();
	if (ADCBossCharacter* Boss = Cast<ADCBossCharacter>(GetAvatarActorFromActorInfo()); Boss && Boss->GetBossData())
	{
		ADCBossTelegraph::Line(Boss, TraceStart, TraceEnd, Boss->GetBossData()->ImpactMaterial, .16f);
	}
}

void UDCGameplayAbility_BossLineShot::BP_OnAttackEnded_Implementation(bool)
{
	if (Warning.IsValid()) { Warning->Destroy(); }
	Warning.Reset();
}
