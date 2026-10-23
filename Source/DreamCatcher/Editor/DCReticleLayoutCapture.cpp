// One-shot, read-only layout capture from a user-started PIE session.
// All geometry is consumed natively; FGeometry never crosses the Python bridge.
#if WITH_EDITOR

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "CommonActivatableWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Layout/WidgetPath.h"
#include "Misc/DateTime.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace DCReticleLayoutCapture
{
struct FRole
{
	const TCHAR* Name;
	const TCHAR* ClassPath;
	TArray<FName> RequiredChildren;
	TArray<FName> OptionalChildren;
};

struct FCaptureContext
{
	UWorld* World = nullptr;
	FVector2D ViewportPixels = FVector2D::ZeroVector;
	bool bViewportReady = false;
	TMap<const UWidget*, bool> MeasuredWidgets;
};

bool Finite(const FVector2D& Value)
{
	return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
}

bool Positive(const FVector2D& Value)
{
	return Finite(Value) && Value.X > 0.0 && Value.Y > 0.0;
}

bool RecordGeometry(const FCaptureContext& Context, const FString& Target, const TCHAR* Source, const FGeometry& Geometry)
{
	const FVector2D Size = Geometry.GetLocalSize();
	if (!Positive(Size))
	{
		UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_GEOMETRY target=%s source=%s valid=0 local=%s reason=%s"),
			*Target, Source, *Size.ToString(), TEXT("zero_or_invalid_size"));
		return false;
	}

	// Size/2 is local to this widget. Desktop, viewport pixels and DPI-scaled
	// viewport coordinates remain separate; desired/slot sizes are never fallbacks.
	const FVector2D Center = Size * 0.5;
	const FVector2D AbsoluteOrigin = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D AbsoluteCenter = Geometry.LocalToAbsolute(Center);
	if (!Context.bViewportReady)
	{
		UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_GEOMETRY target=%s source=%s valid=0 local=%s absolute_origin=%s absolute_center=%s reason=viewport_unavailable"),
			*Target, Source, *Size.ToString(), *AbsoluteOrigin.ToString(), *AbsoluteCenter.ToString());
		return false;
	}
	FVector2D PixelOrigin, ViewportOrigin, PixelCenter, ViewportCenter;
	USlateBlueprintLibrary::LocalToViewport(Context.World, Geometry, FVector2D::ZeroVector, PixelOrigin, ViewportOrigin);
	USlateBlueprintLibrary::LocalToViewport(Context.World, Geometry, Center, PixelCenter, ViewportCenter);
	const FVector2D CenterDelta = PixelCenter - Context.ViewportPixels * 0.5;
	const bool bValid = Finite(AbsoluteOrigin) && Finite(AbsoluteCenter) && Finite(PixelOrigin)
		&& Finite(ViewportOrigin) && Finite(PixelCenter) && Finite(ViewportCenter) && Finite(CenterDelta);
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_GEOMETRY target=%s source=%s valid=%d local=%s absolute_origin=%s absolute_center=%s pixel_origin=%s pixel_center=%s viewport_origin=%s viewport_center=%s center_delta_pixels=%s"),
		*Target, Source, bValid, *Size.ToString(), *AbsoluteOrigin.ToString(), *AbsoluteCenter.ToString(),
		*PixelOrigin.ToString(), *PixelCenter.ToString(), *ViewportOrigin.ToString(), *ViewportCenter.ToString(), *CenterDelta.ToString());
	return bValid;
}

void RecordSlot(const UWidget* Widget)
{
	const UPanelSlot* PanelSlot = Widget->Slot;
	if (!PanelSlot)
	{
		UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_SLOT target=%s slot=None"), *Widget->GetPathName());
		return;
	}
	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
	{
		const FAnchors Anchors = CanvasSlot->GetAnchors();
		const FMargin Offsets = CanvasSlot->GetOffsets();
		UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_SLOT target=%s class=%s anchors_min=%s anchors_max=%s alignment=%s offsets=(%.3f,%.3f,%.3f,%.3f) auto_size=%d z_order=%d"),
			*Widget->GetPathName(), *PanelSlot->GetClass()->GetName(), *Anchors.Minimum.ToString(), *Anchors.Maximum.ToString(),
			*CanvasSlot->GetAlignment().ToString(), Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom,
			CanvasSlot->GetAutoSize(), CanvasSlot->GetZOrder());
		return;
	}
	// SafeZone uses HAlign/VAlign; the other relevant slots use the longer names.
	FString Values;
	for (const TCHAR* PropertyName : { TEXT("HorizontalAlignment"), TEXT("VerticalAlignment"), TEXT("HAlign"), TEXT("VAlign"), TEXT("Padding") })
	{
		if (const FProperty* Property = FindFProperty<FProperty>(PanelSlot->GetClass(), PropertyName))
		{
			FString Value;
			Property->ExportText_InContainer(0, Value, PanelSlot, nullptr, nullptr, PPF_None);
			Values += FString::Printf(TEXT(" %s=%s"), PropertyName, *Value);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_SLOT target=%s class=%s%s"),
		*Widget->GetPathName(), *PanelSlot->GetClass()->GetName(), *Values);
}

bool RecordWidget(FCaptureContext& Context, const UWidget* Widget)
{
	if (!IsValid(Widget)) { return false; }
	if (const bool* Previous = Context.MeasuredWidgets.Find(Widget)) { return *Previous; }
	const FString Target = Widget->GetPathName();
	const FWidgetTransform& Transform = Widget->GetRenderTransform();
	const UUserWidget* UserWidget = Cast<UUserWidget>(Widget);
	const UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(Widget);
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_WIDGET target=%s class=%s parent=%s visible=%d visibility=%d opacity=%.3f in_viewport=%d activated=%d desired_reference_only=%s translation=%s scale=%s angle=%.3f pivot=%s"),
		*Target, *Widget->GetClass()->GetPathName(), *GetPathNameSafe(Widget->GetParent()), Widget->IsVisible(),
		static_cast<int32>(Widget->GetVisibility()), Widget->GetRenderOpacity(), UserWidget ? UserWidget->IsInViewport() : false,
		Activatable ? static_cast<int32>(Activatable->IsActivated()) : -1, *Widget->GetDesiredSize().ToString(),
		*Transform.Translation.ToString(), *Transform.Scale.ToString(), Transform.Angle, *Widget->GetRenderTransformPivot().ToString());
	RecordSlot(Widget);
	RecordGeometry(Context, Target, TEXT("cached"), Widget->GetCachedGeometry());
	RecordGeometry(Context, Target, TEXT("tick"), Widget->GetTickSpaceGeometry());
	const bool bValid = RecordGeometry(Context, Target, TEXT("paint"), Widget->GetPaintSpaceGeometry());
	Context.MeasuredWidgets.Add(Widget, bValid);
	return bValid;
}

bool RecordSlatePath(const FCaptureContext& Context, const UUserWidget* Owner)
{
	// GetCachedWidget never builds a new Slate widget, unlike TakeWidget.
	const TSharedPtr<SWidget> CachedWidget = Owner->GetCachedWidget();
	FWidgetPath Path;
	const bool bFound = CachedWidget.IsValid()
		&& FSlateApplication::Get().GeneratePathToWidgetUnchecked(CachedWidget.ToSharedRef(), Path);
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_SLATE_PATH owner=%s found=%d nodes=%d"),
		*Owner->GetPathName(), bFound, Path.Widgets.Num());
	bool bLeafValid = false;
	if (bFound)
	{
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		{
			const FArrangedWidget& Entry = Path.Widgets[Index];
			const FString Target = FString::Printf(TEXT("%s/Slate[%d]/%s"), *Owner->GetPathName(), Index, *Entry.Widget->GetType().ToString());
			bLeafValid = RecordGeometry(Context, Target, TEXT("arranged_slate_path"), Entry.Geometry);
		}
	}
	return bFound && Path.IsValid() && bLeafValid;
}

void CaptureLayout()
{
	if (!IsInGameThread() || IsRunningCommandlet() || !GEditor || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("RETICLE_LAYOUT_NATIVE_REJECT requires an interactive Editor game-thread command; nothing captured."));
		return;
	}
	UWorld* World = GEditor->PlayWorld.Get();
	APlayerController* Controller = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!World || World->WorldType != EWorldType::PIE || !IsValid(Controller) || !Controller->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("RETICLE_LAYOUT_NATIVE_REJECT start Level_1 PIE manually first; this command never starts PIE."));
		return;
	}

	FCaptureContext Context;
	Context.World = World;
	int32 ViewportWidth = 0, ViewportHeight = 0;
	Controller->GetViewportSize(ViewportWidth, ViewportHeight);
	Context.ViewportPixels = FVector2D(ViewportWidth, ViewportHeight);
	const FGeometry ViewportGeometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(World);
	const FGeometry PlayerGeometry = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(Controller);
	Context.bViewportReady = Positive(Context.ViewportPixels) && Positive(ViewportGeometry.GetLocalSize())
		&& Positive(PlayerGeometry.GetLocalSize());
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_BEGIN frame=%llu utc=%s world=%s controller=%s controller_class=%s viewport_pixels=%s dpi=%.6f automatic_PIE=0 asset_saves=0"),
		GFrameCounter, *FDateTime::UtcNow().ToIso8601(), *World->GetPathName(), *Controller->GetPathName(),
		*Controller->GetClass()->GetPathName(), *Context.ViewportPixels.ToString(), UWidgetLayoutLibrary::GetViewportScale(World));
	const bool bViewportValid = RecordGeometry(Context, TEXT("Viewport"), TEXT("native_viewport"), ViewportGeometry);
	const bool bPlayerValid = RecordGeometry(Context, TEXT("PlayerScreen"), TEXT("native_player_screen"), PlayerGeometry);

	const FRole Roles[] = {
		{ TEXT("overall_layout"), TEXT("/Game/DreamCatcher/Lyra/UI/Shared/Foundation/W_OverallUILayout.W_OverallUILayout_C"),
			{ TEXT("MainOverlay"), TEXT("GameLayer_Stack") }, {} },
		{ TEXT("hud"), TEXT("/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout.W_ShooterHUDLayout_C"),
			{ TEXT("SafeZone_0"), TEXT("Overlay_0"), TEXT("CanvasPanel_0"), TEXT("ExtensionPoint_Reticle") }, {} },
		{ TEXT("host"), TEXT("/Game/DreamCatcher/Lyra/UI/Shared/W_WeaponReticleHost.W_WeaponReticleHost_C"),
			{ TEXT("VisWrapper"), TEXT("WidgetStack") }, {} },
		{ TEXT("rifle_reticle"), TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/W_Reticle_Rifle.W_Reticle_Rifle_C"),
			{ TEXT("PrimarySB"), TEXT("Overlay_0"), TEXT("CrossHairs"), TEXT("HitMarkerConfirmations"), TEXT("TargetDot") },
			{ TEXT("SBOuterReticle"), TEXT("Outer"), TEXT("EliminationMarker") } },
		{ TEXT("pistol_reticle"), TEXT("/Game/DreamCatcher/Lyra/Weapons/Pistol/W_Reticle_Pistol.W_Reticle_Pistol_C"),
			{ TEXT("PrimarySB"), TEXT("Overlay_0"), TEXT("CrossHairs"), TEXT("HitMarkerConfirmations"), TEXT("TargetDot") },
			{ TEXT("SBOuterReticle"), TEXT("Outer"), TEXT("EliminationMarker") } }
	};
	TArray<UUserWidget*> Owners[UE_ARRAY_COUNT(Roles)];
	int32 WorldCounts[UE_ARRAY_COUNT(Roles)] = {};
	int32 LiveCounts[UE_ARRAY_COUNT(Roles)] = {};
	int32 MeasuredCounts[UE_ARRAY_COUNT(Roles)] = {};
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!IsValid(Widget) || Widget->IsTemplate() || Widget->GetWorld() != World) { continue; }
		const FString ClassPath = Widget->GetClass()->GetPathName();
		for (int32 RoleIndex = 0; RoleIndex < UE_ARRAY_COUNT(Roles); ++RoleIndex)
		{
			if (ClassPath == Roles[RoleIndex].ClassPath)
			{
				++WorldCounts[RoleIndex];
				if (Widget->GetOwningPlayer() == Controller) { Owners[RoleIndex].Add(Widget); }
				break;
			}
		}
	}
	for (int32 RoleIndex = 0; RoleIndex < UE_ARRAY_COUNT(Roles); ++RoleIndex)
	{
		const FRole& Role = Roles[RoleIndex];
		for (UUserWidget* Owner : Owners[RoleIndex])
		{
			UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_OWNER role=%s owner=%s owning_player=%s"),
				Role.Name, *Owner->GetPathName(), *GetPathNameSafe(Owner->GetOwningPlayer()));
			bool bGeometryValid = RecordWidget(Context, Owner);
			bGeometryValid = RecordWidget(Context, Owner->GetRootWidget()) && bGeometryValid;
			for (const FName ChildName : Role.RequiredChildren)
			{
				UWidget* Child = Owner->GetWidgetFromName(ChildName);
				if (!Child)
				{
					UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_MISSING owner=%s child=%s"), *Owner->GetPathName(), *ChildName.ToString());
				}
				bGeometryValid = RecordWidget(Context, Child) && bGeometryValid;
			}
			for (const FName ChildName : Role.OptionalChildren) { RecordWidget(Context, Owner->GetWidgetFromName(ChildName)); }
			for (const UWidget* Parent = Owner->GetParent(); Parent; Parent = Parent->GetParent()) { RecordWidget(Context, Parent); }
			const bool bSlatePathValid = RecordSlatePath(Context, Owner);
			const UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(Owner);
			const bool bLive = Owner->IsVisible() && Owner->GetRenderOpacity() > 0.0f && bSlatePathValid
				&& (!Activatable || Activatable->IsActivated());
			LiveCounts[RoleIndex] += bLive ? 1 : 0;
			MeasuredCounts[RoleIndex] += bLive && bGeometryValid ? 1 : 0;
		}
		UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_COUNTS role=%s world_instances=%d player_instances=%d live_paths=%d valid_required_geometry=%d"),
			Role.Name, WorldCounts[RoleIndex], Owners[RoleIndex].Num(), LiveCounts[RoleIndex], MeasuredCounts[RoleIndex]);
	}
	const int32 LiveReticles = LiveCounts[3] + LiveCounts[4];
	const bool bMeasurementsAvailable = bViewportValid && bPlayerValid && MeasuredCounts[0] > 0
		&& MeasuredCounts[1] > 0 && MeasuredCounts[2] > 0 && MeasuredCounts[3] + MeasuredCounts[4] > 0;
	const bool bDuplicateLivePaths = LiveCounts[0] > 1 || LiveCounts[1] > 1 || LiveCounts[2] > 1 || LiveReticles > 1;
	// This reports availability, never a claim that the reticle/custom paint is centered.
	UE_LOG(LogTemp, Display, TEXT("RETICLE_LAYOUT_NATIVE_END frame=%llu measurements_available=%d duplicate_live_paths=%d measured_widgets=%d layout_correctness=unverified automatic_PIE=0 asset_saves=0"),
		GFrameCounter, bMeasurementsAvailable, bDuplicateLivePaths, Context.MeasuredWidgets.Num());
}

FAutoConsoleCommand CaptureCommand(
	TEXT("DC.Reticle.CaptureLayout"),
	TEXT("Read reticle geometry/Slate ancestry once from existing manual PIE. No gameplay changes or asset saves."),
	FConsoleCommandDelegate::CreateStatic(&CaptureLayout));
}

#endif // WITH_EDITOR
