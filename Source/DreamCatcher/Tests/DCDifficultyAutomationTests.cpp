#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && WITH_SERVER_CODE

#include "System/DCDifficultySubsystem.h"
#include "AbilitySystem/DCGameplayEffectContext.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/DCInventoryFragment_SetStats.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Inventory/DCInventoryManagerComponent.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/Lyra/DCLyraPlayerBotController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/DCBossTestTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace DCDifficultyTests
{
bool Allowed(FAutomationTestBase& Test)
{
	if (FApp::IsUnattended() && FParse::Param(FCommandLine::Get(), TEXT("DCDifficultyIsolatedAutomation"))) { return true; }
	Test.AddError(TEXT("Use a separate unattended editor with -DCDifficultyIsolatedAutomation -DCLyraRuntimeDiagnostics."));
	return false;
}

UDCDifficultyCatalog* MakeCatalog(UObject* Outer)
{
	UDCDifficultyCatalog* Catalog = NewObject<UDCDifficultyCatalog>(Outer);
	Catalog->Easy = NewObject<UDCDifficultyPreset>(Catalog);
	Catalog->Normal = NewObject<UDCDifficultyPreset>(Catalog);
	Catalog->Hard = NewObject<UDCDifficultyPreset>(Catalog);
	Catalog->Easy->Values.EnemyDamage = 7.f;
	Catalog->Easy->Values.PlayerSMGDamage = 50.f;
	Catalog->Easy->Values.PlayerPistolDamage = 80.f;
	Catalog->Easy->Values.BossLineDamage = 24.f;
	Catalog->Easy->Values.BossGroundDamage = 35.f;
	Catalog->Hard->Values.SMGLoadedAmmo = 20;
	Catalog->Hard->Values.SMGSpareAmmo = 60;
	Catalog->Hard->Values.PistolLoadedAmmo = 8;
	Catalog->Hard->Values.PistolSpareAmmo = 120;
	// Only read the two real item definitions; never edit CDOs or save packages.
	Catalog->SMGItem = LoadClass<UDCInventoryItemDefinition>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/ID_Rifle.ID_Rifle_C"));
	Catalog->PistolItem = LoadClass<UDCInventoryItemDefinition>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Pistol/ID_Pistol.ID_Pistol_C"));
	Catalog->SMGDamageEffect = UGameplayEffect::StaticClass();
	Catalog->PistolDamageEffect = UDCR10AttackDamageFixture::StaticClass();
	return Catalog;
}

FGameplayEffectSpec Spec(TSubclassOf<UGameplayEffect> Effect, const UGameplayAbility* Ability = nullptr)
{
	FGameplayEffectContextHandle Context(new FDCGameplayEffectContext());
	if (Ability) { Context.SetAbility(Ability); }
	return FGameplayEffectSpec(Effect.GetDefaultObject(), Context, 1.f);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDifficultySnapshotTest, "DreamCatcher.Difficulty.ValidationAndRunSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDifficultySnapshotTest::RunTest(const FString&)
{
	if (!DCDifficultyTests::Allowed(*this)) { return false; }
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	UDCDifficultySubsystem* Difficulty = NewObject<UDCDifficultySubsystem>(Instance.Get());
	UDCDifficultyCatalog* Catalog = DCDifficultyTests::MakeCatalog(Instance.Get());
	FString Error;
	TestEqual(TEXT("New session defaults to Normal"), Difficulty->GetSelectedDifficulty(), EDCDifficulty::Normal);
	if (!TestTrue(TEXT("Existing magazine capacities accept the approved values"), Catalog->Validate(Error))) { AddError(Error); return false; }
	TestTrue(TEXT("Select Easy"), Difficulty->SelectDifficulty(EDCDifficulty::Easy));
	TestTrue(TEXT("Start new run"), Difficulty->BeginStoryRun(Catalog, Error));
	Catalog->Easy->Values.PlayerSMGDamage = 77.f;
	TestEqual(TEXT("Edits cannot change the running story"), Difficulty->GetActiveValues().PlayerSMGDamage, 50.f);
	TestTrue(TEXT("Travel/retry keeps the run"), Difficulty->EnsureStoryRun(Catalog, Error));
	TestEqual(TEXT("Travel does not reread changed values"), Difficulty->GetActiveValues().PlayerSMGDamage, 50.f);
	TestTrue(TEXT("New story reads edited values"), Difficulty->BeginStoryRun(Catalog, Error));
	TestEqual(TEXT("Edited value applied"), Difficulty->GetActiveValues().PlayerSMGDamage, 77.f);
	TestFalse(TEXT("Invalid enum rejected"), Difficulty->SelectDifficulty(static_cast<EDCDifficulty>(255)));
	Catalog->Easy->Values.SMGLoadedAmmo = 100000;
	TestFalse(TEXT("Over-capacity start is refused"), Difficulty->BeginStoryRun(Catalog, Error));
	TestEqual(TEXT("Rejected data does not corrupt the previous run"), Difficulty->GetActiveValues().PlayerSMGDamage, 77.f);
	Difficulty->EndStoryRun();
	TestFalse(TEXT("Menu ends the run"), Difficulty->HasActiveRun());
	TestEqual(TEXT("Menu remembers selection"), Difficulty->GetSelectedDifficulty(), EDCDifficulty::Easy);
	FDCDifficultyValues Values;
	Values.EnemyDamage = -1.f;
	TestFalse(TEXT("Negative damage refused"), Values.Validate(30, 12, Error));
	Values.EnemyDamage = 0.f;
	TestTrue(TEXT("Zero damage is allowed"), Values.Validate(30, 12, Error));
	Values.PistolSpareAmmo = -1;
	TestFalse(TEXT("Negative reserve refused"), Values.Validate(30, 12, Error));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDifficultyRoutingTest, "DreamCatcher.Difficulty.RoleAndPatternRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDifficultyRoutingTest::RunTest(const FString&)
{
	if (!DCDifficultyTests::Allowed(*this)) { return false; }
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	UDCDifficultySubsystem* Difficulty = NewObject<UDCDifficultySubsystem>(Instance.Get());
	UDCDifficultyCatalog* Catalog = DCDifficultyTests::MakeCatalog(Instance.Get());
	Difficulty->SelectDifficulty(EDCDifficulty::Easy);
	FString Error;
	if (!TestTrue(TEXT("Start Easy"), Difficulty->BeginStoryRun(Catalog, Error))) { AddError(Error); return false; }
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	UWorld* World = Fixture.GetTestWorld();
	// Controllers need PostInitializeComponents before role/possession queries.
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
	APawn* PlayerPawn = World->SpawnActor<APawn>();
	APawn* BotPawn = World->SpawnActor<APawn>();
	APlayerController* Player = World->SpawnActor<APlayerController>();
	ADCLyraPlayerBotController* Bot = World->SpawnActor<ADCLyraPlayerBotController>();
	if (!PlayerPawn || !BotPawn || !Player || !Bot) { AddError(TEXT("Could not create transient role fixtures.")); return false; }
	Player->Possess(PlayerPawn);
	Bot->Possess(BotPawn);
	if (!TestTrue(TEXT("Initialized human controller fixture"), Player->IsPlayerController())
		|| !TestTrue(TEXT("Possessed human pawn fixture"), PlayerPawn->IsPlayerControlled())) { return false; }
	float Damage = 999.f;
	TestTrue(TEXT("Player SMG recognized"), Difficulty->TryOverrideBaseDamage(PlayerPawn, DCDifficultyTests::Spec(Catalog->SMGDamageEffect), Damage));
	TestEqual(TEXT("Absolute damage replaces the old sum"), Damage, 50.f);
	TestTrue(TEXT("Player pistol recognized"), Difficulty->TryOverrideBaseDamage(PlayerPawn, DCDifficultyTests::Spec(Catalog->PistolDamageEffect), Damage));
	TestEqual(TEXT("Pistol has its own value"), Damage, 80.f);
	TestTrue(TEXT("Bot with the same SMG recognized"), Difficulty->TryOverrideBaseDamage(BotPawn, DCDifficultyTests::Spec(Catalog->SMGDamageEffect), Damage));
	TestEqual(TEXT("Bot never takes player weapon damage"), Damage, 7.f);
	const ADCBossCharacter* Boss = GetDefault<ADCR12BossFixture>();
	TestTrue(TEXT("Boss line recognized"), Difficulty->TryOverrideBaseDamage(Boss, DCDifficultyTests::Spec(Catalog->SMGDamageEffect, GetDefault<UDCR12LineFixture>()), Damage));
	TestEqual(TEXT("Line value"), Damage, 24.f);
	TestTrue(TEXT("Boss ground recognized"), Difficulty->TryOverrideBaseDamage(Boss, DCDifficultyTests::Spec(Catalog->SMGDamageEffect, GetDefault<UDCR12GroundFixture>()), Damage));
	TestEqual(TEXT("Ground value"), Damage, 35.f);
	Damage = 999.f;
	TestFalse(TEXT("Diagnostic boss effect without an ability is preserved"), Difficulty->TryOverrideBaseDamage(Boss, DCDifficultyTests::Spec(Catalog->SMGDamageEffect), Damage));
	TestEqual(TEXT("Fallback magnitude unchanged"), Damage, 999.f);
	TestNull(TEXT("A world without an opted-in stage is unaffected"), UDCDifficultySubsystem::GetForStage(World));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDifficultyAmmoTest, "DreamCatcher.Difficulty.InitialAmmoPreservesCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDifficultyAmmoTest::RunTest(const FString&)
{
	if (!DCDifficultyTests::Allowed(*this)) { return false; }
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	UDCDifficultySubsystem* Difficulty = NewObject<UDCDifficultySubsystem>(Instance.Get());
	UDCDifficultyCatalog* Catalog = DCDifficultyTests::MakeCatalog(Instance.Get());
	Difficulty->SelectDifficulty(EDCDifficulty::Hard);
	FString Error;
	if (!TestTrue(TEXT("Start Hard"), Difficulty->BeginStoryRun(Catalog, Error))) { AddError(Error); return false; }
	FTestWorldWrapper Fixture;
	if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
	AActor* Owner = Fixture.GetTestWorld()->SpawnActor<AActor>();
	UDCInventoryManagerComponent* Inventory = NewObject<UDCInventoryManagerComponent>(Owner);
	Owner->AddInstanceComponent(Inventory);
	Inventory->RegisterComponent();
	const FGameplayTag Capacity = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineSize"));
	const FGameplayTag Loaded = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineAmmo"));
	const FGameplayTag Spare = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo"));
	for (bool bSMG : { true, false })
	{
		UDCInventoryItemInstance* Item = Inventory->AddItemDefinition(bSMG ? Catalog->SMGItem : Catalog->PistolItem, 1);
		if (!TestNotNull(TEXT("Granted item"), Item)) { return false; }
		const int32 OriginalCapacity = Item->GetStatTagStackCount(Capacity);
		TestTrue(TEXT("Apply initial ammo"), Difficulty->ApplyStartingAmmo(Item));
		TestEqual(TEXT("Capacity unchanged"), Item->GetStatTagStackCount(Capacity), OriginalCapacity);
		TestEqual(TEXT("Absolute loaded count"), Item->GetStatTagStackCount(Loaded), bSMG ? 20 : 8);
		TestEqual(TEXT("Absolute reserve count"), Item->GetStatTagStackCount(Spare), bSMG ? 60 : 120);
		Inventory->RemoveItemInstance(Item);
	}
	return !HasAnyErrors();
}

#endif
