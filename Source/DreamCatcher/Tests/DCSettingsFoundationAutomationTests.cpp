// DreamCatcher-only data contracts for the original-derived Performance/Audio settings.
// Does not apply frame pacing, device profiles, audio mixes, benchmarks, or saved config.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Audio/DCLyraAudioSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/EnumRange.h"
#include "Performance/DCLyraPerformanceSettings.h"
#include "Performance/DCLyraPerformanceStatTypes.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace DCSettingsFoundationTests
{
template <typename T, SIZE_T N>
void CheckEnum(FAutomationTestBase& Test, const TCHAR* Label, const TCHAR* const (&Names)[N])
{
	const UEnum* Enum = StaticEnum<T>();
	if (!Test.TestNotNull(Label, Enum))
	{
		return;
	}
	for (int32 Index = 0; Index < static_cast<int32>(N); ++Index)
	{
		Test.TestEqual(FString::Printf(TEXT("%s ordinal for %s"), Label, Names[Index]),
			Enum->GetValueByNameString(Names[Index]), static_cast<int64>(Index));
		Test.TestEqual(FString::Printf(TEXT("%s declaration order %d"), Label, Index),
			Enum->GetNameStringByIndex(Index), FString(Names[Index]));
	}
}

template <SIZE_T N>
void CheckFrameRates(FAutomationTestBase& Test, const TCHAR* Label, const TArray<int32>& Actual, const int32 (&Expected)[N])
{
	if (Test.TestEqual(Label, Actual.Num(), static_cast<int32>(N)))
	{
		for (int32 Index = 0; Index < Actual.Num(); ++Index)
		{
			Test.TestEqual(FString::Printf(TEXT("%s [%d]"), Label, Index), Actual[Index], Expected[Index]);
		}
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPerformanceEnumContractTest,
	"DreamCatcher.R5.SettingsFoundation.PerformanceEnums",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCPerformanceEnumContractTest::RunTest(const FString& Parameters)
{
	const TCHAR* const DisplayModes[] = { TEXT("Hidden"), TEXT("TextOnly"), TEXT("GraphOnly"), TEXT("TextAndGraph") };
	const TCHAR* const FramePacingModes[] = { TEXT("DesktopStyle"), TEXT("ConsoleStyle"), TEXT("MobileStyle") };
	const TCHAR* const PerformanceStats[] = {
		TEXT("ClientFPS"), TEXT("ServerFPS"), TEXT("IdleTime"), TEXT("FrameTime"),
		TEXT("FrameTime_GameThread"), TEXT("FrameTime_RenderThread"), TEXT("FrameTime_RHIThread"), TEXT("FrameTime_GPU"),
		TEXT("Ping"), TEXT("PacketLoss_Incoming"), TEXT("PacketLoss_Outgoing"), TEXT("PacketRate_Incoming"),
		TEXT("PacketRate_Outgoing"), TEXT("PacketSize_Incoming"), TEXT("PacketSize_Outgoing"),
		TEXT("Latency_Total"), TEXT("Latency_Game"), TEXT("Latency_Render"), TEXT("Count")
	};
	DCSettingsFoundationTests::CheckEnum<EDCLyraStatDisplayMode>(*this, TEXT("Display modes"), DisplayModes);
	DCSettingsFoundationTests::CheckEnum<EDCLyraFramePacingMode>(*this, TEXT("Frame pacing modes"), FramePacingModes);
	DCSettingsFoundationTests::CheckEnum<EDCLyraDisplayablePerformanceStat>(*this, TEXT("Performance stats"), PerformanceStats);
	TestEqual(TEXT("Original performance stat Count"), static_cast<int32>(EDCLyraDisplayablePerformanceStat::Count), 18);

	int32 Ordinal = 0;
	for (EDCLyraDisplayablePerformanceStat Stat : TEnumRange<EDCLyraDisplayablePerformanceStat>())
	{
		TestEqual(TEXT("Enum range preserves ordinal order"), static_cast<int32>(Stat), Ordinal++);
		TestTrue(TEXT("Enum range excludes Count"), Stat != EDCLyraDisplayablePerformanceStat::Count);
	}
	TestEqual(TEXT("Enum range includes all 18 real stats"), Ordinal, 18);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCPerformanceSettingsDefaultsTest,
	"DreamCatcher.R5.SettingsFoundation.PerformanceDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCPerformanceSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDCLyraPerformanceSettings> Settings(NewObject<UDCLyraPerformanceSettings>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UDCLyraPlatformSpecificRenderingSettings> Platform(NewObject<UDCLyraPlatformSpecificRenderingSettings>(GetTransientPackage(), NAME_None, RF_Transient));
	const int32 DesktopRates[] = { 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 };
	const int32 MobileRates[] = { 20, 30, 45, 60, 90, 120 };
	DCSettingsFoundationTests::CheckFrameRates(*this, TEXT("Original desktop FPS candidates"), Settings->DesktopFrameRateLimits, DesktopRates);
	DCSettingsFoundationTests::CheckFrameRates(*this, TEXT("Original mobile FPS candidates"), Platform->MobileFrameRateLimits, MobileRates);
	TestEqual(TEXT("Original performance settings category"), Settings->GetCategoryName(), FName(TEXT("Game")));
	TestTrue(TEXT("Original DesktopStyle default"), Platform->FramePacingMode == EDCLyraFramePacingMode::DesktopStyle);
	TestTrue(TEXT("Granular video quality is supported by default"), Platform->bSupportsGranularVideoQualitySettings);
	TestTrue(TEXT("Automatic quality benchmark is supported, but is NOT run here"), Platform->bSupportsAutomaticVideoQualityBenchmark);
	TestTrue(TEXT("No default device profile suffix without Config"), Platform->DefaultDeviceProfileSuffix.IsEmpty());
	TestEqual(TEXT("No configured profile variants"), Platform->UserFacingDeviceProfileOptions.Num(), 0);

	if (TestEqual(TEXT("Original single all-stats group"), Settings->UserFacingPerformanceStats.Num(), 1))
	{
		const FDCLyraPerformanceStatGroup& Group = Settings->UserFacingPerformanceStats[0];
		TestTrue(TEXT("Default visibility query is empty"), Group.VisibilityQuery.IsEmpty());
		TestEqual(TEXT("All 18 real stats enabled in the data"), Group.AllowedStats.Num(), 18);
		for (EDCLyraDisplayablePerformanceStat Stat : TEnumRange<EDCLyraDisplayablePerformanceStat>())
		{
			TestTrue(FString::Printf(TEXT("AllowedStats contains ordinal %d"), static_cast<int32>(Stat)), Group.AllowedStats.Contains(Stat));
		}
		TestFalse(TEXT("Count is not an allowed display stat"), Group.AllowedStats.Contains(EDCLyraDisplayablePerformanceStat::Count));
	}
	const FDCLyraQualityDeviceProfileVariant Variant;
	TestTrue(TEXT("Default variant has no display name"), Variant.DisplayName.IsEmpty());
	TestTrue(TEXT("Default variant has no suffix"), Variant.DeviceProfileSuffix.IsEmpty());
	TestEqual(TEXT("Original variant minimum refresh rate"), Variant.MinRefreshRate, 0);
	TestTrue(TEXT("Original Game config class"), Settings->GetClass()->ClassConfigName == FName(TEXT("Game")));
	TestTrue(TEXT("Original platform Game config class"), Platform->GetClass()->ClassConfigName == FName(TEXT("Game")));
	AddInfo(TEXT("Data defaults only. Original PerPlatformSettings initialization may create platform data objects; no device profile/FPS/benchmark is applied. Actual platform INI overrides remain unverified."));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCAudioSettingsDataContractTest,
	"DreamCatcher.R5.SettingsFoundation.AudioSchemaAndDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCAudioSettingsDataContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDCLyraAudioSettings> Settings(NewObject<UDCLyraAudioSettings>(GetTransientPackage(), NAME_None, RF_Transient));
	const TCHAR* const ReferenceNames[] = {
		TEXT("DefaultControlBusMix"), TEXT("LoadingScreenControlBusMix"), TEXT("UserSettingsControlBusMix"),
		TEXT("OverallVolumeControlBus"), TEXT("MusicVolumeControlBus"), TEXT("SoundFXVolumeControlBus"),
		TEXT("DialogueVolumeControlBus"), TEXT("VoiceChatVolumeControlBus")
	};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(ReferenceNames)); ++Index)
	{
		const FString Label(ReferenceNames[Index]);
		FStructProperty* Property = FindFProperty<FStructProperty>(Settings->GetClass(), FName(ReferenceNames[Index]));
		if (!TestNotNull(Label + TEXT(" exists"), Property)
			|| !TestTrue(Label + TEXT(" is a SoftObjectPath struct"), Property->Struct->GetFName() == FName(TEXT("SoftObjectPath"))))
		{
			continue;
		}
		TestTrue(Label + TEXT(" retains Config flag"), Property->HasAnyPropertyFlags(CPF_Config));
		TestTrue(Label + TEXT(" retains EditAnywhere flag"), Property->HasAnyPropertyFlags(CPF_Edit));
		TestEqual(Label + TEXT(" original class filter"), Property->GetMetaData(TEXT("AllowedClasses")),
			FString(Index < 3 ? TEXT("/Script/AudioModulation.SoundControlBusMix") : TEXT("/Script/AudioModulation.SoundControlBus")));
		TestTrue(Label + TEXT(" has no implicit asset binding"), Property->ContainerPtrToValuePtr<FSoftObjectPath>(Settings.Get())->IsNull());
	}
	TestEqual(TEXT("Original HDR chain list is empty without Config"), Settings->HDRAudioSubmixEffectChain.Num(), 0);
	TestEqual(TEXT("Original LDR chain list is empty without Config"), Settings->LDRAudioSubmixEffectChain.Num(), 0);
	TestTrue(TEXT("Original audio Game config class"), Settings->GetClass()->ClassConfigName == FName(TEXT("Game")));

	for (const TCHAR* Name : { TEXT("HDRAudioSubmixEffectChain"), TEXT("LDRAudioSubmixEffectChain") })
	{
		FArrayProperty* Property = FindFProperty<FArrayProperty>(Settings->GetClass(), FName(Name));
		if (TestNotNull(Name, Property))
		{
			const FStructProperty* Inner = CastField<FStructProperty>(Property->Inner);
			TestTrue(FString(Name) + TEXT(" uses the original-derived chain entry"), Inner && Inner->Struct == FDCLyraSubmixEffectChainMap::StaticStruct());
			TestTrue(FString(Name) + TEXT(" retains Config flag"), Property->HasAnyPropertyFlags(CPF_Config));
		}
	}
	const FDCLyraSubmixEffectChainMap EmptyChain;
	TestTrue(TEXT("Submix reference defaults to null"), EmptyChain.Submix.IsNull());
	TestEqual(TEXT("Effect chain defaults to empty"), EmptyChain.SubmixEffectChain.Num(), 0);
	const FSoftObjectProperty* SubmixProperty = FindFProperty<FSoftObjectProperty>(FDCLyraSubmixEffectChainMap::StaticStruct(), TEXT("Submix"));
	if (TestNotNull(TEXT("Submix remains a typed soft object property"), SubmixProperty))
	{
		TestEqual(TEXT("Submix points to the Engine type"), SubmixProperty->PropertyClass->GetPathName(), FString(TEXT("/Script/Engine.SoundSubmix")));
	}
	const FArrayProperty* ChainProperty = FindFProperty<FArrayProperty>(FDCLyraSubmixEffectChainMap::StaticStruct(), TEXT("SubmixEffectChain"));
	if (TestNotNull(TEXT("SubmixEffectChain remains an array"), ChainProperty))
	{
		const FSoftObjectProperty* Inner = CastField<FSoftObjectProperty>(ChainProperty->Inner);
		if (TestNotNull(TEXT("Effect entries remain typed soft object properties"), Inner))
		{
			TestEqual(TEXT("Effects point to the Engine preset type"), Inner->PropertyClass->GetPathName(), FString(TEXT("/Script/Engine.SoundEffectSubmixPreset")));
		}
	}
	AddInfo(TEXT("Schema/defaults only. No audio assets loaded, no mix/HDR/LDR processing applied, no playback, and no Config saved."));
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
