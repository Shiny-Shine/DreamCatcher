#pragma once

#include "CoreMinimal.h"
#include "DCWeaponTypes.generated.h"


// 캐릭터가 무기를 들 때 사용할 애니메이션 자세 유형. 
UENUM(BlueprintType)
enum class EDCWeaponAnimationType : uint8
{
	Unarmed UMETA(DisplayName = "Unarmed"),
	Rifle UMETA(DisplayName = "Rifle"),
	Pistol UMETA(DisplayName = "Pistol"),
	Heavy UMETA(DisplayName = "Heavy")
};

USTRUCT(BlueprintType)
struct FDCWeaponHandlingProfile
{
	GENERATED_BODY()
	
	// 이 각도에서 크로스헤어가 최대로 벌어짐.
	// 실제 탄 퍼짐 최대치와 UI 표시 최대치를 분리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crosshair",
		meta=(ClampMin="0.1", Units="deg"))
	float CrosshairFullScaleSpreadDegrees = 5.0f;

	// 정지 상태의 기본 탄 퍼짐 반각.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0", Units="deg"))
	float BaseSpreadDegrees = 0.8f;

	// 최대 이동 속도일 때 추가되는 탄 퍼짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0", Units="deg"))
	float MovementSpreadDegrees = 2.4f;

	// 점프 또는 낙하 중 추가되는 탄 퍼짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0", Units="deg"))
	float AirborneSpreadDegrees = 6.0f;

	// 한 발 발사할 때 누적되는 퍼짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0", Units="deg"))
	float SpreadPerShotDegrees = 0.7f;

	// 연사로 누적될 수 있는 최대 추가 퍼짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0", Units="deg"))
	float MaxShotSpreadDegrees = 4.0f;

	// 초당 회복되는 연사 퍼짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0"))
	float SpreadRecoveryPerSecond = 0.4f;

	// 이동·조준 상태가 바뀔 때 퍼짐이 보간되는 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0"))
	float SpreadInterpSpeed = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0"))
	float ShoulderSpreadMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accuracy",
		meta=(ClampMin="0.0"))
	float ScopeSpreadMultiplier = 0.6f;

	// 한 발의 수직 반동 범위.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recoil",
		meta=(ClampMin="0.0", Units="deg"))
	float RecoilPitchMin = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recoil",
		meta=(ClampMin="0.0", Units="deg"))
	float RecoilPitchMax = 1.7f;

	// 좌우 무작위 반동의 최대 크기.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recoil",
		meta=(ClampMin="0.0", Units="deg"))
	float RecoilYawMax = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recoil",
		meta=(ClampMin="0.0"))
	float ShoulderRecoilMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recoil",
		meta=(ClampMin="0.0"))
	float ScopeRecoilMultiplier = 0.6f;
};