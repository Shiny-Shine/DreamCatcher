#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/Common/DCLyraListView.h"
#include "UI/Common/DCLyraWidgetFactory_Class.h"
#include "UI/Stage/DCMissionListData.h"
#include "DCMissionEntryWidget.generated.h"

class UImage;
class USizeBox;
class UTextBlock;
class STableViewBase;
class UWidgetTree;

DECLARE_MULTICAST_DELEGATE(FDCMissionEntryChanged);

/** Transient view data. Never writes runtime progress into the definition asset. */
UCLASS()
class DREAMCATCHER_API UDCMissionEntryData : public UObject
{
	GENERATED_BODY()
public:
	void Update(const FDCMissionProgress& InProgress, float InWidth, float InHeight);
	void SetPaintContext(const FString& Source, int32 Phase, int32 Page, int64 Serial)
	{ PaintSource = Source; PaintPhase = Phase; PaintPage = Page; PaintSerial = Serial; }
	FString PaintSource;
	int32 PaintPhase = INDEX_NONE;
	int32 PaintPage = INDEX_NONE;
	int64 PaintSerial = 0;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	FDCMissionProgress GetProgress() const { return Progress; }
	float GetAvailableWidth() const { return AvailableWidth; }
	float GetRowHeight() const { return RowHeight; }
	FDCMissionEntryChanged OnChanged;
private:
	UPROPERTY(Transient) FDCMissionProgress Progress;
	float AvailableWidth = 0.f;
	float RowHeight = 48.f;
};

/** Original Lyra class factory with one mission-data rule. */
UCLASS()
class DREAMCATCHER_API UDCMissionEntryFactory : public UDCLyraWidgetFactory_Class
{
	GENERATED_BODY()
public:
	void Configure(TSubclassOf<UUserWidget> WidgetClass);
};

/** Passive configuration of the existing Lyra list implementation. */
UCLASS()
class DREAMCATCHER_API UDCMissionListView : public UDCLyraListView
{
	GENERATED_BODY()
public:
	UDCMissionListView(const FObjectInitializer& Initializer = FObjectInitializer::Get());
	void Configure(TSubclassOf<UUserWidget> WidgetClass);
protected:
	virtual TSharedRef<STableViewBase> RebuildListWidget() override;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCMissionHUDTestAccess;
#endif
};

/** Designer-authored row used by the existing Lyra list pool. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCMissionEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	static FText FormatProgress(const FDCMissionProgress& Progress);
	static const UWidgetTree* GetDesignerTree(TSubclassOf<UUserWidget> EntryClass);
	static UTextBlock* CreateMeasurementText(const UWidgetTree* Design, UObject* Outer);
	static float MeasureTextHeight(UTextBlock* TextWidget, const FText& Text, float Width);
	static float MeasureRowHeight(const FDCMissionProgress& Progress, const UWidgetTree* Design, UTextBlock* MeasurementText, float Width);
	/** Completed-state art. The unchecked brush is authored on MissionCheckbox. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|Appearance")
	FSlateBrush CompletedBrush;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	FText GetDisplayedText() const;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	FName GetDisplayedMissionId() const;
	virtual bool IsListItemSelectable() const override { return false; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeDestruct() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	void UnbindEntry();
	void BindDesignerWidgets();
	void Refresh();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USizeBox> MissionRow;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USizeBox> MissionCheckboxBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> MissionCheckbox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MissionText;
	UPROPERTY(Transient) FSlateBrush UncheckedBrush;
	bool bBrushCached = false;
	TWeakObjectPtr<UDCMissionEntryData> Entry;
	FDelegateHandle ChangeHandle;
	bool bDiagnosticVolatility = false;
	bool bPreviousVolatility = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCMissionHUDTestAccess;
#endif
};
