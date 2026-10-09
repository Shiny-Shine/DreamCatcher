// DreamCatcher-only isolated tests; test curves/quantities are not production rifle balance.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystem/Abilities/DCLyraAbilityCost_ItemTagStack.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "Equipment/Lyra/DCLyraEquipmentInstance.h"
#include "Equipment/Lyra/DCLyraGameplayAbility_FromEquipment.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/DCInventoryItemInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Weapon/Lyra/DCLyraRangedWeaponInstance.h"

namespace DCWeaponFoundationTests
{
bool StartWorld(FAutomationTestBase& Test, FTestWorldWrapper& Fixture)
{
	// GameInstance setup touches process-wide delegates. Use a separate process, never the user's open editor.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCWeaponIsolatedAutomation")))
	{
		Test.AddError(TEXT("Run in a separate UnrealEditor-Cmd process with -DCWeaponIsolatedAutomation."));
		return false;
	}
	if (!Fixture.CreateTestWorld(EWorldType::Game))
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	Fixture.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	if (!Fixture.BeginPlayInTestWorld())
	{
		Fixture.ForwardErrorMessages(&Test);
		return false;
	}
	return true;
}

template <typename T>
T* TestStruct(FAutomationTestBase& Test, UObject* Object, const FName Name)
{
	// Only transient instances created by these tests are changed; never a CDO or a saved asset.
	FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), Name);
	if (!Test.TestNotNull(*Name.ToString(), Property)
		|| !Test.TestTrue(TEXT("Expected reflected struct type"), Property->Struct == T::StaticStruct()))
	{
		return nullptr;
	}
	return Property->ContainerPtrToValuePtr<T>(Object);
}

void SetTestCurve(FRuntimeFloatCurve& Curve, const FVector2D First, const FVector2D Second)
{
	Curve.ExternalCurve = nullptr;
	Curve.EditorCurveData.Reset();
	const FKeyHandle FirstKey = Curve.EditorCurveData.AddKey(static_cast<float>(First.X), static_cast<float>(First.Y));
	const FKeyHandle SecondKey = Curve.EditorCurveData.AddKey(static_cast<float>(Second.X), static_cast<float>(Second.Y));
	Curve.EditorCurveData.SetKeyInterpMode(FirstKey, RCIM_Linear);
	Curve.EditorCurveData.SetKeyInterpMode(SecondKey, RCIM_Linear);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCWeaponSpreadFoundationTest,
	"DreamCatcher.R6.Weapon.SpreadAndAttenuation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCWeaponSpreadFoundationTest::RunTest(const FString& Parameters)
{
	using namespace DCWeaponFoundationTests;
	FTestWorldWrapper Fixture;
	if (!StartWorld(*this, Fixture))
	{
		return false;
	}
	APawn* Pawn = Fixture.GetTestWorld()->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Isolated Pawn"), Pawn))
	{
		return false;
	}
	TStrongObjectPtr<UDCLyraRangedWeaponInstance> WeaponOwner(NewObject<UDCLyraRangedWeaponInstance>(Pawn, NAME_None, RF_Transient));
	UDCLyraRangedWeaponInstance* Weapon = WeaponOwner.Get();
	FRuntimeFloatCurve* Spread = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToSpreadCurve"));
	FRuntimeFloatCurve* HeatPerShot = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToHeatPerShotCurve"));
	FRuntimeFloatCurve* Cooling = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToCoolDownPerSecondCurve"));
	FRuntimeFloatCurve* Falloff = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("DistanceDamageFalloff"));
	FBoolProperty* FirstShot = FindFProperty<FBoolProperty>(Weapon->GetClass(), TEXT("bAllowFirstShotAccuracy"));
	FMapProperty* Materials = FindFProperty<FMapProperty>(Weapon->GetClass(), TEXT("MaterialDamageMultiplier"));
	if (!Spread || !HeatPerShot || !Cooling || !Falloff
		|| !TestNotNull(TEXT("First shot setting"), FirstShot) || !TestNotNull(TEXT("Material multipliers"), Materials))
	{
		return false;
	}
	TestEqual(TEXT("Original default bullets per cartridge"), Weapon->GetBulletsPerCartridge(), 1);
	TestEqual(TEXT("Original default maximum range"), Weapon->GetMaxDamageRange(), 25000.0f);
	TestEqual(TEXT("Original default line trace radius"), Weapon->GetBulletTraceSweepRadius(), 0.0f);
	TestEqual(TEXT("Original default spread exponent"), Weapon->GetSpreadExponent(), 1.0f);
	TestEqual(TEXT("Empty falloff curve has unit attenuation"), Weapon->GetDistanceAttenuation(500.0f), 1.0f);
	TestEqual(TEXT("No physical material has unit attenuation"), Weapon->GetPhysicalMaterialAttenuation(nullptr), 1.0f);

	// Explicit synthetic test data, NOT inferred ID_Rifle/B_WeaponInstance_Rifle values.
	SetTestCurve(*Spread, FVector2D(0.0, 2.0), FVector2D(10.0, 12.0));
	SetTestCurve(*HeatPerShot, FVector2D(0.0, 1.0), FVector2D(10.0, 1.0));
	SetTestCurve(*Cooling, FVector2D(0.0, 2.0), FVector2D(10.0, 2.0));
	FirstShot->SetPropertyValue_InContainer(Weapon, true);
	Weapon->OnEquipped();
	TestEqual(TEXT("Original equip starts heat at range midpoint"), Weapon->GetCalculatedSpreadAngle(), 7.0f);
	Weapon->AddSpread();
	TestEqual(TEXT("One shot adds curve-defined heat"), Weapon->GetCalculatedSpreadAngle(), 8.0f);
	Fixture.TickTestWorld(0.5f);
	Weapon->Tick(0.5f);
	TestEqual(TEXT("Cooling follows heat curve"), Weapon->GetCalculatedSpreadAngle(), 7.0f);
	TestFalse(TEXT("Heated weapon does not have first shot accuracy"), Weapon->HasFirstShotAccuracy());
	Fixture.TickTestWorld(5.0f);
	Weapon->Tick(5.0f);
	TestEqual(TEXT("Heat clamps at minimum"), Weapon->GetCalculatedSpreadAngle(), 2.0f);
	TestTrue(TEXT("First shot accuracy at minimum spread and default multipliers"), Weapon->HasFirstShotAccuracy());
	TestEqual(TEXT("First shot accuracy zeroes final multiplier"), Weapon->GetCalculatedSpreadAngleMultiplier(), 0.0f);
	Weapon->AddSpread();
	Weapon->Tick(0.0f);
	TestFalse(TEXT("Added heat removes first shot accuracy on next Tick"), Weapon->HasFirstShotAccuracy());
	TestEqual(TEXT("Default non-first-shot multiplier"), Weapon->GetCalculatedSpreadAngleMultiplier(), 1.0f);

	SetTestCurve(*Falloff, FVector2D(0.0, 1.0), FVector2D(1000.0, 0.5));
	TestEqual(TEXT("Distance attenuation evaluates original curve"), Weapon->GetDistanceAttenuation(500.0f), 0.75f);
	const FGameplayTag FirstTag = FGameplayTag::RequestGameplayTag(TEXT("Test.R2.CostCharge"));
	const FGameplayTag SecondTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.ActivateFail.Cost"));
	TMap<FGameplayTag, float>* Multipliers = Materials->ContainerPtrToValuePtr<TMap<FGameplayTag, float>>(Weapon);
	Multipliers->Add(FirstTag, 2.0f);
	Multipliers->Add(SecondTag, 0.25f);
	UDCPhysicalMaterialWithTags* Material = NewObject<UDCPhysicalMaterialWithTags>(GetTransientPackage(), NAME_None, RF_Transient);
	Material->Tags.AddTag(FirstTag);
	Material->Tags.AddTag(SecondTag);
	TestEqual(TEXT("Matching physical material tags multiply"), Weapon->GetPhysicalMaterialAttenuation(Material), 0.5f);
	Weapon->OnUnequipped();
	AddInfo(TEXT("Synthetic curves only; actual rifle balance, recovery-delay timing after firing, aim/crouch/fall transitions, viewport projection, and networking remain unverified."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCWeaponSpreadRecoveryDelayTest,
	"DreamCatcher.R6.Weapon.SpreadRecoveryDelay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCWeaponSpreadRecoveryDelayTest::RunTest(const FString& Parameters)
{
	using namespace DCWeaponFoundationTests;
	FTestWorldWrapper Fixture;
	if (!StartWorld(*this, Fixture))
	{
		return false;
	}
	APawn* Pawn = Fixture.GetTestWorld()->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Isolated Pawn"), Pawn))
	{
		return false;
	}
	TStrongObjectPtr<UDCLyraRangedWeaponInstance> WeaponOwner(NewObject<UDCLyraRangedWeaponInstance>(Pawn, NAME_None, RF_Transient));
	UDCLyraRangedWeaponInstance* Weapon = WeaponOwner.Get();
	FRuntimeFloatCurve* Spread = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToSpreadCurve"));
	FRuntimeFloatCurve* HeatPerShot = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToHeatPerShotCurve"));
	FRuntimeFloatCurve* Cooling = TestStruct<FRuntimeFloatCurve>(*this, Weapon, TEXT("HeatToCoolDownPerSecondCurve"));
	FFloatProperty* RecoveryDelay = FindFProperty<FFloatProperty>(Weapon->GetClass(), TEXT("SpreadRecoveryCooldownDelay"));
	if (!Spread || !HeatPerShot || !Cooling || !TestNotNull(TEXT("Recovery delay setting"), RecoveryDelay))
	{
		return false;
	}

	// Synthetic curves on this transient instance only; no production CDO/asset changes.
	SetTestCurve(*Spread, FVector2D(0.0, 2.0), FVector2D(10.0, 12.0));
	SetTestCurve(*HeatPerShot, FVector2D(0.0, 1.0), FVector2D(10.0, 1.0));
	SetTestCurve(*Cooling, FVector2D(0.0, 2.0), FVector2D(10.0, 2.0));
	RecoveryDelay->SetPropertyValue_InContainer(Weapon, 0.15f);
	// Advance beyond the delay so an uninitialized LastFireTime=0 cannot pass this test.
	Fixture.TickTestWorld(1.0f);
	Weapon->OnEquipped();
	Weapon->UpdateFiringTime();
	Weapon->AddSpread();
	TestEqual(TEXT("First shot adds heat"), Weapon->GetCalculatedSpreadAngle(), 8.0f);
	Fixture.TickTestWorld(0.1f);
	Weapon->Tick(0.1f);
	TestEqual(TEXT("No cooling before the recovery delay"), Weapon->GetCalculatedSpreadAngle(), 8.0f);

	Weapon->UpdateFiringTime();
	Weapon->AddSpread();
	TestEqual(TEXT("Next shot accumulates heat during the delay"), Weapon->GetCalculatedSpreadAngle(), 9.0f);
	Fixture.TickTestWorld(0.1f);
	Weapon->Tick(0.1f);
	// 0.20 s since the first shot, but only 0.10 s since the second shot.
	TestEqual(TEXT("Each shot restarts the full recovery delay"), Weapon->GetCalculatedSpreadAngle(), 9.0f);

	Fixture.TickTestWorld(0.06f);
	Weapon->Tick(0.06f);
	// Preserve the original per-Tick cooling calculation once the delay has elapsed.
	TestTrue(TEXT("Cooling resumes using the original curve after the delay"),
		FMath::IsNearlyEqual(Weapon->GetCalculatedSpreadAngle(), 9.0f - 2.0f * 0.06f, KINDA_SMALL_NUMBER));
	Fixture.TickTestWorld(0.05f);
	Weapon->Tick(0.05f);
	TestTrue(TEXT("Cooling continues without another shot"),
		FMath::IsNearlyEqual(Weapon->GetCalculatedSpreadAngle(), 9.0f - 2.0f * (0.06f + 0.05f), KINDA_SMALL_NUMBER));
	Weapon->OnUnequipped();
	AddInfo(TEXT("Synthetic positive-delay regression only; actual rifle PIE, presentation, and networking require separate verification."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCWeaponItemCostFoundationTest,
	"DreamCatcher.R6.Weapon.ItemTagCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCWeaponItemCostFoundationTest::RunTest(const FString& Parameters)
{
	using namespace DCWeaponFoundationTests;
	FTestWorldWrapper Fixture;
	if (!StartWorld(*this, Fixture))
	{
		return false;
	}
	APawn* Pawn = Fixture.GetTestWorld()->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Isolated Pawn"), Pawn))
	{
		return false;
	}
	UDCAbilitySystemComponent* ASC = NewObject<UDCAbilitySystemComponent>(Pawn, TEXT("R6_TestASC"), RF_Transient);
	Pawn->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Pawn, Pawn);
	UDCLyraEquipmentInstance* Equipment = NewObject<UDCLyraEquipmentInstance>(Pawn, NAME_None, RF_Transient);
	UDCInventoryItemInstance* Item = NewObject<UDCInventoryItemInstance>(Pawn, NAME_None, RF_Transient);
	Equipment->SetInstigator(Item);
	const FGameplayTag CostTag = FGameplayTag::RequestGameplayTag(TEXT("Test.R2.CostCharge"));
	Item->AddStatTagStack(CostTag, 7);
	UDCLyraAbilityCost_ItemTagStack* Cost = NewObject<UDCLyraAbilityCost_ItemTagStack>(GetTransientPackage(), NAME_None, RF_Transient);
	FGameplayTag* Tag = TestStruct<FGameplayTag>(*this, Cost, TEXT("Tag"));
	FScalableFloat* Quantity = TestStruct<FScalableFloat>(*this, Cost, TEXT("Quantity"));
	if (!Tag || !Quantity)
	{
		return false;
	}
	*Tag = CostTag;
	Quantity->SetValue(2.9f); // Original truncates to two stacks, it does not round to three.
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(
		UDCLyraGameplayAbility_FromEquipment::StaticClass(), 1, INDEX_NONE, Equipment));
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	if (!TestNotNull(TEXT("Granted FromEquipment spec"), Spec))
	{
		return false;
	}
	UDCLyraGameplayAbility_FromEquipment* Ability = Cast<UDCLyraGameplayAbility_FromEquipment>(Spec->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Instanced FromEquipment ability"), Ability))
	{
		return false;
	}
	TestTrue(TEXT("Ability resolves the actual item"), Ability->GetAssociatedItem() == Item);
	FGameplayTagContainer FailureTags;
	for (int32 ExpectedRemaining : {5, 3, 1})
	{
		TestTrue(TEXT("Item has enough stacks"), Cost->CheckCost(Ability, Handle, ASC->AbilityActorInfo.Get(), &FailureTags));
		Cost->ApplyCost(Ability, Handle, ASC->AbilityActorInfo.Get(), FGameplayAbilityActivationInfo());
		TestEqual(TEXT("Authority consumes truncated item cost"), Item->GetStatTagStackCount(CostTag), ExpectedRemaining);
	}
	TestFalse(TEXT("Insufficient item stacks fail cost check"), Cost->CheckCost(Ability, Handle, ASC->AbilityActorInfo.Get(), &FailureTags));
	TestTrue(TEXT("Original failure tag is reported"), FailureTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Ability.ActivateFail.Cost"))));
	TestEqual(TEXT("Failed check does not consume stacks"), Item->GetStatTagStackCount(CostTag), 1);
	Equipment->SetInstigator(nullptr);
	TestFalse(TEXT("Missing associated item fails cost check"), Cost->CheckCost(Ability, Handle, ASC->AbilityActorInfo.Get(), nullptr));
	ASC->ClearAbility(Handle);
	TestEqual(TEXT("Test Ability cleanup"), ASC->GetActivatableAbilities().Num(), 0);
	AddInfo(TEXT("Authority-side ItemTagStack cost only; original rifle Blueprint firing/reload and client prediction are not exercised."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
