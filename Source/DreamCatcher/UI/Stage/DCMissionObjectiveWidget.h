#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/Stage/DCMissionListData.h"
#include "TimerManager.h"
#include "DCMissionObjectiveWidget.generated.h"

class UBorder;
class UDCMissionEntryData;
class UDCMissionListView;
class UImage;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class ADCStageDirector;
class UDCMissionTransitionSwitcher;

USTRUCT()
struct FDCMissionBodyPage
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Root;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PhaseText;
	UPROPERTY(Transient) TObjectPtr<USizeBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UDCMissionListView> List;
	UPROPERTY(Transient) TArray<TObjectPtr<UDCMissionEntryData>> Items;
	UPROPERTY(Transient) TArray<FDCMissionProgress> Rows;
	UPROPERTY(Transient) FDCMissionHUDStyle Style;
	FText Title;
	int32 PhaseIndex = INDEX_NONE;
};

/** Passive mission presentation. StageDirector is the sole progress source. */
UCLASS()
class DREAMCATCHER_API UDCMissionObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UDCMissionObjectiveWidget(const FObjectInitializer& Initializer = FObjectInitializer::Get());
	void SetMissionSource(ADCStageDirector* Source);
	void SetLiveMissionStatus(ADCStageDirector* Source, int32 PhaseIndex, const UDCMissionListData* Data,
		const FText& StageTitle, const TArray<FDCMissionProgress>& Progress);
	void ShowCompletedPhase(ADCStageDirector* Source, const FDCMissionTransitionSnapshot& Snapshot);
	FSimpleDelegate RequestLatestLive;
	UFUNCTION(BlueprintCallable, Category="Mission|HUD")
	void SetMissionStatus(const UDCMissionListData* Data, const FText& StageTitle, const TArray<FDCMissionProgress>& Progress);
	UFUNCTION(BlueprintCallable, Category="Mission|HUD")
	void ClearMissionStatus();
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	TArray<FDCMissionProgress> GetDisplayedMissions() const { return DisplayedMissions; }
	/** True means authors must shorten/split the configuration; HUD input is unchanged. */
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	bool DoesContentOverflow() const { return bContentOverflow; }
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	float GetRequiredContentHeight() const { return RequiredContentHeight; }
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	UWidget* GetActiveBodyPage() const;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	int32 GetActiveBodyPageIndex() const { return ActivePage; }
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	int32 GetDisplayedPhaseIndex() const;
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	int64 GetAnimationSerial() const { return TransitionSerial; }
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	bool IsMissionTransitionActive() const { return State != EBodyState::Live; }
	UFUNCTION(BlueprintPure, Category="Mission|HUD")
	FName GetBodyTransitionState() const;
	static bool IsEntryClassUsable(TSubclassOf<UUserWidget> EntryClass);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	enum class EBodyState : uint8 { Live, Holding, FadingOut, FadingIn };
	static FDCMissionHUDStyle ResolveStyle(const FDCMissionHUDStyle& Source);
	bool ValidatePresentation(const FDCMissionHUDStyle& Style, const TArray<FDCMissionProgress>& Rows);
	void ApplyPage(int32 Page, int32 PhaseIndex, const FText& Title, const TArray<FDCMissionProgress>& Rows, const FDCMissionHUDStyle& Style);
	void SynchronizePage(int32 Page);
	void SelectActiveAliases();
	void ClearPage(int32 Page);
	void BeginFade(int64 Serial);
	void HandleMidpoint(int64 Serial, int32 Page);
	void HandleFinished(int64 Serial);
	void Refresh();
	void RemoveList(int32 Page);
	UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Contents;
	UPROPERTY(Transient) TObjectPtr<UDCMissionTransitionSwitcher> BodySwitcher;
	UPROPERTY(Transient) TArray<FDCMissionBodyPage> Pages;
	UPROPERTY(Transient) TObjectPtr<USizeBox> HeaderBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UImage> HeaderIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PhaseText;
	UPROPERTY(Transient) TObjectPtr<UDCMissionListView> MissionList;
	UPROPERTY(Transient) TArray<FDCMissionProgress> DisplayedMissions;
	UPROPERTY(Transient) FDCMissionHUDStyle Appearance;
	FText CurrentStageTitle;
	float RequiredContentHeight = 0.f;
	bool bHasStatus = false;
	bool bContentOverflow = false;
	bool bReportedInvalidConfiguration = false;
	bool bReportedStyleFallback = false;
	TWeakObjectPtr<ADCStageDirector> SourceDirector;
	FTimerHandle HoldTimer;
	EBodyState State = EBodyState::Live;
	int32 ActivePage = 0;
	int32 IncomingPage = 1;
	int64 TransitionSerial = 0;
	bool bResolvingMidpoint = false;
	bool bMidpointApplied = false;
	float FrozenPanelHeight = 0.f;
	int32 LastCompletedPhase = INDEX_NONE;
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FDCMissionHUDTestAccess;
#endif
};
