#pragma once

#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "DCPlayerStatusWidget.generated.h"

class APawn;
class APlayerController;
class UAbilitySystemComponent;
class UCanvasPanel;
class UDCLyraHealthComponent;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USizeBox;
class UTextBlock;
class UWidgetAnimation;
class UWorld;

UENUM(BlueprintType)
enum class EDCPlayerSignatureState : uint8
{
	Unavailable,
	Ready,
	Active,
	Cooldown
};

/** Presentation values only. No health, armor, or ability balance is written by this widget. */
USTRUCT(BlueprintType)
struct FDCPlayerStatusStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float BarWidth = 440.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float ArmorHeight = 24.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float ArmorHealthGap = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float HealthHeight = 32.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float BottomMargin = 72.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float HorizontalOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float ArmorSegmentGap = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float StarSize = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float StarGap = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout") float StarIconScale = .6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text") int32 FontSize = 18;
	/** Empty preserves the original HP font; the armor label uses the engine fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text") FSlateFontInfo TextFont;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text") FLinearColor TextColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armor") FLinearColor ArmorColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armor") FSlateBrush ArmorBorderBrush;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armor") FSlateBrush ArmorFillBrush;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FSlateBrush StarBrush;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FSlateBrush SignatureRingBrush;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FLinearColor ReadyColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FLinearColor ActiveColor = FLinearColor(.6f, .85f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FLinearColor CooldownColor = FLinearColor(.35f, .35f, .35f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") FLinearColor UnavailableColor = FLinearColor(.15f, .15f, .15f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") float ReadyGlowAlpha = .3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signature") float InactiveGlowAlpha = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Refresh") float RefreshInterval = .05f;
	bool IsValid() const;
};

USTRUCT()
struct FDCPlayerArmorSegment
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
	UPROPERTY(Transient) TObjectPtr<UImage> Border;
	UPROPERTY(Transient) TObjectPtr<UImage> Fill;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BorderMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FillMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> BorderSource;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> FillSource;
	float Fraction = 0.f;
};

/** Passive Stage HUD wrapper around the original Lyra W_Healthbar. */
UCLASS(BlueprintType, Blueprintable)
class DREAMCATCHER_API UDCPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UDCPlayerStatusWidget(const FObjectInitializer& Initializer = FObjectInitializer::Get());
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Status") FDCPlayerStatusStyle StatusStyle;
	/** The existing W_Healthbar Blueprint, after its three denominator/initial-value edits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Status") TSubclassOf<UUserWidget> HealthWidgetClass;
	/** Nullable-safe UI helper for the existing Health Blueprint. The fallback is presentation-only. */
	UFUNCTION(BlueprintPure, Category="Player Status")
	static float GetSafeMaxHealth(const UDCLyraHealthComponent* HealthComponent);
	UFUNCTION(BlueprintCallable, Category="Player Status") void RefreshPlayerStatus();
	UFUNCTION(BlueprintPure, Category="Player Status") bool HasPlayerStatus() const { return bHasContext; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetDisplayedHealth() const { return DisplayedHealth; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetDisplayedMaxHealth() const { return DisplayedMaxHealth; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetDisplayedArmor() const { return DisplayedArmor; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetDisplayedMaxArmor() const { return DisplayedMaxArmor; }
	UFUNCTION(BlueprintPure, Category="Player Status") TArray<float> GetArmorSegmentFills() const;
	UFUNCTION(BlueprintPure, Category="Player Status") EDCPlayerSignatureState GetSignatureState() const { return SignatureState; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetSignatureCooldownRemaining() const { return CooldownRemaining; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetSignatureCooldownDuration() const { return CooldownDuration; }
	UFUNCTION(BlueprintPure, Category="Player Status") float GetSignatureProgress() const { return SignatureProgress; }
	UFUNCTION(BlueprintPure, Category="Player Status") int32 GetSignatureMatchingSpecCount() const { return SignatureMatchingSpecCount; }
	UFUNCTION(BlueprintPure, Category="Player Status") APawn* GetStatusPawn() const;
	UFUNCTION(BlueprintPure, Category="Player Status") UAbilitySystemComponent* GetStatusAbilitySystem() const;
	UFUNCTION(BlueprintPure, Category="Player Status") UUserWidget* GetOriginalHealthWidget() const { return OriginalHealthWidget; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	static FDCPlayerStatusStyle ResolveStyle(const FDCPlayerStatusStyle& Source);
	void ApplyLayout();
	void EnsureHealthWidget();
	void ReleaseHealthWidget();
	void BindHealthSpawnAnimation();
	void UpdateHealthPresentation();
	void UpdateArmorPresentation();
	void ReadSignatureState();
	void UpdateSignaturePresentation();
	void UnbindSource();
	void UnbindHealth();
	void HandleWorldTearDown(UWorld* World);
	bool IsCurrentSource(const UDCLyraHealthComponent* Component) const;
	void ApplyArmorBrush(UImage* Image, const FSlateBrush& Brush, float Fraction,
		TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& Source);
	void ApplySignatureBrush(UImage* Image, const FSlateBrush& Brush, const FLinearColor& Color, bool bRing,
		TObjectPtr<UMaterialInstanceDynamic>& MID, TObjectPtr<UMaterialInterface>& Source);
	UFUNCTION() void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION() void HandleHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator);
	UFUNCTION() void HandleMaxHealthChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator);
	UFUNCTION() void HandleArmorChanged(UDCLyraHealthComponent* Component, float OldValue, float NewValue, AActor* Instigator);
	UFUNCTION() void HandleDeathStarted(AActor* Actor);
	UFUNCTION() void HandleHealthSpawnFinished();
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> BarGroup;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ArmorPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ArmorText;
	UPROPERTY(Transient) TObjectPtr<USizeBox> HealthHost;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> OriginalHealthWidget;
	UPROPERTY(Transient) TObjectPtr<UWidgetAnimation> HealthSpawnAnimation;
	UPROPERTY(Transient) TArray<FDCPlayerArmorSegment> ArmorSegments;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SignaturePanel;
	UPROPERTY(Transient) TObjectPtr<UImage> SignatureStar;
	UPROPERTY(Transient) TObjectPtr<UImage> SignatureRing;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StarMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> StarSource;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RingSource;
	UPROPERTY(Transient) FDCPlayerStatusStyle Appearance;
	TWeakObjectPtr<APlayerController> SourceController;
	TWeakObjectPtr<APawn> SourcePawn;
	TWeakObjectPtr<UDCLyraHealthComponent> SourceHealth;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TWeakObjectPtr<UWorld> TimerWorld;
	FTimerHandle RefreshTimer;
	FDelegateHandle WorldTearDownHandle;
	FWidgetAnimationDynamicEvent SpawnFinishedDelegate;
	EDCPlayerSignatureState SignatureState = EDCPlayerSignatureState::Unavailable;
	float DisplayedHealth = 0.f;
	float DisplayedMaxHealth = 0.f;
	float DisplayedArmor = 0.f;
	float DisplayedMaxArmor = 0.f;
	float CooldownRemaining = 0.f;
	float CooldownDuration = 0.f;
	float SignatureProgress = 0.f;
	float TimerInterval = 0.f;
	int32 SignatureMatchingSpecCount = 0;
	FString SignatureAbilityPath;
	bool bHasContext = false;
	bool bConstructed = false;
	bool bRefreshing = false;
	bool bNeedsMaxText = true;
	bool bNeedsHealthMaterialSync = true;
	bool bNeedsHealthNumberSync = true;
	bool bReportedInvalidStyle = false;
	bool bReportedInvalidHealthClass = false;
	bool bDiagnosticVolatile = false;
	bool bSavedVolatility = false;
};
