// DreamCatcher-only transient tests for the source-based Lyra circumference widget.
// No saved assets, world, active HUD, or global UI/audio settings are changed.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Framework/Application/SlateApplication.h"
#include "Misc/AutomationTest.h"
#include "UI/Weapons/Lyra/DCLyraCircumferenceMarkerWidget.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

struct FDCLyraCircumferenceTestAccess
{
	static TSharedPtr<SDCLyraCircumferenceMarkerWidget> GetSlate(const UDCLyraCircumferenceMarkerWidget& Widget)
	{
		return Widget.MyMarkerWidget;
	}

	static bool IsOutsideRadius(const SDCLyraCircumferenceMarkerWidget& Widget)
	{
		return Widget.bReticleCornerOutsideSpreadRadius;
	}

	static const TArray<FDCLyraCircumferenceMarkerEntry>& GetMarkers(const SDCLyraCircumferenceMarkerWidget& Widget)
	{
		return Widget.MarkerList;
	}

	static FVector2D TransformMarkerPoint(const SDCLyraCircumferenceMarkerWidget& Widget,
		const FDCLyraCircumferenceMarkerEntry& Marker, float Radius, float HUDScale, const FVector2D& Point)
	{
		return TransformPoint(Widget.GetMarkerRenderTransform(Marker, Radius, HUDScale), Point);
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCircumferenceSlateGeometryTest,
	"DreamCatcher.R5.Reticle.Circumference.SlateGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCircumferenceSlateGeometryTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate initialized in the editor test process"), FSlateApplication::IsInitialized()))
	{
		return false;
	}

	// Synthetic non-square brush deliberately verifies the original ImageSize.X offset on BOTH axes.
	FSlateBrush Brush;
	Brush.ImageSize = FVector2D(10.0, 6.0);
	TSharedRef<SDCLyraCircumferenceMarkerWidget> Slate = SNew(SDCLyraCircumferenceMarkerWidget)
		.MarkerBrush(&Brush).Radius(24.0f);
	TestFalse(TEXT("Outside-radius has a deterministic false default"), FDCLyraCircumferenceTestAccess::IsOutsideRadius(*Slate));
	TestTrue(TEXT("Original desired size calculation"), Slate->ComputeDesiredSize(1.0f).Equals(FVector2D(68.0, 60.0)));
	TestTrue(TEXT("Original volatile drawing policy"), Slate->ComputeVolatility());
	TSharedRef<SDCLyraCircumferenceMarkerWidget> ConstructedOutside = SNew(SDCLyraCircumferenceMarkerWidget)
		.MarkerBrush(&Brush).ReticleCornerOutsideSpreadRadius(true);
	TestTrue(TEXT("Slate construction accepts OutsideRadius=true"), FDCLyraCircumferenceTestAccess::IsOutsideRadius(*ConstructedOutside));

	const FVector2D ExpectedPositions[] = { FVector2D(0, -24), FVector2D(24, 0), FVector2D(0, 24), FVector2D(-24, 0) };
	FDCLyraCircumferenceMarkerEntry Marker;
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(ExpectedPositions)); ++Index)
	{
		Marker.PositionAngle = Index * 90.0f;
		TestTrue(FString::Printf(TEXT("Original cardinal placement %d"), Index),
			FDCLyraCircumferenceTestAccess::TransformMarkerPoint(*Slate, Marker, 24.0f, 1.0f, FVector2D::ZeroVector)
			.Equals(ExpectedPositions[Index], 0.001));
	}

	Marker.PositionAngle = 0.0f;
	Slate->SetReticleCornerOutsideSpreadRadius(true);
	TestTrue(TEXT("Enabled option adds half brush WIDTH on Y, then applies HUD scale"),
		FDCLyraCircumferenceTestAccess::TransformMarkerPoint(*Slate, Marker, 24.0f, 2.0f, FVector2D::ZeroVector)
		.Equals(FVector2D(0, -58), 0.001));
	Marker.PositionAngle = 90.0f;
	TestTrue(TEXT("Enabled option adds half brush width on X"),
		FDCLyraCircumferenceTestAccess::TransformMarkerPoint(*Slate, Marker, 24.0f, 1.0f, FVector2D::ZeroVector)
		.Equals(FVector2D(29, 0), 0.001));
	Slate->SetReticleCornerOutsideSpreadRadius(false);
	TestTrue(TEXT("Disabling the option restores original center-on-radius placement"),
		FDCLyraCircumferenceTestAccess::TransformMarkerPoint(*Slate, Marker, 24.0f, 1.0f, FVector2D::ZeroVector)
		.Equals(FVector2D(24, 0), 0.001));

	Marker.PositionAngle = 0.0f;
	Marker.ImageRotationAngle = 90.0f;
	TestTrue(TEXT("Original image rotation about brush center"),
		FDCLyraCircumferenceTestAccess::TransformMarkerPoint(*Slate, Marker, 0.0f, 1.0f, FVector2D::ZeroVector)
		.Equals(FVector2D(8, -2), 0.001));
	Slate->SetRadius(12.0f);
	TestTrue(TEXT("SetRadius updates original desired size"), Slate->ComputeDesiredSize(1.0f).Equals(FVector2D(44, 36)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCircumferenceWidgetLifecycleTest,
	"DreamCatcher.R5.Reticle.Circumference.WidgetLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCircumferenceWidgetLifecycleTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate initialized in the editor test process"), FSlateApplication::IsInitialized()))
	{
		return false;
	}

	TStrongObjectPtr<UDCLyraCircumferenceMarkerWidget> Widget(NewObject<UDCLyraCircumferenceMarkerWidget>(GetTransientPackage(), NAME_None, RF_Transient));
	TestEqual(TEXT("Original UMG radius default"), Widget->Radius, 48.0f);
	TestFalse(TEXT("Explicit UMG outside-radius default"), !!Widget->bReticleCornerOutsideSpreadRadius);
	TestTrue(TEXT("Original non-hit-testable visibility"), Widget->GetVisibility() == ESlateVisibility::HitTestInvisible);
	Widget->MarkerImage.ImageSize = FVector2D(10, 6);
	Widget->SetRadius(24.0f); // Supported before Slate exists.
	Widget->bReticleCornerOutsideSpreadRadius = true;
	FDCLyraCircumferenceMarkerEntry& Marker = Widget->MarkerList.AddDefaulted_GetRef();
	Marker.PositionAngle = 45.0f;
	Marker.ImageRotationAngle = 90.0f;
	Widget->TakeWidget();
	TSharedPtr<SDCLyraCircumferenceMarkerWidget> Slate = FDCLyraCircumferenceTestAccess::GetSlate(*Widget);
	if (!TestTrue(TEXT("UMG creates the source-based Slate widget"), Slate.IsValid()))
	{
		return false;
	}
	TestTrue(TEXT("Rebuild/Synchronize forwards OutsideRadius=true"), FDCLyraCircumferenceTestAccess::IsOutsideRadius(*Slate));
	TestTrue(TEXT("Radius set before creation is forwarded"), Slate->ComputeDesiredSize(1.0f).Equals(FVector2D(68, 60)));
	const TArray<FDCLyraCircumferenceMarkerEntry>& SlateMarkers = FDCLyraCircumferenceTestAccess::GetMarkers(*Slate);
	if (TestEqual(TEXT("UMG marker list forwarded"), SlateMarkers.Num(), 1))
	{
		TestEqual(TEXT("Position angle preserved"), SlateMarkers[0].PositionAngle, 45.0f);
		TestEqual(TEXT("Image angle preserved"), SlateMarkers[0].ImageRotationAngle, 90.0f);
	}

	Widget->bReticleCornerOutsideSpreadRadius = false;
	Widget->MarkerList.Reset();
	Widget->SynchronizeProperties();
	TestFalse(TEXT("Synchronize forwards OutsideRadius=false"), FDCLyraCircumferenceTestAccess::IsOutsideRadius(*Slate));
	TestEqual(TEXT("Synchronize clears marker list"), FDCLyraCircumferenceTestAccess::GetMarkers(*Slate).Num(), 0);
	Widget->SetRadius(12.0f);
	TestTrue(TEXT("SetRadius after creation reaches Slate"), Slate->ComputeDesiredSize(1.0f).Equals(FVector2D(44, 36)));

	// Detach all external Slate owners before releasing, as when an unmounted UMG widget is rebuilt.
	TWeakPtr<SDCLyraCircumferenceMarkerWidget> PreviousSlate = Slate;
	Slate.Reset();
	Widget->ReleaseSlateResources(true);
	TestFalse(TEXT("UMG releases its Slate resource"), FDCLyraCircumferenceTestAccess::GetSlate(*Widget).IsValid());
	TestFalse(TEXT("Detached old Slate instance is released"), PreviousSlate.IsValid());
	Widget->SetRadius(30.0f);
	Widget->bReticleCornerOutsideSpreadRadius = true;
	Widget->TakeWidget();
	Slate = FDCLyraCircumferenceTestAccess::GetSlate(*Widget);
	if (!TestTrue(TEXT("Slate resource can be recreated"), Slate.IsValid()))
	{
		return false;
	}
	TestTrue(TEXT("Recreated Slate retains current OutsideRadius"), FDCLyraCircumferenceTestAccess::IsOutsideRadius(*Slate));
	TestTrue(TEXT("Recreated Slate retains current radius"), Slate->ComputeDesiredSize(1.0f).Equals(FVector2D(80, 72)));
	Slate.Reset();
	Widget->ReleaseSlateResources(true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCircumferenceDependencyRegistrationTest,
	"DreamCatcher.R5.Reticle.Circumference.DependencyRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCircumferenceDependencyRegistrationTest::RunTest(const FString& Parameters)
{
	const FString OriginalClass = TEXT("/Script/LyraGame.CircumferenceMarkerWidget");
	const FString OriginalStruct = TEXT("/Script/LyraGame.CircumferenceMarkerEntry");
	TestEqual(TEXT("Original class redirect destination"),
		FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(OriginalClass)).ToString(),
		UDCLyraCircumferenceMarkerWidget::StaticClass()->GetPathName());
	TestEqual(TEXT("Original struct redirect destination"),
		FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Struct, FCoreRedirectObjectName(OriginalStruct)).ToString(),
		FDCLyraCircumferenceMarkerEntry::StaticStruct()->GetPathName());
	// Fix up native paths first: a raw TryLoad attempts the absent LyraGame package before its fallback.
	FSoftClassPath ClassPath(OriginalClass);
	FSoftObjectPath StructPath(OriginalStruct);
	TestTrue(TEXT("Native class soft path is redirected"), ClassPath.FixupCoreRedirects());
	TestTrue(TEXT("Native struct soft path is redirected"), StructPath.FixupCoreRedirects());
	TestTrue(TEXT("Redirected class path resolves to ported UMG type"),
		ClassPath.TryLoadClass<UWidget>() == UDCLyraCircumferenceMarkerWidget::StaticClass());
	TestTrue(TEXT("Redirected struct path resolves to ported entry type"),
		StructPath.TryLoad() == FDCLyraCircumferenceMarkerEntry::StaticStruct());
	// Run without a process-only -EnablePlugins=Spatialization override to verify project registration.
	TestNotNull(TEXT("Project-enabled Spatialization source-settings type is loadable"),
		FSoftClassPath(TEXT("/Script/Spatialization.ITDSpatializationSourceSettings")).TryLoadClass<UObject>());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
