#include "Weapon/DCGameplayAbility_RangedWeapon.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "DreamCatcher.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Weapon/DCRangedWeaponInstance.h"
#include "Weapon/DCWeaponActor.h"
#include "GameplayEffectTypes.h"

namespace
{
	// Lyra를 참고해 원뿔 내부의 방향을 선택. Exponent가 클수록 중심 근처에 탄환이 더 많이 모임.
	FVector MakeSpreadDirection(const FVector& ForwardDirection, float HalfAngleRadians, float Exponent)
	{
		const FVector Forward = ForwardDirection.GetSafeNormal();

		if (HalfAngleRadians <= KINDA_SMALL_NUMBER)
		{
			return Forward;
		}

		FVector AxisA;
		FVector AxisB;
		Forward.FindBestAxisVectors(AxisA, AxisB);

		const float AngleFromCenter = HalfAngleRadians * FMath::Pow(FMath::FRand(), FMath::Max(Exponent, 0.1f));

		const float AngleAround = FMath::FRand() * 2.0f * PI;

		const FVector RadialDirection = AxisA * FMath::Cos(AngleAround) + AxisB * FMath::Sin(AngleAround);

		return (Forward * FMath::Cos(AngleFromCenter) + RadialDirection * FMath::Sin(AngleFromCenter)).GetSafeNormal();
	}
}

UDCGameplayAbility_RangedWeapon::UDCGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 입력을 유지하면 Ability가 끝날 때마다 ASC가 다시 활성화.
	ActivationPolicy = EDCAbilityActivationPolicy::WhileInputActive;

	// 현재 Standalone 플레이 기준.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 실행 중인 Ability를 입력마다 다시 시작 X.
	bRetriggerInstancedAbility = false;

	FGameplayTagContainer FireAbilityTags;
	FireAbilityTags.AddTag(DCGameplayTags::Ability_Action_WeaponFire);
	SetAssetTags(FireAbilityTags);

	ActivationBlockedTags.AddTag(DCGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(DCGameplayTags::State_Dodging);
	ActivationBlockedTags.AddTag(DCGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(DCGameplayTags::Gameplay_AbilityInputBlocked);

	// 한 발 처리와 발사 간격 대기 중에 유지됨. Ability가 종료되면 GAS가 자동으로 제거.
	ActivationOwnedTags.AddTag(DCGameplayTags::State_Firing);
}

UDCRangedWeaponInstance* UDCGameplayAbility_RangedWeapon::GetWeaponInstance() const
{
	return Cast<UDCRangedWeaponInstance>(GetAssociatedEquipment());
}

bool UDCGameplayAbility_RangedWeapon::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !DamageEffectClass)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	APawn* SourcePawn = Cast<APawn>(ActorInfo->AvatarActor.Get());

	if (!IsValid(SourceASC) || !IsValid(SourcePawn))
	{
		return false;
	}

	// CanActivateAbility는 활성화 전에 호출됨. CurrentSpec 대신 전달받은 Handle로 장비를 찾음.
	const FGameplayAbilitySpec* AbilitySpec = SourceASC->FindAbilitySpecFromHandle(Handle);

	const UDCRangedWeaponInstance* RangedWeapon =
		AbilitySpec ? Cast<UDCRangedWeaponInstance>(AbilitySpec->SourceObject.Get()) : nullptr;

	if (!IsValid(RangedWeapon) || !RangedWeapon->IsEquipped() || RangedWeapon->GetPawn() != SourcePawn)
	{
		return false;
	}

	const UDCHealthSet* OwnerHealth = SourceASC->GetSet<UDCHealthSet>();

	if (!OwnerHealth || OwnerHealth->GetHealth() <= 0.0f)
	{
		return false;
	}

	if (!SourceASC->GetSet<UDCCombatSet>())
	{
		return false;
	}

	ADCWeaponActor* WeaponActor = RangedWeapon->GetWeaponActor();

	FTransform MuzzleTransform;

	if (!IsValid(WeaponActor)
		|| !WeaponActor->TryGetMuzzleTransform(MuzzleTransform))
	{
		return false;
	}

	// Ability가 중간에 취소되더라도 발사 간격을 우회하지 못하게 함.
	const float TimeSinceLastShot = RangedWeapon->GetTimeSinceLastFired();

	if (TimeSinceLastShot >= 0.0f && TimeSinceLastShot < RangedWeapon->GetFireInterval())
	{
		return false;
	}

	return true;
}

void UDCGameplayAbility_RangedWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	UDCRangedWeaponInstance* WeaponInstance = GetWeaponInstance();

	if (!IsValid(Pawn) || !Pawn->HasAuthority() || !World || !IsValid(SourceASC) || !IsValid(WeaponInstance) || !
		WeaponInstance->IsEquipped() || WeaponInstance->GetPawn() != Pawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ADCWeaponActor* WeaponActor = WeaponInstance->GetWeaponActor();

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());

	const UDCCombatSet* CombatSet = SourceASC->GetSet<UDCCombatSet>();

	FTransform MuzzleTransform;

	if (!IsValid(WeaponActor) || !PC || !CombatSet || !DamageEffectClass || !WeaponActor->TryGetMuzzleTransform(
		MuzzleTransform))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const float MaxRange = WeaponInstance->GetMaxDamageRange();
	const float FireInterval = WeaponInstance->GetFireInterval();

	// 입력이나 이동 상태가 이번 프레임에 바뀌었을 수 있으므로 첫발 정확도와 최종 퍼짐을 발사 직전에 다시 판단.
	WeaponInstance->RefreshSpreadAngle();

	// 이번 탄에 사용할 값은 발사 전 상태로 고정. 첫발 정확도가 활성화되어 있으면 0도가 반환됨.
	const float ShotSpreadDegrees = WeaponInstance->GetCurrentSpreadAngle();

	const float HalfSpreadRadians = FMath::DegreesToRadians(ShotSpreadDegrees * 0.5f);

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector CameraShotDirection = MakeSpreadDirection(
		ViewRotation.Vector(), HalfSpreadRadians, WeaponInstance->GetSpreadExponent());

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DCGASRifleShot), false, Pawn);

	TraceParams.AddIgnoredActor(Pawn);

	for (AActor* SpawnedActor : WeaponInstance->GetSpawnedActors())
	{
		TraceParams.AddIgnoredActor(SpawnedActor);
	}

	FHitResult ShotHit;
	FVector ShotTraceStart = MuzzleLocation;
	FVector ShotTraceEnd = MuzzleLocation;

	// 총구가 벽을 뚫고 반대편으로 나간 경우도 확인.
	// Pawn의 눈 위치에서 총구까지 막혀 있다면 그 장애물을 맞음.
	const FVector BodyOrigin = Pawn->GetPawnViewLocation();

	const bool bBodyToMuzzleBlocked = World->LineTraceSingleByChannel(
		ShotHit, BodyOrigin, MuzzleLocation, ECC_Visibility, TraceParams);

	if (bBodyToMuzzleBlocked)
	{
		ShotTraceStart = BodyOrigin;
		ShotTraceEnd = MuzzleLocation;
	}
	else
	{
		// 카메라는 총구보다 뒤에 있으므로 그 거리만큼 여유를 줌. 실제 최종 사거리는 아래 총구 Trace의 MaxRange로 제한.
		const float CameraTraceDistance = MaxRange + FVector::Dist(ViewLocation, MuzzleLocation);

		const FVector CameraTraceEnd = ViewLocation + CameraShotDirection * CameraTraceDistance;

		FHitResult CameraHit;

		const bool bCameraHit = World->LineTraceSingleByChannel(
			CameraHit, ViewLocation, CameraTraceEnd, ECC_Visibility, TraceParams);

		const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : CameraTraceEnd;

		const FVector MuzzleToAim = AimPoint - MuzzleLocation;

		FVector ShotDirection = CameraShotDirection;

		// 카메라 뒤쪽 장애물 때문에 총구가 역방향으로 사격하지 않게 함.
		if (!MuzzleToAim.IsNearlyZero() && FVector::DotProduct(MuzzleToAim, CameraShotDirection) > 0.0f)
		{
			ShotDirection = MuzzleToAim.GetSafeNormal();
		}

		ShotTraceEnd = MuzzleLocation + ShotDirection * MaxRange;

		World->LineTraceSingleByChannel(ShotHit, ShotTraceStart, ShotTraceEnd, ECC_Visibility, TraceParams);
	}

	const bool bBlockingHit = ShotHit.bBlockingHit;

	const FVector FinalPoint = bBlockingHit ? ShotHit.ImpactPoint : ShotTraceEnd;

	const float DistanceCm = FVector::Dist(MuzzleLocation, FinalPoint);

	const float DistanceMultiplier = WeaponInstance->GetDistanceDamageMultiplier(DistanceCm);

	const float ShotDamage = FMath::Max(CombatSet->GetBaseDamage(), 0.0f) * DistanceMultiplier;

	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

	if (!DamageSpec.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DamageSpec.Data->SetSetByCallerMagnitude(DCGameplayTags::SetByCaller_Damage, ShotDamage);

	FGameplayEffectContextHandle EffectContext = DamageSpec.Data->GetContext();

	EffectContext.AddSourceObject(WeaponInstance);
	EffectContext.AddOrigin(MuzzleLocation);

	FGameplayAbilityTargetDataHandle TargetData;

	if (bBlockingHit)
	{
		TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(ShotHit));
	}

	// 비용과 쿨다운이 승인된 한 발만 실제로 처리합니다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Commit 과정에서 Ability나 장비가 취소된 경우 중단합니다.
	if (!IsActive() || !IsValid(WeaponInstance) || !WeaponInstance->IsEquipped())
	{
		if (IsActive())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
		return;
	}

	// 빗나간 탄환도 발사한 것이므로 Heat를 증가. 마지막 발사 시간도 이 함수 안에서 갱신됨.
	WeaponInstance->AddSpread();

	// 실제 발사가 승인된 뒤, 명중 여부와 관계없이 연출을 한 번 실행. 데미지용 Context와 별개로 만들어 연출 정보를 전달.
	FGameplayEffectContextHandle FireCueContext = SourceASC->MakeEffectContext();

	FireCueContext.AddInstigator(Pawn, WeaponActor);
	FireCueContext.AddSourceObject(WeaponInstance);
	FireCueContext.AddOrigin(MuzzleLocation);

	if (bBlockingHit)
	{
		FireCueContext.AddHitResult(ShotHit);
	}

	FVector FireVisualDirection = (FinalPoint - MuzzleLocation).GetSafeNormal();

	if (FireVisualDirection.IsNearlyZero())
	{
		FireVisualDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	}

	FGameplayCueParameters FireCueParameters;
	FireCueParameters.EffectContext = FireCueContext;
	FireCueParameters.Instigator = Pawn;
	FireCueParameters.EffectCauser = WeaponActor;
	FireCueParameters.SourceObject = WeaponInstance;

	// 이 발사 Cue에서는 Location을 총구 위치, Normal을 발사 연출 방향으로 사용.
	FireCueParameters.Location = MuzzleLocation;
	FireCueParameters.Normal = FireVisualDirection;

	SourceASC->ExecuteGameplayCue(DCGameplayTags::GameplayCue_Weapon_Rifle_Fire, FireCueParameters);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		DrawDebugLine(
			World, ShotTraceStart, FinalPoint, bBlockingHit ? FColor::Yellow : FColor::White, false, 0.5f, 0, 1.5f);

		if (bBlockingHit)
		{
			DrawDebugPoint(World, FinalPoint, 8.0f, FColor::Yellow, false, 1.0f);
		}
	}
#endif

	AActor* HitActor = bBlockingHit ? ShotHit.GetActor() : nullptr;

	UAbilitySystemComponent* TargetASC =
		IsValid(HitActor) ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor) : nullptr;

	if (bDrawDebugTrace)
	{
		UE_LOG(
			LogDreamCatcher, Log, TEXT("RangedFire: Time=%.3f, Spread=%.3f, Heat=%.2f, ")
			TEXT("Distance=%.1fcm, Damage=%.2f, Hit=%s"), World->GetTimeSeconds(), ShotSpreadDegrees,
			WeaponInstance->GetCurrentHeat(), DistanceCm, ShotDamage, *GetNameSafe(HitActor));
	}

	if (TargetASC)
	{
		// 이미 체력이 0인 테스트 표적에는 추가 데미지를 보내지 않음.
		const UDCHealthSet* TargetHealth = TargetASC->GetSet<UDCHealthSet>();

		if (TargetHealth && TargetHealth->GetHealth() > 0.0f)
		{
			ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, DamageSpec, TargetData);
		}
	}

	// 피격 처리 중 사망 등의 이유로 취소될 수 있음.
	if (!IsActive())
	{
		return;
	}

	// 한 발 발사 후 발사 간격 동안 Ability를 유지.
	RefireDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FireInterval);

	RefireDelayTask->OnFinish.AddDynamic(this, &ThisClass::HandleRefireDelayFinished);

	RefireDelayTask->ReadyForActivation();
}

void UDCGameplayAbility_RangedWeapon::HandleRefireDelayFinished()
{
	// WaitDelay는 콜백 이후 자체 종료됨.
	RefireDelayTask = nullptr;

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UDCGameplayAbility_RangedWeapon::EndAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (RefireDelayTask)
	{
		RefireDelayTask->EndTask();
		RefireDelayTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
