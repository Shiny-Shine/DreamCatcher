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
#include "Styling/WidgetStyle.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/UnrealType.h"

namespace DCPlayerStatus
{
// Saved W_Healthbar OnDamaged/OnHealed curves use a 60,000 Hz tick resolution.
constexpr float ArmorTransitionSeconds = 14400.f / 60000.f;
constexpr float ArmorDamageHoldSeconds = 24000.f / 60000.f;
constexpr float ArmorAnimationSeconds = 48000.f / 60000.f;
constexpr float ArmorHealingFadeInSeconds = 600.f / 60000.f;

float ArmorCurve(float Time, float Start, float End)
{
	// These material channels are cubic keys with zero tangents in W_Healthbar.
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp((Time - Start) / (End - Start), 0.f, 1.f));
}

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

struct FHealthPawnParameters { APawn* OldPawn; APawn* NewPawn; };

UFunction* HealthPawnChangedFunction(UUserWidget* Widget)
{
	UFunction* Function = Widget ? Widget->FindFunction(TEXT("OnPosessedPawnChanged")) : nullptr;
	const FObjectPropertyBase* OldProperty = Function ? FindFProperty<FObjectPropertyBase>(Function, TEXT("OldPawn")) : nullptr;
	const FObjectPropertyBase* NewProperty = Function ? FindFProperty<FObjectPropertyBase>(Function, TEXT("NewPawn")) : nullptr;
	return Function && Function->NumParms == 2 && Function->ParmsSize == sizeof(FHealthPawnParameters) && OldProperty && NewProperty
		&& OldProperty->PropertyClass == APawn::StaticClass() && NewProperty->PropertyClass == APawn::StaticClass()
		&& OldProperty->HasAnyPropertyFlags(CPF_Parm) && NewProperty->HasAnyPropertyFlags(CPF_Parm)
		&& !OldProperty->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm) && !NewProperty->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm)
		&& OldProperty->GetOffset_ForInternal() == STRUCT_OFFSET(FHealthPawnParameters, OldPawn)
		&& NewProperty->GetOffset_ForInternal() == STRUCT_OFFSET(FHealthPawnParameters, NewPawn) ? Function : nullptr;
}
}

bool FDCPlayerStatusStyle::IsValid() const
{
	auto Nonnegative = [](float Value) { return FMath::IsFinite(Value) && Value >= 0.f && Value <= 8192.f; };
	return DCPlayerStatus::FiniteColor(ReadyColor)
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
	return Source.IsValid() ? Source : FDCPlayerStatusStyle();
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

bool UDCPlayerStatusWidget::BindDesignerWidgets()
{
	RootCanvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	BarGroup = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("PlayerStatusBars")));
	ArmorPanel = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("PlayerArmorBar")));
	ArmorText = Cast<UTextBlock>(GetWidgetFromName(TEXT("ShieldNumber")));
	HealthHost = Cast<USizeBox>(GetWidgetFromName(TEXT("PlayerHealthHost")));
	SignaturePanel = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("PlayerSignature")));
	SignatureStar = Cast<UImage>(GetWidgetFromName(TEXT("PlayerSignatureStar")));
	SignatureRing = Cast<UImage>(GetWidgetFromName(TEXT("PlayerSignatureRing")));
	const TArray<FDCPlayerArmorSegment> Previous = ArmorSegments;
	ArmorSegments.SetNum(3);
	bool bArmorBindingsChanged = Previous.Num() != ArmorSegments.Num();
	bool bValid = RootCanvas && BarGroup && ArmorPanel && ArmorText && HealthHost
		&& SignaturePanel && SignatureStar && SignatureRing;
	for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
	{
		FDCPlayerArmorSegment& Segment = ArmorSegments[Index];
		Segment.Root = Cast<UCanvasPanel>(GetWidgetFromName(*FString::Printf(TEXT("PlayerArmorSegment%d"), Index)));
		Segment.Border = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("PlayerArmorBorder%d"), Index)));
		Segment.Fill = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("PlayerArmorFill%d"), Index)));
		bValid &= Segment.Root && Segment.Border && Segment.Fill;
		if (!Segment.Root || !Segment.Border || !Segment.Fill) { continue; }
		const FDCPlayerArmorSegment* Old = Previous.IsValidIndex(Index) ? &Previous[Index] : nullptr;
		bArmorBindingsChanged |= !Old || Old->Root != Segment.Root || Old->Fill != Segment.Fill;
		// A rebuild/synchronization may encounter our own live MID, not an authored brush.
		if (!Old || Segment.Border != Old->Border || !Old->BorderMID || Segment.Border->GetBrush().GetResourceObject() != Old->BorderMID)
		{
			Segment.AuthoredBorderBrush = Segment.Border->GetBrush();
			Segment.BorderMID = nullptr; Segment.BorderSource = nullptr;
		}
		if (!Old || Segment.Fill != Old->Fill || !Old->FillMID || Segment.Fill->GetBrush().GetResourceObject() != Old->FillMID)
		{
			Segment.AuthoredFillBrush = Segment.Fill->GetBrush();
			// Do not capture the fraction-scaled texture fallback a second time.
			if (!Old || Segment.Fill != Old->Fill) { Segment.AuthoredFillScale = Segment.Fill->GetRenderTransform().Scale; }
			Segment.FillMID = nullptr; Segment.FillSource = nullptr;
		}
	}
	if (bArmorBindingsChanged) { ResetArmorAnimation(); }
	if (SignatureStar && (!StarMID || SignatureStar->GetBrush().GetResourceObject() != StarMID)) { AuthoredStarBrush = SignatureStar->GetBrush(); }
	if (SignatureRing && (!RingMID || SignatureRing->GetBrush().GetResourceObject() != RingMID)) { AuthoredRingBrush = SignatureRing->GetBrush(); }
	UUserWidget* AuthoredHealth = HealthHost ? Cast<UUserWidget>(HealthHost->GetContent()) : nullptr;
	bValid &= AuthoredHealth && HealthWidgetClass && AuthoredHealth->GetClass() == HealthWidgetClass.Get();
	if (OriginalHealthWidget != AuthoredHealth)
	{
		if (OriginalHealthWidget) { ReleaseHealthWidget(); }
		OriginalHealthWidget = AuthoredHealth; bAuthoredHealthObserved = false; bAuthoredHealthReleased = false;
	}
	bDesignerTreeReady = bValid;
	if (!bValid && !bReportedInvalidDesignerTree)
	{
		UE_LOG(LogTemp, Error, TEXT("Player status Designer hierarchy is incomplete or has incorrect widget types/HealthWidgetClass. The authored tree was preserved; repair its named bindings before playing."));
	}
	bReportedInvalidDesignerTree = !bValid;
	return bValid;
}

void UDCPlayerStatusWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (IsDesignTime() && !IsTemplate() && BindDesignerWidgets()) { ApplyDesignerPreview(); }
}

void UDCPlayerStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime() && !IsTemplate() && BindDesignerWidgets()) { ApplyDesignerPreview(); }
}

void UDCPlayerStatusWidget::ApplyDesignerPreview()
{
	if (!IsDesignTime() || !bDesignerTreeReady || IsTemplate()) { return; }
	TGuardValue<bool> PreviewGuard(bPreviewing, true);
	Appearance = ResolveStyle(StatusStyle);
	DisplayedMaxHealth = FMath::Max(1.f, DCPlayerStatus::SafeValue(PreviewMaxHealth));
	DisplayedHealth = FMath::Clamp(DCPlayerStatus::SafeValue(PreviewHealth), 0.f, DisplayedMaxHealth);
	DisplayedMaxArmor = DCPlayerStatus::SafeValue(PreviewMaxArmor);
	DisplayedArmor = FMath::Clamp(DCPlayerStatus::SafeValue(PreviewArmor), 0.f, DisplayedMaxArmor);
	BarGroup->SetVisibility(ESlateVisibility::HitTestInvisible);
	SignaturePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateArmorPresentation();
	if (UCommonNumericTextBlock* Numeric = Cast<UCommonNumericTextBlock>(OriginalHealthWidget->GetWidgetFromName(TEXT("HealthNumber"))))
	{
		Numeric->SetCurrentValue(DisplayedHealth);
	}
	const float Fraction = DisplayedHealth / DisplayedMaxHealth;
	for (const TCHAR* Name : { TEXT("BarBorder"), TEXT("BarFill"), TEXT("BarGlow") })
	{
		UImage* Image = Cast<UImage>(OriginalHealthWidget->GetWidgetFromName(Name));
		UMaterialInstanceDynamic* MID = Image ? Image->GetDynamicMaterial() : nullptr;
		if (MID)
		{
			MID->SetFlags(RF_Transient);
			MID->SetScalarParameterValue(TEXT("Health_Current"), Fraction);
			MID->SetScalarParameterValue(TEXT("Health_Updated"), Fraction);
		}
	}
	// Preview is deliberately not a gameplay context and does not bind a source/timer.
	SignatureState = EDCPlayerSignatureState::Unavailable;
	UpdateSignaturePresentation();
}

TSharedRef<SWidget> UDCPlayerStatusWidget::RebuildWidget()
{
	BindDesignerWidgets();
	return Super::RebuildWidget();
}

void UDCPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsDesignTime() || !bDesignerTreeReady) { return; }
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

void UDCPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceArmorAnimation(InDeltaTime);
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
		if (Segment.Border) { Segment.Border->SetBrush(Segment.AuthoredBorderBrush); }
		if (Segment.Fill)
		{
			Segment.Fill->SetBrush(Segment.AuthoredFillBrush);
			Segment.Fill->SetRenderScale(Segment.AuthoredFillScale);
		}
		Segment.BorderMID = nullptr; Segment.FillMID = nullptr; Segment.BorderSource = nullptr; Segment.FillSource = nullptr;
		Segment.Fraction = 0.f;
	}
	if (SignatureStar) { SignatureStar->SetBrush(AuthoredStarBrush); }
	if (SignatureRing) { SignatureRing->SetBrush(AuthoredRingBrush); }
	StarMID = nullptr; RingMID = nullptr; StarSource = nullptr; RingSource = nullptr;
	if (BarGroup) { BarGroup->SetVisibility(ESlateVisibility::Collapsed); }
	if (SignaturePanel) { SignaturePanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (bDiagnosticVolatile) { ForceVolatile(bSavedVolatility); bDiagnosticVolatile = false; }
	Super::NativeDestruct();
}

void UDCPlayerStatusWidget::UnbindHealth()
{
	ResetArmorAnimation();
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
	if (IsDesignTime()) { ApplyDesignerPreview(); return; }
	if (!bDesignerTreeReady) { return; }
	if (!bConstructed || bRefreshing) { return; }
	TGuardValue<bool> RefreshGuard(bRefreshing, true);
	if (!StatusStyle.IsValid() && !bReportedInvalidStyle)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player status HUD uses safe state-color and refresh defaults for an invalid style."));
		bReportedInvalidStyle = true;
	}
	if (StatusStyle.IsValid()) { bReportedInvalidStyle = false; }
	Appearance = ResolveStyle(StatusStyle);
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
		UnbindSource();
		// The saved child's initial Construct has already bound its controller. Do not
		// cancel/restart its spawn animation just because this wrapper observes it first.
		if (bAuthoredHealthObserved || bAuthoredHealthReleased) { ReleaseHealthWidget(); }
		SourceController = Controller;
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
	// Track the authored child's controller even before the first possessed Pawn.
	EnsureHealthWidget();
	if (bHasContext) { UpdateHealthPresentation(); }
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
	// The saved child belongs to the Designer hierarchy, including after detach/reentry.
	OriginalHealthWidget = Cast<UUserWidget>(HealthHost->GetContent());
	if (!OriginalHealthWidget || !HealthWidgetClass || OriginalHealthWidget->GetClass() != HealthWidgetClass.Get()) { return; }
	APlayerController* Controller = GetOwningPlayer();
	const bool bOwnerChanged = OriginalHealthWidget->GetOwningPlayer() != Controller;
	if (bOwnerChanged) { OriginalHealthWidget->SetPlayerContext(GetPlayerContext()); }
	if (Controller && DCPlayerStatus::HealthPawnChangedFunction(OriginalHealthWidget))
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(OriginalHealthWidget, TEXT("OnPosessedPawnChanged"));
		Controller->OnPossessedPawnChanged.AddUnique(Delegate);
		if (bOwnerChanged || bAuthoredHealthReleased) { NotifyOriginalHealthPawnChanged(AuthoredHealthPawn.Get(), Controller->GetPawn()); }
		AuthoredHealthController = Controller; AuthoredHealthPawn = Controller->GetPawn();
		bAuthoredHealthReleased = false; bAuthoredHealthObserved = true;
	}
	BindHealthSpawnAnimation();
}

void UDCPlayerStatusWidget::ReleaseHealthWidget()
{
	// A nested BP can Construct before the wrapper sees its first Pawn. In that
	// case use its actual owner; after observation never replace an expired owner
	// with a new controller resolved through a shared LocalPlayer context.
	APlayerController* Controller = AuthoredHealthController.Get();
	if (!bAuthoredHealthObserved && !bAuthoredHealthReleased && OriginalHealthWidget) { Controller = OriginalHealthWidget->GetOwningPlayer(); }
	if (OriginalHealthWidget && DCPlayerStatus::HealthPawnChangedFunction(OriginalHealthWidget))
	{
		if (Controller)
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(OriginalHealthWidget, TEXT("OnPosessedPawnChanged"));
			Controller->OnPossessedPawnChanged.Remove(Delegate);
		}
		APawn* OldPawn = AuthoredHealthPawn.Get();
		if (!bAuthoredHealthObserved && Controller) { OldPawn = Controller->GetPawn(); }
		if (!bAuthoredHealthReleased) { NotifyOriginalHealthPawnChanged(OldPawn, nullptr); }
	}
	// Always record release, including an expired controller or a NoPawn child.
	AuthoredHealthController.Reset(); AuthoredHealthPawn.Reset(); bAuthoredHealthReleased = true;
	if (OriginalHealthWidget && HealthSpawnAnimation)
	{
		OriginalHealthWidget->UnbindFromAnimationFinished(HealthSpawnAnimation, SpawnFinishedDelegate);
	}
	HealthSpawnAnimation = nullptr; SpawnFinishedDelegate.Unbind();
	bNeedsMaxText = true; bNeedsHealthMaterialSync = true; bNeedsHealthNumberSync = true;
}

void UDCPlayerStatusWidget::NotifyOriginalHealthPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (!OriginalHealthWidget || IsDesignTime()) { return; }
	// This is the existing Blueprint event, not a replacement health/animation path.
	if (UFunction* Function = DCPlayerStatus::HealthPawnChangedFunction(OriginalHealthWidget))
	{
		DCPlayerStatus::FHealthPawnParameters Parameters{OldPawn, NewPawn};
		OriginalHealthWidget->ProcessEvent(Function, &Parameters);
	}
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

void UDCPlayerStatusWidget::ApplyArmorBrush(UImage* Image, const FSlateBrush& Brush,
	TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& Source)
{
	FSlateBrush DrawBrush = DCPlayerStatus::SafeBrush(Brush);
	UMaterialInterface* Material = Cast<UMaterialInterface>(Brush.GetResourceObject());
	if (Material)
	{
		if (!MID || Source != Material) { MID = UMaterialInstanceDynamic::Create(Material, this); MID->SetFlags(RF_Transient); Source = Material; }
		const FLinearColor Tint = DrawBrush.GetTint(FWidgetStyle());
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
	else { MID = nullptr; Source = nullptr; DrawBrush.TintColor = FSlateColor(DrawBrush.GetTint(FWidgetStyle())); }
	Image->SetBrush(DrawBrush);
}

void UDCPlayerStatusWidget::ResetArmorAnimation()
{
	bArmorPresentationInitialized = false;
	PresentedMaxArmor = 0.f;
	for (FDCPlayerArmorSegment& Segment : ArmorSegments)
	{
		Segment.Fraction = 0.f;
		Segment.PreviousFraction = 0.f;
		Segment.AnimationElapsed = DCPlayerStatus::ArmorAnimationSeconds;
		Segment.bAnimating = false;
		Segment.bHealing = false;
	}
}

void UDCPlayerStatusWidget::ApplyArmorAnimation(const FDCPlayerArmorSegment& Segment)
{
	const float Current = Segment.bAnimating ? Segment.PreviousFraction : Segment.Fraction;
	const float Transition = Segment.bAnimating
		? DCPlayerStatus::ArmorCurve(Segment.AnimationElapsed, 0.f, DCPlayerStatus::ArmorTransitionSeconds) : 1.f;
	float Fade = 1.f;
	if (Segment.bAnimating)
	{
		const float DamageFade = DCPlayerStatus::ArmorCurve(Segment.AnimationElapsed,
			DCPlayerStatus::ArmorDamageHoldSeconds, DCPlayerStatus::ArmorAnimationSeconds);
		// Match the saved BarFill healing pulse as well as the damage fade.
		Fade = Segment.bHealing
			? FMath::Min(DCPlayerStatus::ArmorCurve(Segment.AnimationElapsed, 0.f, DCPlayerStatus::ArmorHealingFadeInSeconds), 1.f - DamageFade)
			: DamageFade;
	}
	for (UMaterialInstanceDynamic* MID : { Segment.BorderMID.Get(), Segment.FillMID.Get() })
	{
		if (!MID) { continue; }
		MID->SetScalarParameterValue(TEXT("Health_Current"), Current);
		MID->SetScalarParameterValue(TEXT("Health_Updated"), Segment.Fraction);
		MID->SetScalarParameterValue(TEXT("Animate_HealthNewToUpdated"), Transition);
		MID->SetScalarParameterValue(TEXT("Animate_UpdatedFade"), Fade);
		MID->SetScalarParameterValue(TEXT("DamageOrHealing"), Segment.bAnimating && Segment.bHealing ? 1.f : 0.f);
	}
}

void UDCPlayerStatusWidget::AdvanceArmorAnimation(float DeltaTime)
{
	if (!bConstructed || !bHasContext || IsDesignTime() || !bArmorPresentationInitialized
		|| !bDesignerTreeReady || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) { return; }
	for (FDCPlayerArmorSegment& Segment : ArmorSegments)
	{
		if (!Segment.bAnimating) { continue; }
		Segment.AnimationElapsed = FMath::Min(DCPlayerStatus::ArmorAnimationSeconds,
			Segment.AnimationElapsed + FMath::Min(DeltaTime, DCPlayerStatus::ArmorAnimationSeconds));
		if (Segment.AnimationElapsed >= DCPlayerStatus::ArmorAnimationSeconds)
		{
			Segment.PreviousFraction = Segment.Fraction;
			Segment.bAnimating = false;
			Segment.bHealing = false;
		}
		ApplyArmorAnimation(Segment);
	}
}

void UDCPlayerStatusWidget::UpdateArmorPresentation()
{
	if (!ArmorText) { return; }
	ArmorText->SetText(DCPlayerStatus::Number(DisplayedArmor));
	const double SegmentCapacity = static_cast<double>(DisplayedMaxArmor) / 3.0;
	const bool bSnap = !bArmorPresentationInitialized || bPreviewing || IsDesignTime() || !bConstructed || !bHasContext
		|| !FMath::IsNearlyEqual(PresentedMaxArmor, DisplayedMaxArmor);
	for (int32 Index = 0; Index < ArmorSegments.Num(); ++Index)
	{
		FDCPlayerArmorSegment& Segment = ArmorSegments[Index];
		if (!Segment.Border || !Segment.Fill) { continue; }
		const float Fraction = (bHasContext || bPreviewing) && SegmentCapacity > 0.0
			? static_cast<float>(FMath::Clamp((DisplayedArmor - SegmentCapacity * Index) / SegmentCapacity, 0.0, 1.0)) : 0.f;
		if (bSnap)
		{
			Segment.PreviousFraction = Fraction;
			Segment.AnimationElapsed = DCPlayerStatus::ArmorAnimationSeconds;
			Segment.bAnimating = false;
			Segment.bHealing = false;
		}
		else if (!FMath::IsNearlyEqual(Fraction, Segment.Fraction))
		{
			// Repeated damage starts at the last actual fill, so spent shield is never repainted.
			// If a refill is interrupted, continue from its visible position instead of jumping to its target.
			Segment.PreviousFraction = Segment.bAnimating && Segment.bHealing
				? FMath::Lerp(Segment.PreviousFraction, Segment.Fraction,
					DCPlayerStatus::ArmorCurve(Segment.AnimationElapsed, 0.f, DCPlayerStatus::ArmorTransitionSeconds))
				: Segment.Fraction;
			Segment.bAnimating = !FMath::IsNearlyEqual(Segment.PreviousFraction, Fraction);
			Segment.bHealing = Fraction > Segment.PreviousFraction;
			Segment.AnimationElapsed = 0.f;
		}
		Segment.Fraction = Fraction;
		ApplyArmorBrush(Segment.Border, Segment.AuthoredBorderBrush, Segment.BorderMID, Segment.BorderSource);
		ApplyArmorBrush(Segment.Fill, Segment.AuthoredFillBrush, Segment.FillMID, Segment.FillSource);
		ApplyArmorAnimation(Segment);
		const FVector2D Scale = Segment.AuthoredFillScale;
		Segment.Fill->SetRenderScale(FVector2D(Scale.X * (Segment.FillMID ? 1.f : Segment.Fraction), Scale.Y));
	}
	PresentedMaxArmor = DisplayedMaxArmor;
	bArmorPresentationInitialized = bHasContext || bPreviewing;
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
	ApplySignatureBrush(SignatureStar, AuthoredStarBrush, Color, false, StarMID, StarSource);
	ApplySignatureBrush(SignatureRing, AuthoredRingBrush, Color, true, RingMID, RingSource);
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
