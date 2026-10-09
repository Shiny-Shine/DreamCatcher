// Transient contract tests for the source-based Lyra hit confirmation widget.
// No saved assets, player/controller components, active HUD, or viewport are changed.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Framework/Application/SlateApplication.h"
#include "Layout/Geometry.h"
#include "Misc/AutomationTest.h"
#include "UI/Weapons/Lyra/DCLyraHitMarkerConfirmationWidget.h"
#include "UI/Weapons/Lyra/SDCLyraHitMarkerConfirmationWidget.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

struct FDCLyraHitMarkerTestAccess
{
	static float Duration(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.HitNotifyDuration; }
	static float Opacity(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.HitNotifyOpacity; }
	static const FSlateBrush* PerHit(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.PerHitMarkerImage; }
	static const FSlateBrush* AnyHit(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.AnyHitsMarkerImage; }
	static const TMap<FGameplayTag, FSlateBrush>& Zones(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.PerHitMarkerZoneOverrideImages; }
	static bool HasColor(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.bColorAndOpacitySet; }
	static FLinearColor Color(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.ColorAndOpacity.Get(FSlateColor(FLinearColor::Transparent)).GetSpecifiedColor(); }
	static bool HasContext(const SDCLyraHitMarkerConfirmationWidget& Widget) { return Widget.MyContext.IsInitialized(); }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHitMarkerSlateArgumentsTest,
	"DreamCatcher.R5.Reticle.HitMarker.SlateArguments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHitMarkerSlateArgumentsTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate initialized"), FSlateApplication::IsInitialized())) { return false; }
	const FLocalPlayerContext EmptyContext;
	const TMap<FGameplayTag, FSlateBrush> EmptyZones;
	TSharedRef<SDCLyraHitMarkerConfirmationWidget> Defaults = SNew(SDCLyraHitMarkerConfirmationWidget, EmptyContext, EmptyZones);
	TestEqual(TEXT("Original default duration"), FDCLyraHitMarkerTestAccess::Duration(*Defaults), 0.4f);
	TestNotNull(TEXT("Original per-hit default brush"), FDCLyraHitMarkerTestAccess::PerHit(*Defaults));
	TestNull(TEXT("Original optional any-hit default"), FDCLyraHitMarkerTestAccess::AnyHit(*Defaults));
	TestFalse(TEXT("Original optional color default"), FDCLyraHitMarkerTestAccess::HasColor(*Defaults));
	TestTrue(TEXT("Original desired size"), Defaults->ComputeDesiredSize(1.0f).Equals(FVector2D(100.0, 100.0)));
	TestTrue(TEXT("Original volatility"), Defaults->ComputeVolatility());

	FSlateBrush PerHitBrush;
	FSlateBrush AnyHitBrush;
	FSlateBrush ZoneBrush;
	ZoneBrush.ImageSize = FVector2D(6.0, 8.0);
	// Synthetic key tests map forwarding without registering any global gameplay tag.
	TMap<FGameplayTag, FSlateBrush> Zones;
	Zones.Add(FGameplayTag(), ZoneBrush);
	TSharedRef<SDCLyraHitMarkerConfirmationWidget> Custom = SNew(SDCLyraHitMarkerConfirmationWidget, EmptyContext, Zones)
		.PerHitMarkerImage(&PerHitBrush).AnyHitsMarkerImage(&AnyHitBrush)
		.HitNotifyDuration(1.25f).ColorAndOpacity(FSlateColor(FLinearColor::Red));
	TestEqual(TEXT("Approved duration argument is consumed"), FDCLyraHitMarkerTestAccess::Duration(*Custom), 1.25f);
	TestTrue(TEXT("Per-hit brush forwarded"), FDCLyraHitMarkerTestAccess::PerHit(*Custom) == &PerHitBrush);
	TestTrue(TEXT("Any-hit brush forwarded"), FDCLyraHitMarkerTestAccess::AnyHit(*Custom) == &AnyHitBrush);
	TestEqual(TEXT("Zone override copied"), FDCLyraHitMarkerTestAccess::Zones(*Custom).Num(), 1);
	const FSlateBrush* CopiedZone = FDCLyraHitMarkerTestAccess::Zones(*Custom).Find(FGameplayTag());
	if (TestNotNull(TEXT("Zone brush retained"), CopiedZone))
	{
		TestTrue(TEXT("Zone brush size retained"), FVector2D(CopiedZone->ImageSize).Equals(FVector2D(6.0, 8.0)));
	}
	TestTrue(TEXT("Explicit color set"), FDCLyraHitMarkerTestAccess::HasColor(*Custom));
	TestTrue(TEXT("Explicit color forwarded"), FDCLyraHitMarkerTestAccess::Color(*Custom).Equals(FLinearColor::Red));
	TestFalse(TEXT("No local player context fabricated"), FDCLyraHitMarkerTestAccess::HasContext(*Custom));
	Custom->Tick(FGeometry(), 0.0, 0.0f);
	TestEqual(TEXT("No context means no hit opacity"), FDCLyraHitMarkerTestAccess::Opacity(*Custom), 0.0f);

	TSharedRef<SDCLyraHitMarkerConfirmationWidget> Unset = SNew(SDCLyraHitMarkerConfirmationWidget, EmptyContext, EmptyZones)
		.HitNotifyDuration(TAttribute<float>());
	TestEqual(TEXT("Unset duration keeps source default"), FDCLyraHitMarkerTestAccess::Duration(*Unset), 0.4f);
	TSharedRef<SDCLyraHitMarkerConfirmationWidget> Zero = SNew(SDCLyraHitMarkerConfirmationWidget, EmptyContext, EmptyZones)
		.HitNotifyDuration(0.0f);
	Zero->Tick(FGeometry(), 0.0, 0.0f);
	TestEqual(TEXT("Explicit zero duration preserved"), FDCLyraHitMarkerTestAccess::Duration(*Zero), 0.0f);
	TestEqual(TEXT("No-context zero duration is safe"), FDCLyraHitMarkerTestAccess::Opacity(*Zero), 0.0f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHitMarkerWidgetLifecycleTest,
	"DreamCatcher.R5.Reticle.HitMarker.WidgetLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHitMarkerWidgetLifecycleTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate initialized"), FSlateApplication::IsInitialized())) { return false; }
	TStrongObjectPtr<UDCLyraHitMarkerConfirmationWidget> Widget(NewObject<UDCLyraHitMarkerConfirmationWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestNotNull(TEXT("Concrete transient UMG widget"), Widget.Get())) { return false; }
	TestEqual(TEXT("Original UMG duration default"), Widget->HitNotifyDuration, 0.4f);
	TestTrue(TEXT("Original UMG visibility"), Widget->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Original any-hit brush starts disabled"), Widget->AnyHitsMarkerImage.DrawAs == ESlateBrushDrawType::NoDrawType);
	Widget->HitNotifyDuration = 1.25f;
	Widget->PerHitMarkerImage.ImageSize = FVector2D(10.0, 6.0);
	Widget->AnyHitsMarkerImage.DrawAs = ESlateBrushDrawType::Box;
	FSlateBrush ZoneBrush;
	ZoneBrush.ImageSize = FVector2D(7.0, 9.0);
	Widget->PerHitMarkerZoneOverrideImages.Add(FGameplayTag(), ZoneBrush);
	Widget->TakeWidget();
	TSharedPtr<SWidget> Cached = Widget->GetCachedWidget();
	if (!TestTrue(TEXT("UMG creates a cached Slate widget"), Cached.IsValid())) { return false; }
	// This fixture is the concrete native UWidget (not a UUserWidget or a Blueprint override).
	TSharedPtr<SDCLyraHitMarkerConfirmationWidget> Slate = StaticCastSharedPtr<SDCLyraHitMarkerConfirmationWidget>(Cached);
	TestEqual(TEXT("UMG forwards the custom duration at construction"), FDCLyraHitMarkerTestAccess::Duration(*Slate), 1.25f);
	TestTrue(TEXT("UMG per-hit brush identity preserved"), FDCLyraHitMarkerTestAccess::PerHit(*Slate) == &Widget->PerHitMarkerImage);
	TestTrue(TEXT("UMG any-hit brush identity preserved"), FDCLyraHitMarkerTestAccess::AnyHit(*Slate) == &Widget->AnyHitsMarkerImage);
	TestEqual(TEXT("UMG zone overrides forwarded"), FDCLyraHitMarkerTestAccess::Zones(*Slate).Num(), 1);
	TestFalse(TEXT("Standalone UMG rebuild uses empty player context"), FDCLyraHitMarkerTestAccess::HasContext(*Slate));
	Slate->Tick(FGeometry(), 0.0, 0.0f);
	TestEqual(TEXT("Standalone UMG does not display a fabricated hit"), FDCLyraHitMarkerTestAccess::Opacity(*Slate), 0.0f);
	TWeakPtr<SDCLyraHitMarkerConfirmationWidget> Previous = Slate;
	Cached.Reset();
	Slate.Reset();
	Widget->ReleaseSlateResources(true);
	TestFalse(TEXT("UMG cached Slate cleared"), Widget->GetCachedWidget().IsValid());
	TestFalse(TEXT("Detached Slate resource released"), Previous.IsValid());
	Widget->HitNotifyDuration = 0.2f;
	Widget->PerHitMarkerZoneOverrideImages.Reset();
	Widget->TakeWidget();
	Cached = Widget->GetCachedWidget();
	if (!TestTrue(TEXT("UMG can rebuild its Slate resource"), Cached.IsValid())) { return false; }
	Slate = StaticCastSharedPtr<SDCLyraHitMarkerConfirmationWidget>(Cached);
	TestEqual(TEXT("Rebuild forwards updated duration"), FDCLyraHitMarkerTestAccess::Duration(*Slate), 0.2f);
	TestEqual(TEXT("Rebuild forwards cleared zone overrides"), FDCLyraHitMarkerTestAccess::Zones(*Slate).Num(), 0);
	Cached.Reset();
	Slate.Reset();
	Widget->ReleaseSlateResources(true);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCHitMarkerDependencyRegistrationTest,
	"DreamCatcher.R5.Reticle.HitMarker.DependencyRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCHitMarkerDependencyRegistrationTest::RunTest(const FString& Parameters)
{
	const FString OriginalClass = TEXT("/Script/LyraGame.HitMarkerConfirmationWidget");
	TestEqual(TEXT("Original class redirects only to the ported UMG type"),
		FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(OriginalClass)).ToString(),
		UDCLyraHitMarkerConfirmationWidget::StaticClass()->GetPathName());
	FSoftClassPath ClassPath(OriginalClass);
	TestTrue(TEXT("Native class soft path is redirected before load"), ClassPath.FixupCoreRedirects());
	TestTrue(TEXT("Redirected native class resolves"), ClassPath.TryLoadClass<UWidget>() == UDCLyraHitMarkerConfirmationWidget::StaticClass());
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
