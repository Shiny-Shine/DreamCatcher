#include "UI/Player/DCPlayerStatusWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/DCGameplayTags.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Character/DCLyraHealthComponent.h"
#include "CommonNumericTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "Styling/WidgetStyle.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/UnrealType.h"

namespace DCPlayerStatus
{
bool Diagnostics()
{
	static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("DCPlayerStatusPaintDiagnostics"));
	return bEnabled;
}

bool FiniteColor(const FLinearColor& Color)
{
	return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G) && FMath::IsFinite(Color.B)
		&& FMath::IsFinite(Color.A) && Color.A >= 0.f && Color.A <= 1.f;
}

float SafeValue(float Value)
{
	return FMath::IsFinite(Value) ? FMath::Max(0.f, Value) : 0.f;
}

FSlateBrush SafeBrush(const FSlateBrush& Source)
{
	FSlateBrush Brush = Source;
	if (!FiniteColor(Brush.GetTint(FWidgetStyle()))) { Brush.TintColor = FSlateColor(FLinearColor::White); }
	if (Brush.ImageSize.ContainsNaN() || Brush.ImageSize.X < 0.f || Brush.ImageSize.Y < 0.f) { Brush.ImageSize = FVector2D(16.f, 16.f); }
	const FMargin& Margin = Brush.Margin;
	if (!FMath::IsFinite(Margin.Left) || !FMath::IsFinite(Margin.Top) || !FMath::IsFinite(Margin.Right) || !FMath::IsFinite(Margin.Bottom))
	{
		Brush.Margin = FMargin(.5f);
	}
	return Brush;
}

FText Number(float Value)
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	return FText::AsNumber(Value, &Options);
}

void Fill(UCanvasPanel* Parent, UWidget* Child)
{
	UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
	Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	Slot->SetOffsets(FMargin(0.f));
	Child->SetVisibility(ESlateVisibility::HitTestInvisible);
}

FString Escape(const FString& Value)
{
	return Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""))
		.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
}

FString Bounds(const FGeometry& RootGeometry, const UWidget* Widget)
{
	if (!Widget) { return TEXT("null"); }
	const FGeometry Geometry = Widget->GetPaintSpaceGeometry();
	const FVector2D Position = RootGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D Size = Geometry.GetLocalSize();
	if (Position.ContainsNaN() || Size.ContainsNaN()) { return TEXT("null"); }
	return FString::Printf(TEXT("{\"position\":[%.6f,%.6f],\"size\":[%.6f,%.6f]}"), Position.X, Position.Y, Size.X, Size.Y);
}

const TCHAR* StateName(EDCPlayerSignatureState State)
{
	switch (State)
	{
	case EDCPlayerSignatureState::Ready: return TEXT("Ready");
	case EDCPlayerSignatureState::Active: return TEXT("Active");
	case EDCPlayerSignatureState::Cooldown: return TEXT("Cooldown");
	default: return TEXT("Unavailable");
	}
}
}

bool FDCPlayerStatusStyle::IsValid() const
{
	auto Positive = [](float Value) { return FMath::IsFinite(Value) && Value > 0.f && Value <= 8192.f; };
	auto Nonnegative = [](float Value) { return FMath::IsFinite(Value) && Value >= 0.f && Value <= 8192.f; };
	return Positive(BarWidth) && Positive(ArmorHeight) && Positive(HealthHeight)
		&& Nonnegative(ArmorHealthGap) && Nonnegative(BottomMargin) && Nonnegative(ArmorSegmentGap)
		&& ArmorSegmentGap * 2.f < BarWidth && FMath::IsFinite(HorizontalOffset) && FMath::Abs(HorizontalOffset) <= 8192.f
		&& Positive(StarSize) && Nonnegative(StarGap) && FMath::IsFinite(StarIconScale) && StarIconScale > 0.f && StarIconScale <= 1.f
		&& FontSize >= 1 && FontSize <= 200 && (!TextFont.FontObject || (FMath::IsFinite(TextFont.Size) && TextFont.Size > 0.f && TextFont.Size <= 200.f))
		&& DCPlayerStatus::FiniteColor(TextColor)
		&& DCPlayerStatus::FiniteColor(ArmorColor) && DCPlayerStatus::FiniteColor(ReadyColor)
		&& DCPlayerStatus::FiniteColor(ActiveColor) && DCPlayerStatus::FiniteColor(CooldownColor)
		&& DCPlayerStatus::FiniteColor(UnavailableColor) && Nonnegative(ReadyGlowAlpha) && Nonnegative(InactiveGlowAlpha)
		&& FMath::IsFinite(RefreshInterval) && RefreshInterval >= .01f && RefreshInterval <= 1.f;
}

UDCPlayerStatusWidget::UDCPlayerStatusWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
	SetIsFocusable(false);
}

FDCPlayerStatusStyle UDCPlayerStatusWidget::ResolveStyle(const FDCPlayerStatusStyle& Source)
{
	if (Source.IsValid()) { return Source; }
	FDCPlayerStatusStyle Safe;
	Safe.ArmorBorderBrush = Source.ArmorBorderBrush; Safe.ArmorFillBrush = Source.ArmorFillBrush;
	Safe.StarBrush = Source.StarBrush; Safe.SignatureRingBrush = Source.SignatureRingBrush;
	if (Source.TextFont.FontObject && FMath::IsFinite(Source.TextFont.Size) && Source.TextFont.Size > 0.f && Source.TextFont.Size <= 200.f)
	{
		Safe.TextFont = Source.TextFont;
	}
	return Safe;
}

float UDCPlayerStatusWidget::GetSafeMaxHealth(const UDCLyraHealthComponent* HealthComponent)
{
	const float Maximum = IsValid(HealthComponent) ? HealthComponent->GetMaxHealth() : 0.f;
	// Keeps the existing Blueprint's pure getter safe during NoPawn/designer initialization.
	// Runtime presentation is hidden until it has a real health source; no attribute is written.
	return FMath::IsFinite(Maximum) && Maximum > 0.f ? Maximum : 100.f;
}

APawn* UDCPlayerStatusWidget::GetStatusPawn() const { return SourcePawn.Get(); }
UAbilitySystemComponent* UDCPlayerStatusWidget::GetStatusAbilitySystem() const { return SourceASC.Get(); }

TArray<float> UDCPlayerStatusWidget::GetArmorSegmentFills() const
{
	TArray<float> Values;
	for (const FDCPlayerArmorSegment& Segment : ArmorSegments) { Values.Add(Segment.Fraction); }
	return Values;
}

TSharedRef<SWidget> UDCPlayerStatusWidget::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
	if (!RootCanvas)
	{
		// The Blueprint supplies defaults; this native wrapper owns its passive tree.
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerStatusRoot"));
		WidgetTree->RootWidget = RootCanvas;
		BarGroup = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerStatusBars"));
		UCanvasPanelSlot* GroupSlot = RootCanvas->AddChildToCanvas(BarGroup);
		GroupSlot->SetAnchors(FAnchors(.5f, 1.f)); GroupSlot->SetAlignment(FVector2D(.5f, 1.f));
		ArmorPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerArmorBar"));
		BarGroup->AddChildToCanvas(ArmorPanel);
		ArmorSegments.SetNum(3);
		for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
		{
			FDCPlayerArmorSegment& Segment = ArmorSegments[Index];
			Segment.Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), *FString::Printf(TEXT("PlayerArmorSegment%d"), Index));
			ArmorPanel->AddChildToCanvas(Segment.Root);
			Segment.Border = WidgetTree->ConstructWidget<UImage>(); Segment.Fill = WidgetTree->ConstructWidget<UImage>();
			DCPlayerStatus::Fill(Segment.Root, Segment.Border); DCPlayerStatus::Fill(Segment.Root, Segment.Fill);
			Segment.Fill->SetRenderTransformPivot(FVector2D(0.f, .5f));
		}
		ArmorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerArmorText"));
		DCPlayerStatus::Fill(ArmorPanel, ArmorText); ArmorText->SetJustification(ETextJustify::Center);
		HealthHost = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerHealthHost"));
		BarGroup->AddChildToCanvas(HealthHost);
		SignaturePanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerSignature"));
		UCanvasPanelSlot* StarSlot = RootCanvas->AddChildToCanvas(SignaturePanel);
		StarSlot->SetAnchors(FAnchors(.5f, 1.f)); StarSlot->SetAlignment(FVector2D(0.f, .5f));
		SignatureRing = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerSignatureRing"));
		DCPlayerStatus::Fill(SignaturePanel, SignatureRing);
		SignatureStar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerSignatureStar"));
		UCanvasPanelSlot* IconSlot = SignaturePanel->AddChildToCanvas(SignatureStar);
		IconSlot->SetAnchors(FAnchors(.5f, .5f)); IconSlot->SetAlignment(FVector2D(.5f, .5f));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible); SetIsFocusable(false);
	Appearance = ResolveStyle(StatusStyle); ApplyLayout();
	BarGroup->SetVisibility(bHasContext ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SignaturePanel->SetVisibility(bHasContext ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

void UDCPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible); SetIsFocusable(false);
	bConstructed = true;
	if (WorldTearDownHandle.IsValid()) { FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle); }
	WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::HandleWorldTearDown);
	if (DCPlayerStatus::Diagnostics() && !bDiagnosticVolatile)
	{
		bSavedVolatility = bIsVolatile; bDiagnosticVolatile = true; ForceVolatile(true);
	}
	RefreshPlayerStatus();
}

void UDCPlayerStatusWidget::NativeDestruct()
{
	bConstructed = false;
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle); WorldTearDownHandle.Reset();
	if (UWorld* World = TimerWorld.Get()) { World->GetTimerManager().ClearTimer(RefreshTimer); }
	TimerWorld.Reset(); RefreshTimer.Invalidate(); TimerInterval = 0.f;
	UnbindSource(); ReleaseHealthWidget();
	for (FDCPlayerArmorSegment& Segment : ArmorSegments)
	{
		if (Segment.Border) { Segment.Border->SetBrush(FSlateBrush()); }
		if (Segment.Fill) { Segment.Fill->SetBrush(FSlateBrush()); }
		Segment.BorderMID = nullptr; Segment.FillMID = nullptr; Segment.BorderSource = nullptr; Segment.FillSource = nullptr;
		Segment.Fraction = 0.f;
	}
	if (SignatureStar) { SignatureStar->SetBrush(FSlateBrush()); }
	if (SignatureRing) { SignatureRing->SetBrush(FSlateBrush()); }
	StarMID = nullptr; RingMID = nullptr; StarSource = nullptr; RingSource = nullptr;
	if (BarGroup) { BarGroup->SetVisibility(ESlateVisibility::Collapsed); }
	if (SignaturePanel) { SignaturePanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (bDiagnosticVolatile) { ForceVolatile(bSavedVolatility); bDiagnosticVolatile = false; }
	Super::NativeDestruct();
}

void UDCPlayerStatusWidget::ApplyLayout()
{
	if (!BarGroup || !ArmorPanel || !HealthHost || !SignaturePanel) { return; }
	const float Height = Appearance.ArmorHeight + Appearance.ArmorHealthGap + Appearance.HealthHeight;
	UCanvasPanelSlot* GroupSlot = CastChecked<UCanvasPanelSlot>(BarGroup->Slot);
	GroupSlot->SetPosition(FVector2D(Appearance.HorizontalOffset, -Appearance.BottomMargin));
	GroupSlot->SetSize(FVector2D(Appearance.BarWidth, Height));
	UCanvasPanelSlot* ArmorSlot = CastChecked<UCanvasPanelSlot>(ArmorPanel->Slot);
	ArmorSlot->SetPosition(FVector2D::ZeroVector); ArmorSlot->SetSize(FVector2D(Appearance.BarWidth, Appearance.ArmorHeight));
	const float SegmentWidth = (Appearance.BarWidth - Appearance.ArmorSegmentGap * 2.f) / 3.f;
	for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
	{
		UCanvasPanelSlot* SegmentSlot = CastChecked<UCanvasPanelSlot>(ArmorSegments[Index].Root->Slot);
		SegmentSlot->SetPosition(FVector2D(Index * (SegmentWidth + Appearance.ArmorSegmentGap), 0.f));
		SegmentSlot->SetSize(FVector2D(SegmentWidth, Appearance.ArmorHeight));
	}
	FSlateFontInfo Font = Appearance.TextFont.FontObject ? Appearance.TextFont : FCoreStyle::GetDefaultFontStyle("Regular", Appearance.FontSize);
	Font.Size = Appearance.FontSize;
	ArmorText->SetFont(Font);
	ArmorText->SetColorAndOpacity(FSlateColor(Appearance.TextColor));
	UCanvasPanelSlot* HealthSlot = CastChecked<UCanvasPanelSlot>(HealthHost->Slot);
	HealthSlot->SetPosition(FVector2D(0.f, Appearance.ArmorHeight + Appearance.ArmorHealthGap));
	HealthSlot->SetSize(FVector2D(Appearance.BarWidth, Appearance.HealthHeight));
	HealthHost->SetWidthOverride(Appearance.BarWidth); HealthHost->SetHeightOverride(Appearance.HealthHeight);
	UCanvasPanelSlot* StarSlot = CastChecked<UCanvasPanelSlot>(SignaturePanel->Slot);
	// The HP/armor bar stays centered independently of the icon to its right.
	StarSlot->SetPosition(FVector2D(Appearance.HorizontalOffset + Appearance.BarWidth * .5f + Appearance.StarGap,
		-Appearance.BottomMargin - Height * .5f));
	StarSlot->SetSize(FVector2D(Appearance.StarSize, Appearance.StarSize));
	CastChecked<UCanvasPanelSlot>(SignatureStar->Slot)->SetSize(FVector2D(Appearance.StarSize * Appearance.StarIconScale));
}

void UDCPlayerStatusWidget::UnbindHealth()
{
	if (UDCLyraHealthComponent* Health = SourceHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		Health->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::HandleMaxHealthChanged);
		Health->OnArmorChanged.RemoveDynamic(this, &ThisClass::HandleArmorChanged);
		Health->OnMaxArmorChanged.RemoveDynamic(this, &ThisClass::HandleArmorChanged);
		Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
	}
	SourceHealth.Reset();
}

void UDCPlayerStatusWidget::UnbindSource()
{
	if (APlayerController* Controller = SourceController.Get())
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	UnbindHealth(); SourceController.Reset(); SourcePawn.Reset(); SourceASC.Reset();
	bHasContext = false; DisplayedHealth = 0.f; DisplayedMaxHealth = 0.f; DisplayedArmor = 0.f; DisplayedMaxArmor = 0.f;
	SignatureState = EDCPlayerSignatureState::Unavailable; SignatureMatchingSpecCount = 0;
	CooldownRemaining = 0.f; CooldownDuration = 0.f; SignatureProgress = 0.f; SignatureAbilityPath.Reset();
	bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
}

bool UDCPlayerStatusWidget::IsCurrentSource(const UDCLyraHealthComponent* Component) const
{
	const UWorld* World = GetWorld();
	APlayerController* Controller = GetOwningPlayer();
	APawn* Pawn = SourcePawn.Get();
	return bConstructed && World && World->IsGameWorld() && !World->bIsTearingDown
		&& IsValid(Component) && Component == SourceHealth.Get() && IsValid(Controller)
		&& Controller == SourceController.Get() && Controller->GetWorld() == GetWorld()
		&& IsValid(Pawn) && Pawn == Controller->GetPawn() && Pawn->GetWorld() == GetWorld()
		&& Component->GetOwner() == Pawn && Component->GetWorld() == GetWorld();
}

void UDCPlayerStatusWidget::RefreshPlayerStatus()
{
	if (!bConstructed || bRefreshing) { return; }
	TGuardValue<bool> RefreshGuard(bRefreshing, true);
	if (!StatusStyle.IsValid() && !bReportedInvalidStyle)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player status HUD uses safe layout/color defaults for an invalid style; original brushes are retained."));
		bReportedInvalidStyle = true;
	}
	if (StatusStyle.IsValid()) { bReportedInvalidStyle = false; }
	Appearance = ResolveStyle(StatusStyle); ApplyLayout();
	UWorld* CurrentWorld = GetWorld();
	if (CurrentWorld && CurrentWorld->bIsTearingDown) { HandleWorldTearDown(CurrentWorld); return; }
	if (TimerWorld.Get() != CurrentWorld || TimerInterval != Appearance.RefreshInterval)
	{
		if (UWorld* OldWorld = TimerWorld.Get()) { OldWorld->GetTimerManager().ClearTimer(RefreshTimer); }
		RefreshTimer.Invalidate(); TimerWorld = CurrentWorld; TimerInterval = Appearance.RefreshInterval;
		if (CurrentWorld)
		{
			CurrentWorld->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshPlayerStatus, TimerInterval, true);
		}
	}
	APlayerController* Controller = GetOwningPlayer();
	if (!CurrentWorld || !CurrentWorld->IsGameWorld() || !IsValid(Controller) || Controller->GetWorld() != CurrentWorld
		|| !Controller->IsLocalController() || !Controller->GetLocalPlayer())
	{
		Controller = nullptr;
	}
	if (SourceController.Get() != Controller)
	{
		UnbindSource(); ReleaseHealthWidget(); SourceController = Controller;
		if (Controller) { Controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged); }
	}
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!IsValid(Pawn) || Pawn->GetWorld() != GetWorld() || Pawn->GetController() != Controller) { Pawn = nullptr; }
	UDCLyraHealthComponent* Health = UDCLyraHealthComponent::FindHealthComponent(Pawn);
	if (SourcePawn.Get() != Pawn || SourceHealth.Get() != Health)
	{
		UnbindHealth(); SourcePawn = Pawn; SourceHealth = Health; SourceASC.Reset();
		bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
		if (Health)
		{
			Health->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
			Health->OnMaxHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleMaxHealthChanged);
			Health->OnArmorChanged.AddUniqueDynamic(this, &ThisClass::HandleArmorChanged);
			Health->OnMaxArmorChanged.AddUniqueDynamic(this, &ThisClass::HandleArmorChanged);
			Health->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleDeathStarted);
		}
	}
	SourceASC = Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn) : nullptr;
	const float HealthValue = Health ? Health->GetHealth() : 0.f;
	const float MaximumHealth = Health ? Health->GetMaxHealth() : 0.f;
	const float ArmorValue = Health ? Health->GetArmor() : 0.f;
	const float MaximumArmor = Health ? Health->GetMaxArmor() : 0.f;
	bHasContext = IsCurrentSource(Health) && FMath::IsFinite(HealthValue) && HealthValue >= 0.f
		&& FMath::IsFinite(MaximumHealth) && MaximumHealth > 0.f
		&& FMath::IsFinite(ArmorValue) && ArmorValue >= 0.f && FMath::IsFinite(MaximumArmor) && MaximumArmor >= 0.f;
	// Also catches max-HP aggregator/base changes without the custom max delegate.
	if (MaximumHealth != DisplayedMaxHealth)
	{
		bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
	}
	DisplayedHealth = DCPlayerStatus::SafeValue(HealthValue); DisplayedMaxHealth = DCPlayerStatus::SafeValue(MaximumHealth);
	DisplayedArmor = DCPlayerStatus::SafeValue(ArmorValue); DisplayedMaxArmor = DCPlayerStatus::SafeValue(MaximumArmor);
	if (BarGroup) { BarGroup->SetVisibility(bHasContext ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (SignaturePanel) { SignaturePanel->SetVisibility(bHasContext ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (bHasContext) { EnsureHealthWidget(); UpdateHealthPresentation(); }
	UpdateArmorPresentation(); ReadSignatureState(); UpdateSignaturePresentation();
}

void UDCPlayerStatusWidget::HandleWorldTearDown(UWorld* World)
{
	if (!World || (World != GetWorld() && World != TimerWorld.Get())) { return; }
	bConstructed = false;
	if (UWorld* OldWorld = TimerWorld.Get()) { OldWorld->GetTimerManager().ClearTimer(RefreshTimer); }
	TimerWorld.Reset(); RefreshTimer.Invalidate(); TimerInterval = 0.f;
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle); WorldTearDownHandle.Reset();
	UnbindSource(); ReleaseHealthWidget();
	if (BarGroup) { BarGroup->SetVisibility(ESlateVisibility::Collapsed); }
	if (SignaturePanel) { SignaturePanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (bDiagnosticVolatile) { ForceVolatile(bSavedVolatility); bDiagnosticVolatile = false; }
}

void UDCPlayerStatusWidget::EnsureHealthWidget()
{
	if (!HealthHost) { return; }
	if (OriginalHealthWidget && OriginalHealthWidget->GetClass() != HealthWidgetClass.Get()) { ReleaseHealthWidget(); }
	if (!HealthWidgetClass || HealthWidgetClass->IsChildOf(StaticClass())
		|| HealthWidgetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		ReleaseHealthWidget();
		if (!bReportedInvalidHealthClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player status HUD requires the original W_Healthbar Blueprint class; HP presentation is unavailable."));
			bReportedInvalidHealthClass = true;
		}
		return;
	}
	bReportedInvalidHealthClass = false;
	if (!OriginalHealthWidget && SourceController.IsValid())
	{
		OriginalHealthWidget = CreateWidget<UUserWidget>(SourceController.Get(), HealthWidgetClass);
		if (OriginalHealthWidget)
		{
			OriginalHealthWidget->SetIsFocusable(false); OriginalHealthWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			HealthHost->SetContent(OriginalHealthWidget);
			bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
		}
	}
	BindHealthSpawnAnimation();
}

void UDCPlayerStatusWidget::ReleaseHealthWidget()
{
	if (OriginalHealthWidget && HealthSpawnAnimation)
	{
		OriginalHealthWidget->UnbindFromAnimationFinished(HealthSpawnAnimation, SpawnFinishedDelegate);
	}
	HealthSpawnAnimation = nullptr; SpawnFinishedDelegate.Unbind();
	if (HealthHost) { HealthHost->SetContent(nullptr); }
	OriginalHealthWidget = nullptr;
}

void UDCPlayerStatusWidget::BindHealthSpawnAnimation()
{
	if (!OriginalHealthWidget || HealthSpawnAnimation) { return; }
	const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(OriginalHealthWidget->GetClass(), TEXT("OnSpawned"));
	UWidgetAnimation* Animation = Property ? Cast<UWidgetAnimation>(Property->GetObjectPropertyValue_InContainer(OriginalHealthWidget)) : nullptr;
	if (Animation)
	{
		HealthSpawnAnimation = Animation;
		SpawnFinishedDelegate.BindDynamic(this, &ThisClass::HandleHealthSpawnFinished);
		OriginalHealthWidget->BindToAnimationFinished(Animation, SpawnFinishedDelegate);
	}
}

void UDCPlayerStatusWidget::UpdateHealthPresentation()
{
	if (!OriginalHealthWidget || !IsCurrentSource(SourceHealth.Get())) { return; }
	UCommonNumericTextBlock* Numeric = Cast<UCommonNumericTextBlock>(OriginalHealthWidget->GetWidgetFromName(TEXT("HealthNumber")));
	UTextBlock* MaximumText = Cast<UTextBlock>(OriginalHealthWidget->GetWidgetFromName(TEXT("DCMaxHealthText")));
	if (USizeBox* OriginalRoot = Cast<USizeBox>(OriginalHealthWidget->GetWidgetFromName(TEXT("AnimBoundSB"))))
	{
		OriginalRoot->SetWidthOverride(Appearance.BarWidth); OriginalRoot->SetHeightOverride(Appearance.HealthHeight);
	}
	FSlateFontInfo Font = Numeric ? Numeric->GetFont() : FCoreStyle::GetDefaultFontStyle("Regular", Appearance.FontSize);
	if (Appearance.TextFont.FontObject) { Font = Appearance.TextFont; }
	Font.Size = Appearance.FontSize;
	for (UTextBlock* Label : { static_cast<UTextBlock*>(Numeric), MaximumText })
	{
		if (Label) { Label->SetFont(Font); Label->SetColorAndOpacity(FSlateColor(Appearance.TextColor)); }
	}
	if (bNeedsMaxText)
	{
		if (MaximumText)
		{
			MaximumText->SetText(FText::Format(NSLOCTEXT("DCPlayerStatus", "MaximumHealth", "/ {0}"), DCPlayerStatus::Number(DisplayedMaxHealth)));
			bNeedsMaxText = false;
		}
	}
	UDCLyraHealthComponent* Health = SourceHealth.Get();
	// Death retains the original elimination timeline. Spawn writes 0/1 itself;
	// its finish delegate reapplies only the pending normalized max correction.
	if (!Health || Health->IsDeadOrDying() || DisplayedHealth <= 0.f
		|| (HealthSpawnAnimation && OriginalHealthWidget->IsAnimationPlaying(HealthSpawnAnimation))) { return; }
	if (bNeedsHealthNumberSync)
	{
		if (Numeric)
		{
			// A max clamp can change HP without the original BP's health callback.
			// Preserve its numeric interpolation; never reset it on ordinary HP updates.
			if (!FMath::IsNearlyEqual(Numeric->GetTargetValue(), DisplayedHealth))
			{
				Numeric->InterpolateToValue(DisplayedHealth, 1.f, 4.f, 0.f);
			}
			bNeedsHealthNumberSync = false;
		}
	}
	if (!bNeedsHealthMaterialSync) { return; }
	const float Fraction = FMath::Clamp(DisplayedHealth / DisplayedMaxHealth, 0.f, 1.f);
	bool bAllMaterialsReady = true;
	for (const TCHAR* Name : { TEXT("BarBorder"), TEXT("BarFill"), TEXT("BarGlow") })
	{
		UImage* Image = Cast<UImage>(OriginalHealthWidget->GetWidgetFromName(Name));
		UMaterialInstanceDynamic* MID = Image ? Image->GetDynamicMaterial() : nullptr;
		if (!MID) { bAllMaterialsReady = false; continue; }
		MID->SetScalarParameterValue(TEXT("Health_Current"), Fraction);
		MID->SetScalarParameterValue(TEXT("Health_Updated"), Fraction);
	}
	bNeedsHealthMaterialSync = !bAllMaterialsReady;
}

void UDCPlayerStatusWidget::ApplyArmorBrush(UImage* Image, const FSlateBrush& Brush, float Fraction,
	TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& Source)
{
	FSlateBrush DrawBrush = DCPlayerStatus::SafeBrush(Brush);
	UMaterialInterface* Material = Cast<UMaterialInterface>(Brush.GetResourceObject());
	if (Material)
	{
		if (!MID || Source != Material) { MID = UMaterialInstanceDynamic::Create(Material, this); MID->SetFlags(RF_Transient); Source = Material; }
		MID->SetScalarParameterValue(TEXT("Health_Current"), Fraction); MID->SetScalarParameterValue(TEXT("Health_Updated"), Fraction);
		const FLinearColor Tint = DrawBrush.GetTint(FWidgetStyle()) * Appearance.ArmorColor;
		// Actual Lyra health material vector names, matching the existing boss HUD.
		for (const TCHAR* Name : { TEXT("Bar_Health_Fill_RGBA"), TEXT("Bar_Health_Highlight_RGBA"), TEXT("Bar_Damage_Fill_RGBA"),
			TEXT("Bar_Damage_Highlight_RGBA"), TEXT("Glow_Health_RGBA"), TEXT("Glow_Damage_RGBA"), TEXT("Outline_Damaged_RGBA"),
			TEXT("Glow_Healing_RGBA"), TEXT("Bar_Healing_Fill_RGBA"), TEXT("Bar_Healing_Highlight_RGBA"), TEXT("Outline_Healing_RGBA"),
			TEXT("Glow_Simple_Active_RGBA"), TEXT("Glow_Simple_Inactive_RGBA"), TEXT("Outline_RGB"), TEXT("Fill_RGB") })
		{
			FLinearColor Original;
			if (Material->GetVectorParameterValue(Name, Original)) { MID->SetVectorParameterValue(Name, Original * Tint); }
		}
		DrawBrush.SetResourceObject(MID); DrawBrush.TintColor = FSlateColor(FLinearColor::White);
	}
	else { MID = nullptr; Source = nullptr; DrawBrush.TintColor = FSlateColor(DrawBrush.GetTint(FWidgetStyle()) * Appearance.ArmorColor); }
	Image->SetBrush(DrawBrush);
}

void UDCPlayerStatusWidget::UpdateArmorPresentation()
{
	if (!ArmorText) { return; }
	ArmorText->SetText(DCPlayerStatus::Number(DisplayedArmor));
	const double SegmentCapacity = static_cast<double>(DisplayedMaxArmor) / 3.0;
	for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
	{
		FDCPlayerArmorSegment& Segment = ArmorSegments[Index];
		Segment.Fraction = bHasContext && SegmentCapacity > 0.0
			? static_cast<float>(FMath::Clamp((DisplayedArmor - SegmentCapacity * Index) / SegmentCapacity, 0.0, 1.0)) : 0.f;
		ApplyArmorBrush(Segment.Border, Appearance.ArmorBorderBrush, Segment.Fraction, Segment.BorderMID, Segment.BorderSource);
		ApplyArmorBrush(Segment.Fill, Appearance.ArmorFillBrush, Segment.Fraction, Segment.FillMID, Segment.FillSource);
		Segment.Fill->SetRenderScale(FVector2D(Segment.FillMID ? 1.f : Segment.Fraction, 1.f));
	}
}

void UDCPlayerStatusWidget::ReadSignatureState()
{
	SignatureState = EDCPlayerSignatureState::Unavailable; SignatureMatchingSpecCount = 0;
	CooldownRemaining = 0.f; CooldownDuration = 0.f; SignatureProgress = 0.f; SignatureAbilityPath.Reset();
	UAbilitySystemComponent* ASC = SourceASC.Get();
	if (!bHasContext || !IsValid(ASC) || ASC->GetWorld() != GetWorld() || ASC->GetAvatarActor() != SourcePawn.Get()
		|| !ASC->AbilityActorInfo.IsValid() || ASC->AbilityActorInfo->AbilitySystemComponent.Get() != ASC) { return; }
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	FScopedAbilityListLock AbilityLock(*ASC);
	const FGameplayAbilitySpec* Match = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(DCGameplayTags::InputTag_Ultimate))
		{
			++SignatureMatchingSpecCount; Match = &Spec;
		}
	}
	if (SignatureMatchingSpecCount != 1 || !Match || Match->PendingRemove || !IsValid(Match->Ability.Get())) { return; }
	UGameplayAbility* Ability = Match->GetPrimaryInstance();
	if (!Ability) { Ability = Match->Ability; }
	SignatureAbilityPath = GetPathNameSafe(Ability);
	float Remaining = 0.f, Duration = 0.f;
	Ability->GetCooldownTimeRemainingAndDuration(Match->Handle, ActorInfo, Remaining, Duration);
	if (!FMath::IsFinite(Remaining) || !FMath::IsFinite(Duration) || Remaining < 0.f || Duration < 0.f
		|| (Remaining > 0.f && Duration <= 0.f)) { return; }
	CooldownRemaining = Remaining; CooldownDuration = Duration;
	UDCLyraHealthComponent* Health = SourceHealth.Get();
	APlayerController* Controller = SourceController.Get();
	if (!Health || Health->IsDeadOrDying() || DisplayedHealth <= 0.f
		|| !Controller || Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored()
		|| ASC->HasMatchingGameplayTag(DCGameplayTags::Status_Death)
		|| ASC->HasMatchingGameplayTag(DCGameplayTags::Gameplay_AbilityInputBlocked)) { return; }
	if (Match->IsActive()) { SignatureState = EDCPlayerSignatureState::Active; SignatureProgress = 1.f; }
	else if (Remaining > 0.f)
	{
		SignatureState = EDCPlayerSignatureState::Cooldown;
		SignatureProgress = FMath::Clamp(1.f - Remaining / Duration, 0.f, 1.f);
	}
	else if (Ability->CanActivateAbility(Match->Handle, ActorInfo)) { SignatureState = EDCPlayerSignatureState::Ready; SignatureProgress = 1.f; }
}

void UDCPlayerStatusWidget::ApplySignatureBrush(UImage* Image, const FSlateBrush& Brush, const FLinearColor& Color, bool bRing,
	TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& Source)
{
	FSlateBrush DrawBrush = DCPlayerStatus::SafeBrush(Brush);
	UMaterialInterface* Material = Cast<UMaterialInterface>(Brush.GetResourceObject());
	if (Material)
	{
		if (!MID || Source != Material) { MID = UMaterialInstanceDynamic::Create(Material, this); MID->SetFlags(RF_Transient); Source = Material; }
		const FLinearColor Tint = DrawBrush.GetTint(FWidgetStyle()) * Color;
		FLinearColor Original;
		const FName ColorParameter = bRing ? FName(TEXT("BarColor")) : FName(TEXT("Inner_RGBA"));
		if (Material->GetVectorParameterValue(ColorParameter, Original)) { MID->SetVectorParameterValue(ColorParameter, Original * Tint); }
		if (bRing)
		{
			// Cached source M_UI_RadialProgress connects R/G to ArcMin/ArcMax.
			// It has no scalar parameter named Percent.
			MID->SetVectorParameterValue(TEXT("ActiveRange"), FLinearColor(0.f, SignatureProgress, 0.f, 0.f));
		}
		else
		{
			MID->SetScalarParameterValue(TEXT("EliminationGlowAlpha"),
				SignatureState == EDCPlayerSignatureState::Ready ? Appearance.ReadyGlowAlpha : Appearance.InactiveGlowAlpha);
		}
		DrawBrush.SetResourceObject(MID); DrawBrush.TintColor = FSlateColor(FLinearColor::White);
	}
	else { MID = nullptr; Source = nullptr; DrawBrush.TintColor = FSlateColor(DrawBrush.GetTint(FWidgetStyle()) * Color); }
	Image->SetBrush(DrawBrush);
}

void UDCPlayerStatusWidget::UpdateSignaturePresentation()
{
	if (!SignatureStar || !SignatureRing) { return; }
	FLinearColor Color = Appearance.UnavailableColor;
	switch (SignatureState)
	{
	case EDCPlayerSignatureState::Ready: Color = Appearance.ReadyColor; break;
	case EDCPlayerSignatureState::Active: Color = Appearance.ActiveColor; break;
	case EDCPlayerSignatureState::Cooldown: Color = Appearance.CooldownColor; break;
	default: break;
	}
	ApplySignatureBrush(SignatureStar, Appearance.StarBrush, Color, false, StarMID, StarSource);
	ApplySignatureBrush(SignatureRing, Appearance.SignatureRingBrush, Color, true, RingMID, RingSource);
}

void UDCPlayerStatusWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn) { RefreshPlayerStatus(); }

void UDCPlayerStatusWidget::HandleHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator)
{
	// Read the source, not callback arguments: a prior listener can already have
	// performed a nested attribute change by the time this listener is invoked.
	if (IsCurrentSource(Component)) { RefreshPlayerStatus(); }
}

void UDCPlayerStatusWidget::HandleMaxHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator)
{
	if (!IsCurrentSource(Component)) { return; }
	bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
	RefreshPlayerStatus();
}

void UDCPlayerStatusWidget::HandleArmorChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator)
{
	if (IsCurrentSource(Component)) { RefreshPlayerStatus(); }
}

void UDCPlayerStatusWidget::HandleDeathStarted(AActor* Actor)
{
	if (Actor == SourcePawn.Get()) { RefreshPlayerStatus(); }
}

void UDCPlayerStatusWidget::HandleHealthSpawnFinished()
{
	if (!IsCurrentSource(SourceHealth.Get())) { return; }
	bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
	RefreshPlayerStatus();
}

int32 UDCPlayerStatusWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	const FVector2D RootSize = Geometry.GetLocalSize();
	if (!DCPlayerStatus::Diagnostics() || !bHasContext || !GetWorld() || !GetWorld()->IsGameWorld()
		|| !BarGroup || !SignaturePanel || RootSize.ContainsNaN() || RootSize.X <= 0 || RootSize.Y <= 0) { return Layer; }
	FString SegmentJSON;
	for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
	{
		const FDCPlayerArmorSegment& Segment = ArmorSegments[Index];
		float MaterialCurrent = -1.f, MaterialUpdated = -1.f;
		if (Segment.FillMID)
		{
			Segment.FillMID->GetScalarParameterValue(TEXT("Health_Current"), MaterialCurrent);
			Segment.FillMID->GetScalarParameterValue(TEXT("Health_Updated"), MaterialUpdated);
		}
		if (Index) { SegmentJSON += TEXT(","); }
		SegmentJSON += FString::Printf(TEXT("{\"index\":%d,\"bounds\":%s,\"fraction\":%.6f,\"material_current\":%.6f,\"material_updated\":%.6f}"),
			Index, *DCPlayerStatus::Bounds(Geometry, Segment.Root), Segment.Fraction, MaterialCurrent, MaterialUpdated);
	}
	UTextBlock* Numeric = OriginalHealthWidget ? Cast<UTextBlock>(OriginalHealthWidget->GetWidgetFromName(TEXT("HealthNumber"))) : nullptr;
	UTextBlock* Maximum = OriginalHealthWidget ? Cast<UTextBlock>(OriginalHealthWidget->GetWidgetFromName(TEXT("DCMaxHealthText"))) : nullptr;
	FString HealthMaterialJSON;
	for (const TCHAR* Name : { TEXT("BarBorder"), TEXT("BarFill"), TEXT("BarGlow") })
	{
		UImage* Image = OriginalHealthWidget ? Cast<UImage>(OriginalHealthWidget->GetWidgetFromName(Name)) : nullptr;
		// Observe the existing brush; diagnostic paint must not allocate an HP MID.
		UMaterialInstanceDynamic* MID = Image ? Cast<UMaterialInstanceDynamic>(Image->GetBrush().GetResourceObject()) : nullptr;
		float Current = -1.f, Updated = -1.f;
		if (MID) { MID->GetScalarParameterValue(TEXT("Health_Current"), Current); MID->GetScalarParameterValue(TEXT("Health_Updated"), Updated); }
		if (!HealthMaterialJSON.IsEmpty()) { HealthMaterialJSON += TEXT(","); }
		HealthMaterialJSON += FString::Printf(TEXT("{\"name\":\"%s\",\"current\":%.6f,\"updated\":%.6f}"), Name, Current, Updated);
	}
	FLinearColor ActiveRange = FLinearColor::Transparent, RingColor = FLinearColor::Transparent, StarColor = FLinearColor::Transparent;
	if (RingMID) { RingMID->GetVectorParameterValue(TEXT("ActiveRange"), ActiveRange); RingMID->GetVectorParameterValue(TEXT("BarColor"), RingColor); }
	if (StarMID) { StarMID->GetVectorParameterValue(TEXT("Inner_RGBA"), StarColor); }
	FString JSON = FString::Printf(TEXT("{\"frame\":%llu,\"world\":\"%s\",\"widget\":\"%s\",\"world_time\":%.6f,\"controller\":\"%s\",\"pawn\":\"%s\",\"asc\":\"%s\",\"root_size\":[%.6f,%.6f],\"inherited_alpha\":%.6f"),
		GFrameCounter, *DCPlayerStatus::Escape(GetWorld()->GetPathName()), *DCPlayerStatus::Escape(GetPathName()), GetWorld()->GetTimeSeconds(),
		*DCPlayerStatus::Escape(GetPathNameSafe(SourceController.Get())), *DCPlayerStatus::Escape(GetPathNameSafe(SourcePawn.Get())),
		*DCPlayerStatus::Escape(GetPathNameSafe(SourceASC.Get())), RootSize.X, RootSize.Y, Style.GetColorAndOpacityTint().A);
	JSON += FString::Printf(TEXT(",\"bars\":%s,\"armor_bounds\":%s,\"health_bounds\":%s,\"star_bounds\":%s,\"ring_bounds\":%s"),
		*DCPlayerStatus::Bounds(Geometry, BarGroup), *DCPlayerStatus::Bounds(Geometry, ArmorPanel),
		*DCPlayerStatus::Bounds(Geometry, OriginalHealthWidget), *DCPlayerStatus::Bounds(Geometry, SignatureStar), *DCPlayerStatus::Bounds(Geometry, SignatureRing));
	JSON += FString::Printf(TEXT(",\"health\":%.6f,\"max_health\":%.6f,\"armor\":%.6f,\"max_armor\":%.6f,\"armor_text\":\"%s\",\"health_current_text\":\"%s\",\"health_max_text\":\"%s\",\"segments\":[%s]"),
		DisplayedHealth, DisplayedMaxHealth, DisplayedArmor, DisplayedMaxArmor, *DCPlayerStatus::Escape(ArmorText->GetText().ToString()),
		*DCPlayerStatus::Escape(Numeric ? Numeric->GetText().ToString() : FString()), *DCPlayerStatus::Escape(Maximum ? Maximum->GetText().ToString() : FString()), *SegmentJSON);
	JSON += FString::Printf(TEXT(",\"health_materials\":[%s],\"health_max_sync_pending\":%s"),
		*HealthMaterialJSON, bNeedsHealthMaterialSync ? TEXT("true") : TEXT("false"));
	JSON += FString::Printf(TEXT(",\"signature_state\":\"%s\",\"matching_specs\":%d,\"ability\":\"%s\",\"cooldown_remaining\":%.6f,\"cooldown_duration\":%.6f,\"progress\":%.6f,\"active_range\":[%.6f,%.6f,%.6f,%.6f],\"ring_color\":[%.6f,%.6f,%.6f,%.6f],\"star_color\":[%.6f,%.6f,%.6f,%.6f]}"),
		DCPlayerStatus::StateName(SignatureState), SignatureMatchingSpecCount, *DCPlayerStatus::Escape(SignatureAbilityPath),
		CooldownRemaining, CooldownDuration, SignatureProgress, ActiveRange.R, ActiveRange.G, ActiveRange.B, ActiveRange.A,
		RingColor.R, RingColor.G, RingColor.B, RingColor.A, StarColor.R, StarColor.G, StarColor.B, StarColor.A);
	UE_LOG(LogTemp, Display, TEXT("DCPLAYER_STATUS_PAINT %s"), *JSON);
	return Layer;
}
