#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "UI/Common/DCLyraListView.h"
#include "UI/Common/DCLyraWidgetFactory_Class.h"
#include "UI/Stage/DCMissionListData.h"
#include "DCMissionEntryWidget.generated.h"

class UImage;
class USizeBox;
class UTextBlock;
class STableViewBase;

DECLARE_MULTICAST_DELEGATE(FDCMissionEntryChanged);

/** Transient view data. Never writes runtime progress into the definition asset. */
UCLASS()
class DREAMCATCHER_API UDCMissionEntryData : public UObject
{
	GENERATED_BODY()
public:
	void Update(const FDCMissionProgress& InProgress, const FDCMissionHUDStyle& InStyle);
	void SetPaintContext(const FString& Source, int32 Phase, int32 Page, int64 Serial)
	{ PaintSource = Source; PaintPhase = Phase; PaintPage = Page; PaintSerial = Serial; }
	FString PaintSource;
	int32 PaintPhase = INDEX_NONE;
	int32 PaintPage = INDEX_NONE;
	int64 PaintSerial = 0;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	FDCMissionProgress GetProgress() const { return Progress; }
	const FDCMissionHUDStyle& GetStyle() const { return Appearance; }
	float GetRowHeight() const { return RowHeight; }
	FDCMissionEntryChanged OnChanged;
private:
	UPROPERTY(Transient) FDCMissionProgress Progress;
	UPROPERTY(Transient) FDCMissionHUDStyle Appearance;
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

/** Native contents of the thin W_DC_MissionEntry Blueprint used by Lyra's pool. */
UCLASS(Blueprintable)
class DREAMCATCHER_API UDCMissionEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	static FText FormatProgress(const FDCMissionProgress& Progress);
	static float MeasureTextHeight(const FText& Text, int32 FontSize, float Width);
	static float MeasureTextHeight(const FText& Text, const FSlateFontInfo& Font, float Width);
	static float MeasureRowHeight(const FDCMissionProgress& Progress, const FDCMissionHUDStyle& Style);
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
	void Refresh();
	UPROPERTY(Transient) TObjectPtr<USizeBox> RowBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> CheckboxBox;
	UPROPERTY(Transient) TObjectPtr<UImage> Checkbox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
	TWeakObjectPtr<UDCMissionEntryData> Entry;
	FDelegateHandle ChangeHandle;
	bool bDiagnosticVolatility = false;
	bool bPreviousVolatility = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCMissionHUDTestAccess;
#endif
};
