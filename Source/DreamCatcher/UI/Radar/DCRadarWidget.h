#pragma once

#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "DCRadarWidget.generated.h"

class APawn;
class APlayerController;
class USceneComponent;

UENUM(BlueprintType)
enum class EDCRadarMarkerKind : uint8
{
	Player,
	Enemy,
	Objective
};

/** Local coordinates in the 240 x 240 radar, before viewport DPI is applied. */
USTRUCT(BlueprintType)
struct DREAMCATCHER_API FDCRadarMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Radar")
	EDCRadarMarkerKind Kind = EDCRadarMarkerKind::Player;
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	float SizePixels = 24.f;
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	bool bClamped = false;
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	float RotationDegrees = 0.f;
	/** Identity only; displaying a marker never retains the target actor/component. */
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	FString ActorPath;
	UPROPERTY(BlueprintReadOnly, Category="Radar")
	FString ComponentPath;
};

/** Passive radar. Reuses Lyra teams/health, with no world-indicator registration. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCRadarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCRadarWidget(const FObjectInitializer& ObjectInitializer);

	/** Null/invalid/foreign-world targets clear the previous objective. */
	UFUNCTION(BlueprintCallable, Category="Radar")
	void SetObjectiveComponent(USceneComponent* Component);
	UFUNCTION(BlueprintCallable, Category="Radar")
	void ClearObjective();
	UFUNCTION(BlueprintPure, Category="Radar")
	USceneComponent* GetObjectiveComponent() const;
	/** NativePaint consumes this same read-only projection of real cached Pawns. */
	UFUNCTION(BlueprintPure, Category="Radar")
	TArray<FDCRadarMarker> GetRadarMarkers() const;
	UFUNCTION(BlueprintPure, Category="Radar")
	bool IsRadarActive() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar", meta=(ClampMin="0.01", Units="m"))
	float RangeMeters = 50.f;
	/** Additional space inside the circle; icon half-diagonal is reserved separately. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar", meta=(ClampMin="0.0"))
	float InnerPadding = 4.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar", meta=(ClampMin="0.05", Units="s"))
	float CandidateRefreshInterval = .2f;
	/** Icon sizes are radar-local pixels before viewport DPI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art", meta=(ClampMin="0.01"))
	float PlayerIconSize = 24.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art", meta=(ClampMin="0.01"))
	float EnemyIconSize = 12.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art", meta=(ClampMin="0.01"))
	float ObjectiveIconSize = 24.f;
	/** Original SimpleArrow material points left; positive screen rotation makes it point up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	float PlayerIconRotationDegrees = 90.f;
	/** Optional transparent texture padding crop; native defaults use the full texture. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FVector2D BackgroundUVMin = FVector2D::ZeroVector;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FVector2D BackgroundUVMax = FVector2D(1.f, 1.f);
	/** Assigned by the project Widget Blueprint; native defaults draw no artwork. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FSlateBrush BackgroundBrush;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FSlateBrush PlayerBrush;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FSlateBrush EnemyBrush;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radar|Art")
	FSlateBrush ObjectiveBrush;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	static constexpr float Diameter = 240.f;
	bool SettingsAreValid() const;
	bool IsViewUsable() const;
	bool IsEnemy(const APawn* Candidate) const;
	bool IsObjectiveValid(const USceneComponent* Component) const;
	bool ProjectOffset(const FVector& Delta, double CameraYaw, EDCRadarMarkerKind Kind, FDCRadarMarker& Out) const;
	void SynchronizeContext();
	void RefreshCandidates();
	void StopRefreshTimer();
	void ClearTrackedObjects();
	void RefreshDrawBrushes();
	UPROPERTY(Transient)
	FSlateBrush BackgroundDrawBrush;
	UPROPERTY(Transient)
	FSlateBrush PlayerDrawBrush;
	UPROPERTY(Transient)
	FSlateBrush ObjectiveDrawBrush;

	TWeakObjectPtr<UWorld> TrackedWorld;
	TWeakObjectPtr<APlayerController> TrackedController;
	TWeakObjectPtr<APawn> TrackedPawn;
	TWeakObjectPtr<USceneComponent> Objective;
	TArray<TWeakObjectPtr<APawn>> Candidates;
	FTimerHandle RefreshTimer;
	bool bTracking = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCRadarTestAccess;
#endif
};
