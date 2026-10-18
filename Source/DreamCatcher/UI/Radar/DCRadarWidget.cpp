// Copyright Epic Games, Inc. All Rights Reserved.
// The brush/layer painting API pattern is reused from DCLyraPerfStatWidgetBase.
// Radar projection, actor selection and lifetime handling are DreamCatcher code.
#include "UI/Radar/DCRadarWidget.h"

#include "Camera/PlayerCameraManager.h"
#include "Character/DCLyraHealthComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/DCStageRunGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Rendering/DrawElements.h"
#include "Styling/WidgetStyle.h"
#include "Teams/DCTeamSubsystem.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCRadarWidget)

UDCRadarWidget::UDCRadarWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	// External actor/camera positions are not Slate attributes. Repaint this small
	// radar each frame, matching the Lyra graph's volatile paint behavior.
	ForceVolatile(true);
	BackgroundBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	PlayerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	EnemyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	ObjectiveBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
}

TSharedRef<SWidget> UDCRadarWidget::RebuildWidget()
{
	return SNew(SBox).WidthOverride(Diameter).HeightOverride(Diameter);
}

bool UDCRadarWidget::SettingsAreValid() const
{
	return FMath::IsFinite(RangeMeters) && RangeMeters > 0.f
		&& FMath::IsFinite(PlayerIconSize) && PlayerIconSize > 0.f
		&& FMath::IsFinite(EnemyIconSize) && EnemyIconSize > 0.f
		&& FMath::IsFinite(ObjectiveIconSize) && ObjectiveIconSize > 0.f
		&& FMath::IsFinite(InnerPadding) && InnerPadding >= 0.f
		&& InnerPadding + FMath::Max3(PlayerIconSize, EnemyIconSize, ObjectiveIconSize) * UE_SQRT_2 / 2.f < Diameter / 2.f
		&& FMath::IsFinite(CandidateRefreshInterval) && CandidateRefreshInterval >= .05f
		&& FMath::IsFinite(PlayerIconRotationDegrees) && !BackgroundUVMin.ContainsNaN() && !BackgroundUVMax.ContainsNaN()
		&& BackgroundUVMin.X >= 0 && BackgroundUVMin.Y >= 0 && BackgroundUVMax.X <= 1 && BackgroundUVMax.Y <= 1
		&& BackgroundUVMin.X < BackgroundUVMax.X && BackgroundUVMin.Y < BackgroundUVMax.Y;
}

void UDCRadarWidget::RefreshDrawBrushes()
{
	BackgroundDrawBrush = BackgroundBrush;
	if (SettingsAreValid()) { BackgroundDrawBrush.SetUVRegion(FBox2f(FVector2f(BackgroundUVMin), FVector2f(BackgroundUVMax))); }
	auto TintedMaterial = [this](const FSlateBrush& Source, FSlateBrush& Destination)
	{
		Destination = Source;
		if (UMaterialInterface* Material = Cast<UMaterialInterface>(Source.GetResourceObject()))
		{
			// Runtime instances only. SimpleFontColor uses state RGBA parameters,
			// rather than VertexColor, for emissive/opacity. Pass the authored brush
			// tint through those inputs; never modify the original material asset.
			UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, this);
			Instance->SetFlags(RF_Transient);
			const FLinearColor Tint = Source.GetTint(FWidgetStyle());
			for (const TCHAR* Name : {TEXT("Base_RGBA"), TEXT("Hover_RGBA"), TEXT("Pressed_RGBA"), TEXT("Disabled_RGBA")})
			{
				Instance->SetVectorParameterValue(FName(Name), Tint);
			}
			Destination.SetResourceObject(Instance);
			Destination.TintColor = FSlateColor(FLinearColor::White);
		}
	};
	TintedMaterial(PlayerBrush, PlayerDrawBrush);
	TintedMaterial(ObjectiveBrush, ObjectiveDrawBrush);
}

void UDCRadarWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshDrawBrushes();
}

bool UDCRadarWidget::IsViewUsable() const
{
	UWorld* World = TrackedWorld.Get();
	APlayerController* PC = TrackedController.Get();
	APawn* Pawn = TrackedPawn.Get();
	if (!bTracking || !SettingsAreValid() || !IsValid(World) || World != GetWorld() || !World->IsGameWorld()
		|| World->bIsTearingDown || World->IsInSeamlessTravel() || !IsValid(PC) || PC != GetOwningPlayer()
		|| PC->IsActorBeingDestroyed() || PC->GetWorld() != World || !PC->IsLocalController()
		|| !IsValid(Pawn) || PC->GetPawn() != Pawn || Pawn->GetWorld() != World || Pawn->IsActorBeingDestroyed()
		|| !IsValid(PC->PlayerCameraManager) || Pawn->GetActorLocation().ContainsNaN()) { return false; }
	if (const ADCStageRunGameMode* Mode = World->GetAuthGameMode<ADCStageRunGameMode>())
	{
		if (Mode->HasRunEnded() || Mode->IsStageTravelPending()) { return false; }
	}
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	return IsValid(Health) && !Health->IsDeadOrDying() && FMath::IsFinite(Health->GetHealth())
		&& FMath::IsFinite(Health->GetMaxHealth()) && Health->GetHealth() > 0.f && Health->GetMaxHealth() > 0.f
		&& FMath::IsFinite(PC->PlayerCameraManager->GetCameraRotation().Yaw);
}

bool UDCRadarWidget::IsRadarActive() const { return IsViewUsable(); }

bool UDCRadarWidget::IsEnemy(const APawn* Candidate) const
{
	UWorld* World = TrackedWorld.Get();
	const APawn* Observer = TrackedPawn.Get();
	if (!IsValid(World) || !IsValid(Observer) || !IsValid(Candidate) || Candidate == Observer
		|| Candidate->GetWorld() != World || Candidate->IsActorBeingDestroyed()) { return false; }
	const UDCTeamSubsystem* Teams = World->GetSubsystem<UDCTeamSubsystem>();
	const UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Candidate);
	return Teams && Teams->CompareTeams(Observer, Candidate) == EDCTeamComparison::DifferentTeams
		&& IsValid(Health) && !Health->IsDeadOrDying() && FMath::IsFinite(Health->GetHealth())
		&& FMath::IsFinite(Health->GetMaxHealth()) && Health->GetHealth() > 0.f && Health->GetMaxHealth() > 0.f;
}

bool UDCRadarWidget::IsObjectiveValid(const USceneComponent* Component) const
{
	const AActor* Owner = IsValid(Component) ? Component->GetOwner() : nullptr;
	return IsValid(Component) && !Component->IsBeingDestroyed() && Component->GetWorld() == TrackedWorld.Get()
		&& IsValid(Owner) && !Owner->IsActorBeingDestroyed() && Owner->GetWorld() == TrackedWorld.Get()
		&& !Component->GetComponentLocation().ContainsNaN();
}

void UDCRadarWidget::SetObjectiveComponent(USceneComponent* Component)
{
	SynchronizeContext();
	Objective = IsViewUsable() && IsObjectiveValid(Component) ? Component : nullptr;
}

void UDCRadarWidget::ClearObjective() { Objective.Reset(); }

USceneComponent* UDCRadarWidget::GetObjectiveComponent() const
{
	return IsViewUsable() && IsObjectiveValid(Objective.Get()) ? Objective.Get() : nullptr;
}

bool UDCRadarWidget::ProjectOffset(const FVector& Delta, double CameraYaw, EDCRadarMarkerKind Kind, FDCRadarMarker& Out) const
{
	if (!SettingsAreValid() || Delta.ContainsNaN() || !FMath::IsFinite(CameraYaw)) { return false; }
	const double Distance = FVector2D(Delta.X, Delta.Y).Length();
	const double Range = static_cast<double>(RangeMeters) * 100.0;
	if (!FMath::IsFinite(Distance) || (Kind == EDCRadarMarkerKind::Enemy && Distance > Range)) { return false; }
	const float Size = Kind == EDCRadarMarkerKind::Enemy ? EnemyIconSize
		: Kind == EDCRadarMarkerKind::Objective ? ObjectiveIconSize : PlayerIconSize;
	const double InnerRadius = Diameter / 2.0 - InnerPadding;
	// One world-to-radar scale for all kinds; reserve the largest configured icon.
	const double CenterLimit = InnerRadius - FMath::Max3(PlayerIconSize, EnemyIconSize, ObjectiveIconSize) * UE_SQRT_2 / 2.0;
	double SinYaw, CosYaw;
	FMath::SinCos(&SinYaw, &CosYaw, FMath::DegreesToRadians(CameraYaw));
	FVector2D Offset((-SinYaw * Delta.X + CosYaw * Delta.Y) / Range * CenterLimit,
		-(CosYaw * Delta.X + SinYaw * Delta.Y) / Range * CenterLimit);
	const double ProjectedDistance = Offset.Length();
	if (!FMath::IsFinite(ProjectedDistance)) { return false; }
	Out.Kind = Kind;
	Out.SizePixels = Size;
	Out.bClamped = ProjectedDistance > CenterLimit;
	if (Out.bClamped) { Offset *= CenterLimit / ProjectedDistance; }
	Out.Center = FVector2D(Diameter / 2.f) + Offset;
	return true;
}

TArray<FDCRadarMarker> UDCRadarWidget::GetRadarMarkers() const
{
	TArray<FDCRadarMarker> Result;
	if (!IsViewUsable()) { return Result; }
	const APawn* Pawn = TrackedPawn.Get();
	const FVector Origin = Pawn->GetActorLocation();
	const double Yaw = TrackedController->PlayerCameraManager->GetCameraRotation().Yaw;
	FDCRadarMarker Player;
	Player.Center = FVector2D(Diameter / 2.f);
	Player.SizePixels = PlayerIconSize;
	Player.ActorPath = Pawn->GetPathName();
	Player.RotationDegrees = PlayerIconRotationDegrees;
	Result.Add(Player);
	for (const TWeakObjectPtr<APawn>& Weak : Candidates)
	{
		const APawn* Candidate = Weak.Get();
		FDCRadarMarker Marker;
		if (IsEnemy(Candidate) && ProjectOffset(Candidate->GetActorLocation() - Origin, Yaw, EDCRadarMarkerKind::Enemy, Marker))
		{
			Marker.ActorPath = Candidate->GetPathName();
			Result.Add(MoveTemp(Marker));
		}
	}
	if (const USceneComponent* Component = GetObjectiveComponent())
	{
		FDCRadarMarker Marker;
		if (ProjectOffset(Component->GetComponentLocation() - Origin, Yaw, EDCRadarMarkerKind::Objective, Marker))
		{
			Marker.ActorPath = Component->GetOwner()->GetPathName();
			Marker.ComponentPath = Component->GetPathName();
			Result.Add(MoveTemp(Marker));
		}
	}
	return Result;
}

void UDCRadarWidget::ClearTrackedObjects()
{
	Candidates.Reset();
	Objective.Reset();
}

void UDCRadarWidget::StopRefreshTimer()
{
	if (UWorld* World = TrackedWorld.Get()) { World->GetTimerManager().ClearTimer(RefreshTimer); }
	RefreshTimer.Invalidate();
}

void UDCRadarWidget::SynchronizeContext()
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (TrackedWorld.Get() != World || TrackedController.Get() != PC || TrackedPawn.Get() != Pawn)
	{
		StopRefreshTimer();
		ClearTrackedObjects();
		TrackedWorld = World; TrackedController = PC; TrackedPawn = Pawn;
	}
	if (!IsViewUsable()) { StopRefreshTimer(); ClearTrackedObjects(); return; }
	if (!World->GetTimerManager().TimerExists(RefreshTimer))
	{
		World->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshCandidates, CandidateRefreshInterval, true);
	}
}

void UDCRadarWidget::RefreshCandidates()
{
	SynchronizeContext();
	Candidates.Reset();
	if (!IsViewUsable()) { return; }
	// Only this short timer scans the world. Paint/tick never iterate world actors.
	for (TActorIterator<APawn> It(TrackedWorld.Get()); It; ++It)
	{
		if (IsEnemy(*It)) { Candidates.Add(*It); }
	}
}

void UDCRadarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDrawBrushes();
	bTracking = true;
	RefreshCandidates();
}

void UDCRadarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SynchronizeContext();
}

void UDCRadarWidget::NativeDestruct()
{
	bTracking = false;
	StopRefreshTimer();
	ClearTrackedObjects();
	TrackedPawn.Reset(); TrackedController.Reset(); TrackedWorld.Reset();
	BackgroundDrawBrush = FSlateBrush(); PlayerDrawBrush = FSlateBrush(); ObjectiveDrawBrush = FSlateBrush();
	Super::NativeDestruct();
}

int32 UDCRadarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	const TArray<FDCRadarMarker> Markers = GetRadarMarkers();
	const FVector2D Allocated = Geometry.GetLocalSize();
	if (Markers.IsEmpty() || Allocated.ContainsNaN() || Allocated.X <= 0 || Allocated.Y <= 0) { return Layer; }
	const float Scale = FMath::Min(Allocated.X, Allocated.Y) / Diameter;
	const FVector2D Origin = (Allocated - FVector2D(Diameter * Scale)) / 2.f;
	int32 DrawCount = 0;
	// Slate brush/layer/tint drawing follows the existing original-derived
	// DCLyraPerfStatWidgetBase.cpp paint pattern (Epic Games copyright). XY radar
	// projection/filtering are project code; the Lyra viewport indicators are untouched.
	auto Draw = [&](const FSlateBrush& Brush, FVector2D Position, float Size, float Rotation = 0.f)
	{
		if (Brush.DrawAs == ESlateBrushDrawType::NoDrawType) { return; }
		const FPaintGeometry Paint = Geometry.ToPaintGeometry(FVector2D(Size * Scale), FSlateLayoutTransform(Origin + Position * Scale));
		FSlateDrawElement::MakeRotatedBox(Elements, ++Layer, Paint, &Brush,
			bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
			FMath::DegreesToRadians(Rotation), TOptional<FVector2D>(), FSlateDrawElement::RelativeToElement,
			Style.GetColorAndOpacityTint() * Brush.GetTint(Style));
		++DrawCount;
	};
	Draw(BackgroundDrawBrush, FVector2D::ZeroVector, Diameter);
	for (EDCRadarMarkerKind Kind : {EDCRadarMarkerKind::Enemy, EDCRadarMarkerKind::Objective, EDCRadarMarkerKind::Player})
	{
		for (const FDCRadarMarker& Marker : Markers)
		{
			if (Marker.Kind != Kind) { continue; }
			const FSlateBrush& Brush = Kind == EDCRadarMarkerKind::Player ? PlayerDrawBrush
				: Kind == EDCRadarMarkerKind::Enemy ? EnemyBrush : ObjectiveDrawBrush;
			Draw(Brush, Marker.Center - FVector2D(Marker.SizePixels / 2.f), Marker.SizePixels, Marker.RotationDegrees);
		}
	}
	static const bool bPaintDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("DCRadarPaintDiagnostics"));
	if (bPaintDiagnostics && DrawCount > 0 && GFrameCounter % 4 == 0)
	{
		// Opt-in numeric evidence from the actual paint input, not reflected
		// FGeometry copies or configured Canvas slot metadata. Never substitute
		// desired/slot dimensions if the real parent geometry is unavailable.
		const FVector2D AbsoluteOrigin = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
		const FVector2D PixelSize = Geometry.LocalToAbsolute(Allocated) - AbsoluteOrigin;
		const UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetParent());
		const FGeometry ParentGeometry = Canvas ? Canvas->GetPaintSpaceGeometry() : FGeometry();
		const FVector2D ParentSize = ParentGeometry.GetLocalSize();
		const FVector2D CanvasPosition = ParentGeometry.AbsoluteToLocal(AbsoluteOrigin);
		auto Escape = [](const FString& Value) { return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")); };
		FString MarkerData;
		for (const FDCRadarMarker& Marker : Markers)
		{
			if (!MarkerData.IsEmpty()) { MarkerData += TEXT(","); }
			MarkerData += FString::Printf(TEXT("{\"kind\":%d,\"center\":[%.6f,%.6f],\"size\":%.6f,\"clamped\":%s,\"actor\":\"%s\",\"component\":\"%s\"}"),
				static_cast<int32>(Marker.Kind), Marker.Center.X, Marker.Center.Y, Marker.SizePixels,
				Marker.bClamped ? TEXT("true") : TEXT("false"), *Escape(Marker.ActorPath), *Escape(Marker.ComponentPath));
		}
		UE_LOG(LogTemp, Display, TEXT("DCRADAR_PAINT {\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"local_size\":[%.6f,%.6f],\"parent_canvas_size\":[%.6f,%.6f],\"canvas_position\":[%.6f,%.6f],\"absolute_origin\":[%.6f,%.6f],\"pixel_size\":[%.6f,%.6f],\"camera_yaw\":%.6f,\"draw_count\":%d,\"markers\":[%s]}"),
			GFrameCounter, *Escape(GetWorld()->GetPathName()), *Escape(GetPathName()), Allocated.X, Allocated.Y,
			ParentSize.X, ParentSize.Y, CanvasPosition.X, CanvasPosition.Y, AbsoluteOrigin.X, AbsoluteOrigin.Y,
			PixelSize.X, PixelSize.Y, TrackedController->PlayerCameraManager->GetCameraRotation().Yaw, DrawCount, *MarkerData);
	}
	return Layer;
}
