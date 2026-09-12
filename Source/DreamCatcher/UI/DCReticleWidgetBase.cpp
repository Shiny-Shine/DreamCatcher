#include "UI/DCReticleWidgetBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Equipment/DCEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Weapon/DCRangedWeaponInstance.h"
#include "Weapon/DCWeaponActor.h"

void UDCReticleWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯을 화면에서 제거했다가 다시 추가하는 경우도 처리.
	UnbindEquipment();
	RefreshEquipmentBinding();
}

void UDCReticleWidgetBase::NativeDestruct()
{
	UnbindEquipment();

	Super::NativeDestruct();
}

void UDCReticleWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 사망, 재빙의, Pawn 교체 이후에도 현재 장비를 관찰.
	RefreshEquipmentBinding();

	BP_OnReticleUpdated(ComputeSpreadRadius(), ComputeSpreadAngle(), ShouldShowReticle());
}

void UDCReticleWidgetBase::RefreshEquipmentBinding()
{
	APawn* Pawn = GetOwningPlayerPawn();

	UDCEquipmentManagerComponent* NewEquipment =
		IsValid(Pawn) ? Pawn->FindComponentByClass<UDCEquipmentManagerComponent>() : nullptr;

	if (ObservedEquipment.Get() == NewEquipment)
	{
		if (!IsValid(NewEquipment))
		{
			WeaponInstance = nullptr;
		}

		return;
	}

	UnbindEquipment();

	ObservedEquipment = NewEquipment;

	if (IsValid(NewEquipment))
	{
		NewEquipment->OnEquipmentChanged.AddUniqueDynamic(this, &ThisClass::HandleEquipmentChanged);
	}

	// 위젯 생성 전에 이미 장착된 무기도 즉시 반영.
	HandleEquipmentChanged();
}

void UDCReticleWidgetBase::UnbindEquipment()
{
	if (UDCEquipmentManagerComponent* Equipment = ObservedEquipment.Get())
	{
		Equipment->OnEquipmentChanged.RemoveDynamic(this, &ThisClass::HandleEquipmentChanged);
	}

	ObservedEquipment.Reset();
	WeaponInstance = nullptr;
}

void UDCReticleWidgetBase::HandleEquipmentChanged()
{
	UDCEquipmentManagerComponent* Equipment = ObservedEquipment.Get();

	WeaponInstance =
		Equipment ? Cast<UDCRangedWeaponInstance>(Equipment->GetCurrentWeaponInstance()) : nullptr;
}

float UDCReticleWidgetBase::ComputeSpreadAngle() const
{
	if (!IsValid(WeaponInstance.Get()) || !WeaponInstance->IsEquipped())
	{
		return 0.0f;
	}

	// 이미 모든 배율을 적용한 최종 전체 각도.
	return WeaponInstance->GetCurrentSpreadAngle();
}

float UDCReticleWidgetBase::ComputeSpreadRadius() const
{
	APlayerController* PC = GetOwningPlayer();

	if (!PC || !PC->IsLocalController())
	{
		return 0.0f;
	}

	const float FullSpreadAngle = ComputeSpreadAngle();

	if (FullSpreadAngle <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 투영 계산을 위한 기준 거리. 실제 무기 사거리나 데미지에 영향 X.
	constexpr float ProjectionDistance = 10000.0f;

	const float HalfAngleRadians = FMath::DegreesToRadians(FullSpreadAngle * 0.5f);

	const float WorldRadius = FMath::Tan(HalfAngleRadians) * ProjectionDistance;

	const FVector Forward = ViewRotation.Vector();

	const FVector Up = ViewRotation.RotateVector(FVector::UpVector);

	const FVector CenterPoint = ViewLocation + Forward * ProjectionDistance;

	const FVector EdgePoint = CenterPoint + Up * WorldRadius;

	FVector2D CenterPixels;
	FVector2D EdgePixels;

	if (!PC->ProjectWorldLocationToScreen(CenterPoint, CenterPixels, true) ||
		!PC->ProjectWorldLocationToScreen(EdgePoint, EdgePixels, true))
	{
		return 0.0f;
	}

	const float RadiusPixels = static_cast<float>((EdgePixels - CenterPixels).Size());

	// 화면 픽셀을 UMG 배치에 사용하는 좌표 단위로 변환.
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);

	return RadiusPixels / ViewportScale;
}

bool UDCReticleWidgetBase::ShouldShowReticle() const
{
	APawn* Pawn = GetOwningPlayerPawn();

	if (!IsValid(Pawn) || !Pawn->IsLocallyControlled() || !IsValid(WeaponInstance.Get()) || !WeaponInstance->
		IsEquipped() || WeaponInstance->GetPawn() != Pawn || !IsValid(WeaponInstance->GetWeaponActor()))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);

	if (!ASC)
	{
		return false;
	}

	return !ASC->HasMatchingGameplayTag(DCGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(DCGameplayTags::State_Aim_Scope);
}
