// Opt-in, unsaved Inspector FK pose preflight. This does not activate a bot,
// retarget animation assets, run PIE, or validate skinning/hand/foot contact.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "AnimPose.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ReferenceSkeleton.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/RootMotionGeneratorOp.h"
#include "Retargeter/RetargetOps/RunIKRigOp.h"
#include "Retargeter/RetargetOps/PinBoneOp.h"
#include "Rig/IKRigDefinition.h"
#include "RigEditor/IKRigController.h"
#include "SkinnedAssetCompiler.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DCInspectorRetargetPreflight
{
constexpr TCHAR ExpectedProject[] = TEXT("E:/Epic Games/UEProjects/DreamCatcher/");
constexpr TCHAR SourceMeshPath[] = TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny.SKM_Manny");
constexpr TCHAR TargetMeshPath[] = TEXT("/Game/DreamCatcher/Main_Charcher/Inspector/Inspector/Inspector.Inspector");
constexpr TCHAR SourceSkeletonPath[] = TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin");
constexpr TCHAR TargetSkeletonPath[] = TEXT("/Game/DreamCatcher/Main_Charcher/Inspector/Inspector/Inspector_Skeleton.Inspector_Skeleton");
constexpr double TransformTolerance = 1.e-5;
constexpr int32 ExpectedClipCount = 10;
constexpr int32 ExpectedSampleCount = 50;
constexpr int32 ExpectedFKChainCount = 21;

const TCHAR* const ClipPaths[] = {
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Idle_Hipfire.MM_Rifle_Idle_Hipfire"),
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Jog_Fwd.MM_Rifle_Jog_Fwd"),
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Crouch_Idle.MM_Rifle_Crouch_Idle"),
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Jump_Apex.MM_Rifle_Jump_Apex"),
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Rifle_Fire.MM_Rifle_Fire"),
	TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/MM_Rifle_Reload.MM_Rifle_Reload"),
	TEXT("/Game/DreamCatcher/Lyra/Dash/MM_Dash_Forward.MM_Dash_Forward"),
	TEXT("/Game/DreamCatcher/Lyra/Dash/MM_Dash_Backward.MM_Dash_Backward"),
	TEXT("/Game/DreamCatcher/Lyra/Dash/MM_Dash_Left.MM_Dash_Left"),
	TEXT("/Game/DreamCatcher/Lyra/Dash/MM_Dash_Right.MM_Dash_Right")
};

struct FChain
{
	FName Name;
	FName SourceStart;
	FName SourceEnd;
	FName TargetStart;
	FName TargetEnd;
};

struct FOutput
{
	FString Directory;
	TArray<FString> Metadata{ TEXT("key,value") };
	TArray<FString> Samples{ TEXT("clip,fraction,time_seconds,delta_seconds,source_bones,target_bones,source_mapped_translation_from_reference_cm,source_mapped_rotation_from_reference_deg,target_translation_from_reference_cm,target_rotation_from_reference_deg,source_mapped_translation_from_first_cm,source_mapped_rotation_from_first_deg,target_translation_from_first_cm,target_rotation_from_first_deg") };
	TArray<FString> Bones{ TEXT("clip,fraction,time_seconds,space,bone,tx,ty,tz,qx,qy,qz,qw,sx,sy,sz") };
	int32 EvaluatedSamples = 0;
	int32 ScaleBearingInputTransforms = 0;
	double MaxBakeTranslationError = 0.0;
	double MaxBakeRotationErrorRadians = 0.0;
	double MaxLocalTranslationCompensation = 0.0;
};

FString Csv(const FString& Value)
{
	return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
}

void Meta(FOutput& Output, const FString& Key, const FString& Value)
{
	Output.Metadata.Add(Csv(Key) + TEXT(",") + Csv(Value));
}

TSet<FString> DirtyPackages()
{
	TArray<UPackage*> Packages;
	FEditorFileUtils::GetDirtyPackages(Packages);
	TSet<FString> Names;
	for (const UPackage* Package : Packages)
	{
		if (Package && Package != GetTransientPackage())
		{
			Names.Add(Package->GetName());
		}
	}
	return Names;
}

bool CheckClean(FAutomationTestBase& Test, const TCHAR* Phase)
{
	const TSet<FString> Names = DirtyPackages();
	for (const FString& Name : Names)
	{
		Test.AddError(FString::Printf(TEXT("%s: dirty original package %s; never save this process."), Phase, *Name));
	}
	return Names.IsEmpty();
}

bool ValidTransform(const FTransform& Transform)
{
	const FVector Translation = Transform.GetTranslation();
	const FVector Scale = Transform.GetScale3D();
	const FQuat Rotation = Transform.GetRotation();
	return FMath::IsFinite(Translation.X) && FMath::IsFinite(Translation.Y) && FMath::IsFinite(Translation.Z)
		&& FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y) && FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W)
		&& FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
		&& Rotation.IsNormalized()
		&& FMath::Abs(Scale.X) > UE_SMALL_NUMBER && FMath::Abs(Scale.Y) > UE_SMALL_NUMBER && FMath::Abs(Scale.Z) > UE_SMALL_NUMBER;
}

bool ValidatePose(FAutomationTestBase& Test, const TArray<FTransform>& Pose, const TArray<FName>& Names, const FString& Label)
{
	if (!Test.TestEqual(Label + TEXT(" transform/name count"), Pose.Num(), Names.Num())) { return false; }
	for (int32 Index = 0; Index < Pose.Num(); ++Index)
	{
		if (!ValidTransform(Pose[Index]))
		{
			Test.AddError(Label + TEXT(" has nonfinite, nonnormalized or zero-scale bone ") + Names[Index].ToString());
			return false;
		}
	}
	return true;
}

void RecordBones(FOutput& Output, const FString& Clip, double Fraction, double Time, const TCHAR* Space,
	const TArray<FName>& Names, const TArray<FTransform>& Pose)
{
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		const FVector Translation = Pose[Index].GetTranslation();
		const FQuat Rotation = Pose[Index].GetRotation();
		const FVector Scale = Pose[Index].GetScale3D();
		Output.Bones.Add(FString::Printf(TEXT("%s,%.9f,%.9f,%s,%s,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f"),
			*Csv(Clip), Fraction, Time, Space, *Csv(Names[Index].ToString()),
			Translation.X, Translation.Y, Translation.Z, Rotation.X, Rotation.Y, Rotation.Z, Rotation.W, Scale.X, Scale.Y, Scale.Z));
	}
}

bool BakeInputScaleIntoTranslations(FAutomationTestBase& Test, const FRetargetSkeleton& Skeleton,
	const TArray<FTransform>& OriginalGlobal, TArray<FTransform>& UnitGlobal, TArray<FTransform>& BakedLocal, FOutput& Output)
{
	if (!ValidatePose(Test, OriginalGlobal, Skeleton.BoneNames, TEXT("Before inherited-scale bake"))
		|| !Test.TestEqual(TEXT("Scale-bake parent/name count"), Skeleton.ParentIndices.Num(), Skeleton.BoneNames.Num())) { return false; }
	UnitGlobal.SetNum(OriginalGlobal.Num());
	BakedLocal.SetNum(OriginalGlobal.Num());
	for (int32 Index = 0; Index < OriginalGlobal.Num(); ++Index)
	{
		const int32 Parent = Skeleton.ParentIndices[Index];
		if (!Test.TestTrue(TEXT("Scale-bake parent-before-child order"), Parent == INDEX_NONE || (Parent >= 0 && Parent < Index))) { return false; }
		FTransform Local = Skeleton.GetLocalTransformOfSingleBone(Index, OriginalGlobal);
		const FVector OriginalLocalTranslation = Local.GetTranslation();
		if (Parent != INDEX_NONE)
		{
			// FTransform composition is Qp * (Sp * Tl) + Tp. Keep the
			// inherited scale in each local offset before using a unit parent.
			// OriginalGlobal already includes its effect on child bone origins.
			Local.ScaleTranslation(OriginalGlobal[Parent].GetScale3D());
		}
		BakedLocal[Index] = FTransform(Local.GetRotation(), Local.GetTranslation(), FVector::OneVector);
		UnitGlobal[Index] = Parent == INDEX_NONE ? BakedLocal[Index] : BakedLocal[Index] * UnitGlobal[Parent];
		const double TranslationError = FVector::Distance(UnitGlobal[Index].GetTranslation(), OriginalGlobal[Index].GetTranslation());
		const double RotationError = UnitGlobal[Index].GetRotation().AngularDistance(OriginalGlobal[Index].GetRotation());
		Output.MaxBakeTranslationError = FMath::Max(Output.MaxBakeTranslationError, TranslationError);
		Output.MaxBakeRotationErrorRadians = FMath::Max(Output.MaxBakeRotationErrorRadians, RotationError);
		Output.MaxLocalTranslationCompensation = FMath::Max(Output.MaxLocalTranslationCompensation,
			FVector::Distance(Local.GetTranslation(), OriginalLocalTranslation));
		if (!OriginalGlobal[Index].GetScale3D().Equals(FVector::OneVector, 0.0)) { ++Output.ScaleBearingInputTransforms; }
		if (!Test.TestTrue(TEXT("Scale bake preserves original component bone position: ") + Skeleton.BoneNames[Index].ToString(),
			TranslationError <= 1.e-4)
			|| !Test.TestTrue(TEXT("Scale bake preserves original component bone rotation: ") + Skeleton.BoneNames[Index].ToString(),
			RotationError <= 1.e-5)
			|| !Test.TestTrue(TEXT("Processor input is exactly unit scale: ") + Skeleton.BoneNames[Index].ToString(),
			UnitGlobal[Index].GetScale3D().Equals(FVector::OneVector, 0.0))) { return false; }
	}
	// This preserves joint positions/orientations and inherited bone offsets;
	// it does not claim to preserve scale-driven skin/weapon volume deformation.
	return ValidatePose(Test, BakedLocal, Skeleton.BoneNames, TEXT("Scale-baked local input"))
		&& ValidatePose(Test, UnitGlobal, Skeleton.BoneNames, TEXT("Unit component processor input"));
}

struct FChange
{
	double Translation = 0.0;
	double RotationDegrees = 0.0;
	bool bDifferent = false;
};

FChange Changes(const TArray<FTransform>& Pose, const TArray<FTransform>& Baseline, const TArray<bool>* Mapped = nullptr)
{
	FChange Result;
	for (int32 Index = 0; Index < Pose.Num(); ++Index)
	{
		if (Mapped && !(*Mapped)[Index]) { continue; }
		Result.Translation = FMath::Max(Result.Translation, FVector::Distance(Pose[Index].GetTranslation(), Baseline[Index].GetTranslation()));
		Result.RotationDegrees = FMath::Max(Result.RotationDegrees,
			FMath::RadiansToDegrees(Pose[Index].GetRotation().AngularDistance(Baseline[Index].GetRotation())));
		Result.bDifferent |= !Pose[Index].Equals(Baseline[Index], TransformTolerance);
	}
	return Result;
}

bool ReferencePose(FAutomationTestBase& Test, const USkeletalMesh* Mesh, const FRetargetSkeleton& Resolved,
	TArray<FTransform>& OutPose, const FString& Label)
{
	const FReferenceSkeleton& Reference = Mesh->GetRefSkeleton();
	const TArray<FTransform>& Local = Reference.GetRefBonePose();
	TArray<FTransform> Global;
	Global.SetNum(Local.Num());
	for (int32 Index = 0; Index < Local.Num(); ++Index)
	{
		const int32 Parent = Reference.GetParentIndex(Index);
		if (!Test.TestTrue(Label + TEXT(" parent-before-child order"), Parent == INDEX_NONE || (Parent >= 0 && Parent < Index))) { return false; }
		Global[Index] = Parent == INDEX_NONE ? Local[Index] : Local[Index] * Global[Parent];
	}
	OutPose.Reset(Resolved.BoneNames.Num());
	for (int32 Index = 0; Index < Resolved.BoneNames.Num(); ++Index)
	{
		const int32 ReferenceIndex = Reference.FindBoneIndex(Resolved.BoneNames[Index]);
		if (!Test.TestTrue(Label + TEXT(" resolved bone exists in reference"), ReferenceIndex != INDEX_NONE)) { return false; }
		const FName ParentName = Reference.GetParentIndex(ReferenceIndex) == INDEX_NONE ? NAME_None
			: Reference.GetBoneName(Reference.GetParentIndex(ReferenceIndex));
		const int32 ResolvedParent = Resolved.ParentIndices[Index];
		if (!Test.TestTrue(Label + TEXT(" resolved parent index"), ResolvedParent == INDEX_NONE || (ResolvedParent >= 0 && ResolvedParent < Index))) { return false; }
		const FName ResolvedParentName = ResolvedParent == INDEX_NONE ? NAME_None : Resolved.BoneNames[ResolvedParent];
		if (!Test.TestEqual(Label + TEXT(" resolved parent name"), ResolvedParentName, ParentName)) { return false; }
		OutPose.Add(Global[ReferenceIndex]);
	}
	return ValidatePose(Test, OutPose, Resolved.BoneNames, Label);
}

bool AnimationPose(FAutomationTestBase& Test, UAnimSequence* Sequence, USkeletalMesh* SourceMesh,
	double Time, const FRetargetSkeleton& Resolved, TArray<FTransform>& OutPose, int32 FrameIndex = INDEX_NONE)
{
	FAnimPoseEvaluationOptions Options;
	Options.EvaluationType = EAnimDataEvalType::Raw;
	Options.OptionalSkeletalMesh = SourceMesh;
	Options.bShouldRetarget = true;
	Options.bRetrieveAdditiveAsFullPose = true;
	Options.bExtractRootMotion = false;
	Options.bIncorporateRootMotionIntoPose = true;
	Options.bEvaluateCurves = true;
	FAnimPose Evaluated;
	if (FrameIndex == INDEX_NONE) { UAnimPoseExtensions::GetAnimPoseAtTime(Sequence, Time, Options, Evaluated); }
	else { UAnimPoseExtensions::GetAnimPoseAtFrame(Sequence, FrameIndex, Options, Evaluated); }
	if (!Test.TestTrue(TEXT("Source animation pose is valid"), Evaluated.IsValid())) { return false; }
	TArray<FName> EvaluatedNames;
	UAnimPoseExtensions::GetBoneNames(Evaluated, EvaluatedNames);
	TSet<FName> UniqueNames;
	for (const FName Name : EvaluatedNames)
	{
		if (!Test.TestFalse(TEXT("Source pose has no duplicate bone names"), UniqueNames.Contains(Name))) { return false; }
		UniqueNames.Add(Name);
	}
	if (!Test.TestTrue(TEXT("Source pose contains all resolved input bones"), EvaluatedNames.Num() >= Resolved.BoneNames.Num())) { return false; }
	// Align by name into the processor's required order. An absent name is an error;
	// GetBonePose's documented identity fallback must never stand in for a bone.
	for (const FName Name : Resolved.BoneNames)
	{
		if (!Test.TestTrue(TEXT("Source pose contains required bone ") + Name.ToString(), UniqueNames.Contains(Name))) { return false; }
	}
	OutPose.Reset(Resolved.BoneNames.Num());
	for (const FName Name : Resolved.BoneNames)
	{
		OutPose.Add(UAnimPoseExtensions::GetBonePose(Evaluated, Name, EAnimPoseSpaces::World));
	}
	return ValidatePose(Test, OutPose, Resolved.BoneNames, Sequence->GetName());
}

TArray<FChain> Chains()
{
	TArray<FChain> Result{
		{ TEXT("Spine"), TEXT("spine_01"), TEXT("spine_05"), TEXT("spine_01"), TEXT("spine_03") },
		{ TEXT("Neck"), TEXT("neck_01"), TEXT("neck_02"), TEXT("neck_01"), TEXT("neck_01") },
		{ TEXT("Head"), TEXT("head"), TEXT("head"), TEXT("head"), TEXT("head") }
	};
	for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
	{
		auto Bone = [Side](const TCHAR* Name) { return FName(*(FString(Name) + TEXT("_") + Side)); };
		Result.Add({ Bone(TEXT("Clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")) });
		Result.Add({ Bone(TEXT("Arm")), Bone(TEXT("upperarm")), Bone(TEXT("hand")), Bone(TEXT("upperarm")), Bone(TEXT("hand")) });
		Result.Add({ Bone(TEXT("Leg")), Bone(TEXT("thigh")), Bone(TEXT("foot")), Bone(TEXT("thigh")), Bone(TEXT("foot")) });
		Result.Add({ Bone(TEXT("Foot")), Bone(TEXT("foot")), Bone(TEXT("ball")), Bone(TEXT("foot")), Bone(TEXT("ball")) });
		for (const TCHAR* Digit : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
		{
			FString Name(Digit);
			Name[0] = FChar::ToUpper(Name[0]);
			Result.Add({ Bone(*Name), Bone(*(FString(Digit) + TEXT("_01"))), Bone(*(FString(Digit) + TEXT("_03"))),
				Bone(*(FString(Digit) + TEXT("_01"))), Bone(*(FString(Digit) + TEXT("_03"))) });
		}
	}
	return Result;
}

bool ConfigureRig(FAutomationTestBase& Test, UIKRigDefinition* Rig, USkeletalMesh* Mesh, bool bSource, const TArray<FChain>& Definitions)
{
	const UIKRigController* Controller = UIKRigController::GetController(Rig);
	if (!Test.TestNotNull(TEXT("Transient rig controller"), Controller)) { return false; }
	if (!Test.TestTrue(TEXT("Rig assigns its mesh"), Controller->SetSkeletalMesh(Mesh))
		|| !Test.TestTrue(TEXT("Rig mesh compatibility"), Controller->IsSkeletalMeshCompatible(Mesh))
		|| !Test.TestTrue(TEXT("Rig pelvis assignment"), Controller->SetRetargetRoot(TEXT("pelvis")))
		|| !Test.TestTrue(TEXT("Rig root-motion bone assignment"), Controller->SetRootMotionBone(TEXT("root")))) { return false; }
	if (!Test.TestEqual(TEXT("Rig pelvis readback"), Controller->GetRetargetRoot(), FName(TEXT("pelvis")))
		|| !Test.TestEqual(TEXT("Rig root-motion readback"), Controller->GetRootMotionBone(), FName(TEXT("root")))) { return false; }
	const FReferenceSkeleton& Reference = Mesh->GetRefSkeleton();
	for (const FChain& Chain : Definitions)
	{
		const FName Start = bSource ? Chain.SourceStart : Chain.TargetStart;
		const FName End = bSource ? Chain.SourceEnd : Chain.TargetEnd;
		const int32 StartIndex = Reference.FindBoneIndex(Start);
		int32 Index = Reference.FindBoneIndex(End);
		if (!Test.TestTrue(TEXT("Chain endpoints exist"), StartIndex != INDEX_NONE && Index != INDEX_NONE)) { return false; }
		while (Index != INDEX_NONE && Index != StartIndex) { Index = Reference.GetParentIndex(Index); }
		if (!Test.TestEqual(TEXT("Chain endpoint ancestry"), Index, StartIndex)
			|| !Test.TestEqual(TEXT("Chain creation name"), Controller->AddRetargetChain(Chain.Name, Start, End, NAME_None), Chain.Name)
			|| !Test.TestEqual(TEXT("Chain start readback"), Controller->GetRetargetChainStartBone(Chain.Name), Start)
			|| !Test.TestEqual(TEXT("Chain end readback"), Controller->GetRetargetChainEndBone(Chain.Name), End)
			|| !Test.TestTrue(TEXT("No IK goal authored"), Controller->GetRetargetChainGoal(Chain.Name).IsNone())) { return false; }
	}
	return Test.TestEqual(TEXT("Rig explicit body FK chain count; root is handled by Root Motion"), Controller->GetRetargetChains().Num(), ExpectedFKChainCount)
		&& Test.TestEqual(TEXT("Rig has no IK goals"), Rig->GetGoalArray().Num(), 0)
		&& Test.TestEqual(TEXT("Rig has no solvers"), Rig->GetSolverStructs().Num(), 0);
}

bool ProcessorMessages(FAutomationTestBase& Test, const FIKRetargetProcessor& Processor)
{
	for (const FText& Error : Processor.Log.GetErrors()) { Test.AddError(TEXT("IK processor: ") + Error.ToString()); }
	for (const FText& Warning : Processor.Log.GetWarnings()) { Test.AddWarning(TEXT("IK processor: ") + Warning.ToString()); }
	return Processor.Log.GetErrors().IsEmpty() && Processor.Log.GetWarnings().IsEmpty();
}

bool EvaluatePrivate(FAutomationTestBase& Test, FOutput& Output);

bool Evaluate(FAutomationTestBase& Test, FOutput& Output)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DCInspectorRetargetPrivate"))) { return EvaluatePrivate(Test, Output); }
	USkeletalMesh* SourceMesh = LoadObject<USkeletalMesh>(nullptr, SourceMeshPath);
	USkeletalMesh* TargetMesh = LoadObject<USkeletalMesh>(nullptr, TargetMeshPath);
	if (!Test.TestNotNull(TEXT("Existing Manny mesh"), SourceMesh) || !Test.TestNotNull(TEXT("Existing Inspector mesh"), TargetMesh)) { return false; }
	Meta(Output, TEXT("source_mesh_was_compiling"), SourceMesh->IsCompiling() ? TEXT("true") : TEXT("false"));
	Meta(Output, TEXT("target_mesh_was_compiling"), TargetMesh->IsCompiling() ? TEXT("true") : TEXT("false"));
	// Resolve only these meshes' pending derived-data builds before reading
	// skeletons/poses or constructing rigs. This waits; it never saves a package.
	USkinnedAsset* const MeshesToFinish[] = { SourceMesh, TargetMesh };
	FSkinnedAssetCompilingManager::Get().FinishCompilation(MakeArrayView(MeshesToFinish));
	if (!Test.TestFalse(TEXT("Manny async mesh compilation completed"), SourceMesh->IsCompiling())
		|| !Test.TestFalse(TEXT("Inspector async mesh compilation completed"), TargetMesh->IsCompiling())
		|| !CheckClean(Test, TEXT("After mesh readiness wait"))) { return false; }
	Meta(Output, TEXT("source_and_target_meshes_ready"), TEXT("true"));
	if (!Test.TestEqual(TEXT("Manny skeleton path"), GetPathNameSafe(SourceMesh->GetSkeleton()), FString(SourceSkeletonPath))
		|| !Test.TestEqual(TEXT("Inspector skeleton path"), GetPathNameSafe(TargetMesh->GetSkeleton()), FString(TargetSkeletonPath))
		|| !Test.TestEqual(TEXT("Manny final reference bone count, including virtual bones"), SourceMesh->GetRefSkeleton().GetNum(), 164)
		|| !Test.TestEqual(TEXT("Inspector final reference bone count"), TargetMesh->GetRefSkeleton().GetNum(), 61)
		|| !CheckClean(Test, TEXT("After mesh load"))) { return false; }

	TStrongObjectPtr<UIKRigDefinition> SourceRig(NewObject<UIKRigDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UIKRigDefinition> TargetRig(NewObject<UIKRigDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
	TStrongObjectPtr<UIKRetargeter> Retargeter(NewObject<UIKRetargeter>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!Test.TestTrue(TEXT("Only transient rig/retargeter outers"), SourceRig->GetOuter() == GetTransientPackage()
		&& TargetRig->GetOuter() == GetTransientPackage() && Retargeter->GetOuter() == GetTransientPackage())) { return false; }
	const TArray<FChain> Definitions = Chains();
	if (!Test.TestEqual(TEXT("Exactly 21 body FK definitions; no zero-length root chain"), Definitions.Num(), ExpectedFKChainCount)
		|| !ConfigureRig(Test, SourceRig.Get(), SourceMesh, true, Definitions)
		|| !ConfigureRig(Test, TargetRig.Get(), TargetMesh, false, Definitions)) { return false; }
	const UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter.Get());
	if (!Test.TestNotNull(TEXT("Transient retargeter controller"), Controller)) { return false; }
	Controller->SetIKRig(ERetargetSourceOrTarget::Source, SourceRig.Get());
	Controller->SetIKRig(ERetargetSourceOrTarget::Target, TargetRig.Get());
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Source, SourceMesh);
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Target, TargetMesh);
	Controller->AddDefaultOps();
	Controller->AssignIKRigToAllOps(ERetargetSourceOrTarget::Source, SourceRig.Get());
	Controller->AssignIKRigToAllOps(ERetargetSourceOrTarget::Target, TargetRig.Get());
	if (!Test.TestEqual(TEXT("Documented current default op count"), Controller->GetNumRetargetOps(), 5)) { return false; }
	int32 FKIndex = INDEX_NONE;
	int32 RunIKIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Retargeter->GetRetargetOps().Num(); ++Index)
	{
		const UScriptStruct* Type = Retargeter->GetRetargetOps()[Index].GetScriptStruct();
		if (Type == FIKRetargetFKChainsOp::StaticStruct()) { FKIndex = Index; }
		if (Type == FIKRetargetRunIKRigOp::StaticStruct()) { RunIKIndex = Index; }
	}
	if (!Test.TestTrue(TEXT("Default FK and Run IK Rig ops found by type"), FKIndex != INDEX_NONE && RunIKIndex != INDEX_NONE)) { return false; }
	const FName FKName = Controller->GetOpName(FKIndex);
	for (const FChain& Chain : Definitions)
	{
		if (!Test.TestTrue(TEXT("Explicit FK mapping ") + Chain.Name.ToString(), Controller->SetSourceChain(Chain.Name, Chain.Name, FKName))
			|| !Test.TestEqual(TEXT("Per-op FK mapping readback"), Controller->GetSourceChain(Chain.Name, FKName), Chain.Name)) { return false; }
	}
	if (!Test.TestTrue(TEXT("Disable only transient Run IK Rig because no solver is authored"), Controller->SetRetargetOpEnabled(RunIKIndex, false))) { return false; }
	for (int32 Index = 0; Index < Controller->GetNumRetargetOps(); ++Index)
	{
		const bool bExpectedEnabled = Index != RunIKIndex;
		if (!Test.TestEqual(TEXT("Transient op enabled state"), Controller->GetRetargetOpEnabled(Index), bExpectedEnabled)) { return false; }
		Meta(Output, TEXT("op_") + FString::FromInt(Index), Controller->GetOpName(Index).ToString() + (bExpectedEnabled ? TEXT(":enabled") : TEXT(":disabled_no_solver")));
	}

	FIKRetargetProcessor Processor;
	FRetargetInitParameters Init;
	Init.SourceSkeletalMesh = SourceMesh;
	Init.TargetSkeletalMesh = TargetMesh;
	Init.RetargeterAsset = Retargeter.Get();
	Init.CustomProfile = Retargeter->GetCurrentProfile();
	Init.bSuppressWarnings = false;
	Processor.Initialize(Init);
	if (!ProcessorMessages(Test, Processor)
		|| !Test.TestTrue(TEXT("Modern IK processor initialized"), Processor.IsInitialized())
		|| !Test.TestTrue(TEXT("Processor uses exactly the approved transient assets"), Processor.WasInitializedWithTheseAssets(SourceMesh, TargetMesh, Retargeter.Get()))) { return false; }
	Meta(Output, TEXT("native_source_scale_factor"), FString::Printf(TEXT("%.12f"), Processor.GetSourceScaleFactor()));
	const FRetargetSkeleton& Source = Processor.GetSkeleton(ERetargetSourceOrTarget::Source);
	const FRetargetSkeleton& Target = Processor.GetSkeleton(ERetargetSourceOrTarget::Target);
	if (!Test.TestEqual(TEXT("Resolved source name/parent count"), Source.BoneNames.Num(), Source.ParentIndices.Num())
		|| !Test.TestEqual(TEXT("Resolved target name/parent count"), Target.BoneNames.Num(), Target.ParentIndices.Num())
		|| !Test.TestEqual(TEXT("Resolved target has 61 bones"), Target.BoneNames.Num(), 61)) { return false; }
	const FIKRetargetFKChainsOp* FKOp = Processor.GetFirstRetargetOpOfType<FIKRetargetFKChainsOp>();
	if (!Test.TestNotNull(TEXT("Resolved FK operation"), FKOp)
		|| !Test.TestTrue(TEXT("FK operation is active and initialized"), FKOp->IsEnabled() && FKOp->IsInitialized())) { return false; }
	const FIKRetargetRootMotionOp* RootMotion = Processor.GetFirstRetargetOpOfType<FIKRetargetRootMotionOp>();
	if (!Test.TestNotNull(TEXT("Dedicated resolved Root Motion operation"), RootMotion)
		|| !Test.TestTrue(TEXT("Root Motion operation is active and initialized"), RootMotion->IsEnabled() && RootMotion->IsInitialized())
		|| !Test.TestEqual(TEXT("Root Motion directly reads the source root bone"), RootMotion->Settings.SourceRoot.BoneName, FName(TEXT("root")))
		|| !Test.TestEqual(TEXT("Root Motion directly drives the target root bone"), RootMotion->Settings.TargetRoot.BoneName, FName(TEXT("root")))
		|| !Test.TestTrue(TEXT("Root Motion keeps its source-root-copy policy"), RootMotion->Settings.RootMotionSource == ERootMotionSource::CopyFromSourceRoot)) { return false; }
	Meta(Output, TEXT("root_motion_verified"), TEXT("enabled; initialized; SourceRoot=root; TargetRoot=root; CopyFromSourceRoot; root poses recorded for reference and each completed sample"));
	for (const FName Required : { FName(TEXT("root")), FName(TEXT("pelvis")), FName(TEXT("head")), FName(TEXT("hand_l")), FName(TEXT("hand_r")), FName(TEXT("foot_l")), FName(TEXT("foot_r")) })
	{
		if (!Test.TestTrue(TEXT("Required source bone ") + Required.ToString(), Source.BoneNames.Contains(Required))
			|| !Test.TestTrue(TEXT("Required target bone ") + Required.ToString(), Target.BoneNames.Contains(Required))
			|| !Test.TestTrue(TEXT("Required target bone mapped or explicitly driven by Root Motion: ") + Required.ToString(),
				Required == RootMotion->Settings.TargetRoot.BoneName || Processor.IsBoneMapped(Required, ERetargetSourceOrTarget::Target))) { return false; }
	}
	TArray<bool> SourceMapped;
	for (const FName Name : Source.BoneNames)
	{
		SourceMapped.Add(Name == RootMotion->Settings.SourceRoot.BoneName || Processor.IsBoneMapped(Name, ERetargetSourceOrTarget::Source));
	}
	TArray<FTransform> SourceReference;
	TArray<FTransform> TargetBindReference;
	if (!ReferencePose(Test, SourceMesh, Source, SourceReference, TEXT("Manny reference"))
		|| !ReferencePose(Test, TargetMesh, Target, TargetBindReference, TEXT("Inspector bind reference"))) { return false; }
	auto Run = [&Processor, &Retargeter, &Test, &Source, &Output](TArray<FTransform>& SourcePose, float DeltaTime,
		TArray<FTransform>& Converted, const FString& Clip, double Fraction, double Time)
	{
		// The processor's public contract requires this once per fresh source
		// pose. Call only on working copies, preserving the sampled evidence.
		Processor.ApplySourceScaleToPose(SourcePose);
		if (!ValidatePose(Test, SourcePose, Source.BoneNames, TEXT("Scaled working input"))) { return false; }
		TArray<FTransform> UnitInput;
		TArray<FTransform> BakedLocal;
		if (!BakeInputScaleIntoTranslations(Test, Source, SourcePose, UnitInput, BakedLocal, Output)) { return false; }
		RecordBones(Output, Clip, Fraction, Time, TEXT("processor_scale_baked_local"), Source.BoneNames, BakedLocal);
		RecordBones(Output, Clip, Fraction, Time, TEXT("processor_input_unit_component"), Source.BoneNames, UnitInput);
		FRetargetRunParameters Parameters;
		Parameters.SourceGlobalPose = &UnitInput;
		Parameters.Profile = Retargeter->GetCurrentProfile();
		Parameters.OverrideSetsToApply = Retargeter->GetOverrideSetsToApply();
		Parameters.DeltaTime = DeltaTime;
		Parameters.LOD = 0;
		Converted = Processor.RunRetargeter(Parameters);
		return true;
	};
	Processor.OnPlaybackReset();
	TArray<FTransform> ReferenceInput = SourceReference;
	TArray<FTransform> TargetReference;
	if (!Run(ReferenceInput, 0.0f, TargetReference, TEXT("REFERENCE"), 0.0, 0.0)) { return false; }
	if (!ValidatePose(Test, TargetReference, Target.BoneNames, TEXT("Retargeted reference")) || !ProcessorMessages(Test, Processor)) { return false; }
	RecordBones(Output, TEXT("REFERENCE"), 0.0, 0.0, TEXT("source_component"), Source.BoneNames, SourceReference);
	RecordBones(Output, TEXT("REFERENCE"), 0.0, 0.0, TEXT("target_component"), Target.BoneNames, TargetReference);
	RecordBones(Output, TEXT("BIND_REFERENCE"), 0.0, 0.0, TEXT("target_component"), Target.BoneNames, TargetBindReference);

	for (const TCHAR* ClipPath : ClipPaths)
	{
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, ClipPath);
		if (!Test.TestNotNull(TEXT("Allowlisted source sequence"), Sequence)
			|| !Test.TestTrue(TEXT("Sequence uses source skeleton"), Sequence->GetSkeleton() == SourceMesh->GetSkeleton())) { return false; }
		const double Duration = Sequence->GetPlayLength();
		if (!Test.TestTrue(TEXT("Finite positive sequence duration"), FMath::IsFinite(Duration) && Duration > 0.0)) { return false; }
		Meta(Output, TEXT("clip_duration_") + Sequence->GetName(), FString::Printf(TEXT("%.9f"), Duration));
		Processor.OnPlaybackReset();
		TArray<FTransform> FirstInput;
		TArray<FTransform> FirstOutput;
		bool bSourceOutsideReference = false;
		bool bOutputOutsideReference = false;
		bool bSourceVaries = false;
		bool bOutputVaries = false;
		double PreviousTime = 0.0;
		for (int32 Sample = 0; Sample < 5; ++Sample)
		{
			const double Fraction = static_cast<double>(Sample) / 4.0;
			const double Time = Duration * Fraction;
			const double DeltaTime = Sample == 0 ? 0.0 : Time - PreviousTime;
			if (!Test.TestTrue(TEXT("Sample time/delta within sequence"), FMath::IsFinite(Time) && Time >= 0.0 && Time <= Duration && DeltaTime >= 0.0)) { return false; }
			TArray<FTransform> Input;
			if (!AnimationPose(Test, Sequence, SourceMesh, Time, Source, Input)) { return false; }
			// RunRetargeter may use the input array as working storage; keep the
			// evaluated source separately for the evidence and variation checks.
			TArray<FTransform> WorkingInput = Input;
			TArray<FTransform> Converted;
			if (!Run(WorkingInput, static_cast<float>(DeltaTime), Converted, Sequence->GetPathName(), Fraction, Time)) { return false; }
			if (!ValidatePose(Test, Converted, Target.BoneNames, Sequence->GetName() + TEXT(" target")) || !ProcessorMessages(Test, Processor)) { return false; }
			if (Sample == 0) { FirstInput = Input; FirstOutput = Converted; }
			const FChange SourceFromReference = Changes(Input, SourceReference, &SourceMapped);
			const FChange TargetFromReference = Changes(Converted, TargetReference);
			const FChange SourceFromFirst = Changes(Input, FirstInput, &SourceMapped);
			const FChange TargetFromFirst = Changes(Converted, FirstOutput);
			bSourceOutsideReference |= SourceFromReference.bDifferent;
			bOutputOutsideReference |= TargetFromReference.bDifferent;
			bSourceVaries |= SourceFromFirst.bDifferent;
			bOutputVaries |= TargetFromFirst.bDifferent;
			Output.Samples.Add(FString::Printf(TEXT("%s,%.9f,%.9f,%.9f,%d,%d,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f"),
				*Csv(Sequence->GetPathName()), Fraction, Time, DeltaTime, Input.Num(), Converted.Num(),
				SourceFromReference.Translation, SourceFromReference.RotationDegrees, TargetFromReference.Translation, TargetFromReference.RotationDegrees,
				SourceFromFirst.Translation, SourceFromFirst.RotationDegrees, TargetFromFirst.Translation, TargetFromFirst.RotationDegrees));
			RecordBones(Output, Sequence->GetPathName(), Fraction, Time, TEXT("source_component"), Source.BoneNames, Input);
			RecordBones(Output, Sequence->GetPathName(), Fraction, Time, TEXT("target_component"), Target.BoneNames, Converted);
			++Output.EvaluatedSamples;
			PreviousTime = Time;
		}
		if (!Test.TestTrue(Sequence->GetName() + TEXT(" does not stay at retargeted reference for nonreference mapped input"), !bSourceOutsideReference || bOutputOutsideReference)
			|| !Test.TestTrue(Sequence->GetName() + TEXT(" does not freeze while mapped source bones vary"), !bSourceVaries || bOutputVaries)
			|| !CheckClean(Test, TEXT("After sequence evaluation"))) { return false; }
		UE_LOG(LogTemp, Display, TEXT("INSPECTOR_POSE_CLIP %s samples=5 FKOnly=true rootDisplacementMayBeZero=true"), *Sequence->GetName());
	}
	return Test.TestEqual(TEXT("All allowlisted clips sampled"), static_cast<int32>(UE_ARRAY_COUNT(ClipPaths)), ExpectedClipCount)
		&& Test.TestEqual(TEXT("Exactly fifty animation poses evaluated"), Output.EvaluatedSamples, ExpectedSampleCount);
}

bool EvaluatePrivate(FAutomationTestBase& Test, FOutput& Output)
{
	constexpr TCHAR PrivateMeshPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector.Inspector");
	constexpr TCHAR PrivateSkeletonPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector_Skeleton.Inspector_Skeleton");
	constexpr TCHAR PermanentRetargeter[] = TEXT("/Game/DreamCatcher/AI/Inspector/Rig/RTG_DC_Manny_Inspector.RTG_DC_Manny_Inspector");
	USkeletalMesh* SourceMesh = LoadObject<USkeletalMesh>(nullptr, SourceMeshPath);
	USkeletalMesh* Original = LoadObject<USkeletalMesh>(nullptr, TargetMeshPath);
	USkeletalMesh* TargetMesh = LoadObject<USkeletalMesh>(nullptr, PrivateMeshPath);
	UIKRetargeter* Retargeter = LoadObject<UIKRetargeter>(nullptr, PermanentRetargeter);
	if (!Test.TestNotNull(TEXT("Private source mesh"), SourceMesh)
		|| !Test.TestNotNull(TEXT("Protected original Inspector"), Original)
		|| !Test.TestNotNull(TEXT("Private Inspector mesh"), TargetMesh)
		|| !Test.TestNotNull(TEXT("Permanent private retargeter"), Retargeter)) { return false; }
	USkinnedAsset* const Waiting[] = { SourceMesh, Original, TargetMesh };
	FSkinnedAssetCompilingManager::Get().FinishCompilation(MakeArrayView(Waiting));
	const FReferenceSkeleton& A = Original->GetRefSkeleton();
	const FReferenceSkeleton& B = TargetMesh->GetRefSkeleton();
	if (!Test.TestEqual(TEXT("Protected original remains 61 raw bones"), A.GetRawBoneNum(), 61)
		|| !Test.TestEqual(TEXT("Private has exactly 69 raw bones"), B.GetRawBoneNum(), 69)
		|| !Test.TestEqual(TEXT("Private has exactly three virtual bones"), B.GetNum(), 72)
		|| !Test.TestEqual(TEXT("Private skeleton identity"), GetPathNameSafe(TargetMesh->GetSkeleton()), FString(PrivateSkeletonPath))) { return false; }
	for (int32 I = 0; I < 61; ++I)
	{
		if (!Test.TestEqual(TEXT("Original bone name preserved"), A.GetRawRefBoneInfo()[I].Name, B.GetRawRefBoneInfo()[I].Name)
			|| !Test.TestEqual(TEXT("Original bone parent preserved"), A.GetRawRefBoneInfo()[I].ParentIndex, B.GetRawRefBoneInfo()[I].ParentIndex)
			|| !Test.TestTrue(TEXT("Original bind transform preserved exactly"), A.GetRawRefBonePose()[I].Equals(B.GetRawRefBonePose()[I], 0.0f))) { return false; }
	}
	const UIKRigDefinition* SourceRig = Retargeter->GetIKRig(ERetargetSourceOrTarget::Source);
	const UIKRigDefinition* TargetRig = Retargeter->GetIKRig(ERetargetSourceOrTarget::Target);
	if (!Test.TestNotNull(TEXT("Permanent source rig"), SourceRig) || !Test.TestNotNull(TEXT("Permanent target rig"), TargetRig)
		|| !Test.TestEqual(TEXT("Permanent source rig binding"), UIKRigController::GetController(SourceRig)->GetSkeletalMesh(), SourceMesh)
		|| !Test.TestEqual(TEXT("Permanent target rig binding"), UIKRigController::GetController(TargetRig)->GetSkeletalMesh(), TargetMesh)
		|| !Test.TestEqual(TEXT("Permanent body plus weapon chains"), UIKRigController::GetController(TargetRig)->GetRetargetChains().Num(), 22)
		|| !Test.TestTrue(TEXT("Permanent IK solver remains enabled"), UIKRigController::GetController(TargetRig)->GetNumSolvers() > 0)) { return false; }
	FIKRetargetProcessor Processor;
	FRetargetInitParameters Init;
	Init.SourceSkeletalMesh = SourceMesh;
	Init.TargetSkeletalMesh = TargetMesh;
	Init.RetargeterAsset = Retargeter;
	Init.CustomProfile = Retargeter->GetCurrentProfile();
	Init.bSuppressWarnings = false;
	Processor.Initialize(Init);
	if (!ProcessorMessages(Test, Processor) || !Test.TestTrue(TEXT("Permanent processor initialized"), Processor.IsInitialized())) { return false; }
	const FRetargetSkeleton& Source = Processor.GetSkeleton(ERetargetSourceOrTarget::Source);
	const FRetargetSkeleton& Target = Processor.GetSkeleton(ERetargetSourceOrTarget::Target);
	const FIKRetargetRootMotionOp* Root = Processor.GetFirstRetargetOpOfType<FIKRetargetRootMotionOp>();
	const FIKRetargetPinBoneOp* Pins = Processor.GetFirstRetargetOpOfType<FIKRetargetPinBoneOp>();
	if (!Test.TestTrue(TEXT("Separate Root Motion active and initialized"), Root && Root->IsEnabled() && Root->IsInitialized()
		&& Root->Settings.SourceRoot.BoneName == TEXT("root") && Root->Settings.TargetRoot.BoneName == TEXT("root"))
		|| !Test.TestTrue(TEXT("Permanent helper pins active and initialized"), Pins && Pins->IsEnabled() && Pins->IsInitialized())) { return false; }
	TArray<FTransform> SourceReference, TargetBind;
	if (!ReferencePose(Test, SourceMesh, Source, SourceReference, TEXT("Private-mode source reference"))
		|| !ReferencePose(Test, TargetMesh, Target, TargetBind, TEXT("Private-mode bind reference"))) { return false; }
	auto Run = [&](TArray<FTransform> Input, double DeltaTime, const FString& Clip, double Fraction, double Time, TArray<FTransform>& Converted)
	{
		Processor.ApplySourceScaleToPose(Input);
		TArray<FTransform> Unit, Local;
		if (!BakeInputScaleIntoTranslations(Test, Source, Input, Unit, Local, Output)) { return false; }
		RecordBones(Output, Clip, Fraction, Time, TEXT("processor_input_unit_component"), Source.BoneNames, Unit);
		FRetargetRunParameters P;
		P.SourceGlobalPose = &Unit;
		P.Profile = Retargeter->GetCurrentProfile();
		P.OverrideSetsToApply = Retargeter->GetOverrideSetsToApply();
		P.DeltaTime = static_cast<float>(DeltaTime);
		P.LOD = 0;
		Converted = Processor.RunRetargeter(P);
		return ValidatePose(Test, Converted, Target.BoneNames, Clip) && ProcessorMessages(Test, Processor);
	};
	auto HelpersFollow = [&](const TArray<FTransform>& Pose)
	{
		for (const TPair<FName, FName>& Pair : TArray<TPair<FName, FName>>{
			{ TEXT("foot_l"), TEXT("ik_foot_l") }, { TEXT("foot_r"), TEXT("ik_foot_r") },
			{ TEXT("hand_l"), TEXT("ik_hand_l") }, { TEXT("hand_r"), TEXT("ik_hand_r") },
			{ TEXT("weapon_r"), TEXT("ik_hand_gun") } })
		{
			const int32 X = Target.FindBoneIndexByName(Pair.Key);
			const int32 Y = Target.FindBoneIndexByName(Pair.Value);
			if (!Test.TestTrue(TEXT("Private helper follows endpoint: ") + Pair.Value.ToString(), X != INDEX_NONE && Y != INDEX_NONE
				&& Pose[X].GetTranslation().Equals(Pose[Y].GetTranslation(), 1.e-2))) { return false; }
		}
		return true;
	};
	auto ObserveSavedHelpers = [&](const TArray<FTransform>& Pose, const FString& Clip, double Fraction, double Time, const TCHAR* Space)
	{
		for (const TPair<FName, FName>& Pair : TArray<TPair<FName, FName>>{
			{ TEXT("foot_l"), TEXT("ik_foot_l") }, { TEXT("foot_r"), TEXT("ik_foot_r") },
			{ TEXT("hand_l"), TEXT("ik_hand_l") }, { TEXT("hand_r"), TEXT("ik_hand_r") },
			{ TEXT("weapon_r"), TEXT("ik_hand_gun") } })
		{
			const int32 X = Target.FindBoneIndexByName(Pair.Key), Y = Target.FindBoneIndexByName(Pair.Value);
			if (!Test.TestTrue(TEXT("Observed saved helper bones are present"), Pose.IsValidIndex(X) && Pose.IsValidIndex(Y))) { return false; }
			const FVector Delta = Pose[Y].GetTranslation() - Pose[X].GetTranslation();
			Meta(Output, FString(TEXT("saved_helper_observation_")) + FPaths::GetBaseFilename(Clip) + TEXT("_") + Space + TEXT("_")
				+ FString::Printf(TEXT("%.2f"), Fraction) + TEXT("_") + Pair.Value.ToString(),
				FString::Printf(TEXT("time=%.12f endpoint=%s helper=%s delta=(%.12f,%.12f,%.12f) distance_cm=%.12f max_axis_cm=%.12f; observation only"),
					Time, *Pair.Key.ToString(), *Pair.Value.ToString(), Delta.X, Delta.Y, Delta.Z, Delta.Size(), Delta.GetAbsMax()));
		}
		return true;
	};
	Processor.OnPlaybackReset();
	TArray<FTransform> Standing;
	if (!Run(SourceReference, 0, TEXT("PRIVATE_REFERENCE"), 0, 0, Standing) || !HelpersFollow(Standing)) { return false; }
	for (const FName Bone : { FName(TEXT("foot_l")), FName(TEXT("foot_r")), FName(TEXT("head")), FName(TEXT("pelvis")) })
	{
		const int32 I = Target.FindBoneIndexByName(Bone);
		const double Delta = FMath::Abs(Standing[I].GetTranslation().Z - TargetBind[I].GetTranslation().Z);
		const double Limit = Bone == TEXT("head") ? 12.0 : Bone == TEXT("pelvis") ? 8.0 : 5.0;
		Meta(Output, TEXT("private_standing_height_delta_") + Bone.ToString(), FString::Printf(TEXT("%.9f"), Delta));
		if (!Test.TestTrue(TEXT("Private standing height bound: ") + Bone.ToString(), Delta <= Limit)) { return false; }
	}
	const bool bSavedAssets = FParse::Param(FCommandLine::Get(), TEXT("DCInspectorPrivateSavedAssets"));
	double MaxSavedPositionError = 0.0;
	double MaxSavedAngleErrorDegrees = 0.0;
	int32 SavedBoneComparisons = 0;
	int32 SavedKeySamples = 0, SavedBodyFailures = 0;
	bool bSavedChecksPassed = true;
	TArray<bool> SourceMapped;
	for (const FName Name : Source.BoneNames)
	{
		SourceMapped.Add(Name == TEXT("root") || Processor.IsBoneMapped(Name, ERetargetSourceOrTarget::Source));
	}
	for (const TCHAR* Path : ClipPaths)
	{
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, Path);
		if (!Test.TestNotNull(TEXT("Original allowlisted source"), Sequence)) { return false; }
		UAnimSequence* Saved = nullptr;
		if (bSavedAssets)
		{
			const FString Name = Sequence->GetName();
			const FString SavedPath = TEXT("/Game/DreamCatcher/AI/Inspector/Animation/") + Name + TEXT(".") + Name;
			Saved = LoadObject<UAnimSequence>(nullptr, *SavedPath);
			if (!Test.TestNotNull(TEXT("Saved private animation"), Saved)
				|| !Test.TestTrue(TEXT("Saved animation targets private skeleton"), Saved->GetSkeleton() == TargetMesh->GetSkeleton())
				|| !Test.TestTrue(TEXT("Saved animation duration preserved"), FMath::IsNearlyEqual(Saved->GetPlayLength(), Sequence->GetPlayLength(), 1.e-5f))) { return false; }
		}
		Processor.OnPlaybackReset();
		TArray<FTransform> FirstSource, FirstSaved;
		double MaxSourceTranslation = 0.0, MaxSourceRotation = 0.0;
		double MaxSavedTranslation = 0.0, MaxSavedRotation = 0.0;
		int32 SavedSamples = 0;
		for (const double Fraction : { 0.0, 0.25, 0.5, 0.75, 1.0 })
		{
			const double Time = Sequence->GetPlayLength() * Fraction;
			TArray<FTransform> Input, Pose;
			if (!AnimationPose(Test, Sequence, SourceMesh, Time, Source, Input)
				|| !Run(Input, Fraction == 0 ? 0 : Sequence->GetPlayLength() * 0.25, Sequence->GetPathName(), Fraction, Time, Pose)) { return false; }
			RecordBones(Output, Sequence->GetPathName(), Fraction, Time, TEXT("private_target_component"), Target.BoneNames, Pose);
			if (!HelpersFollow(Pose)) { return false; } // Live processor endpoints remain strict at every sampled time.
			if (Saved)
			{
				TArray<FTransform> Baked;
				if (!AnimationPose(Test, Saved, TargetMesh, Time, Target, Baked)) { return false; }
				RecordBones(Output, Saved->GetPathName(), Fraction, Time, TEXT("saved_private_component"), Target.BoneNames, Baked);
				if (!ObserveSavedHelpers(Baked, Saved->GetPathName(), Fraction, Time, TEXT("interpolated"))) { return false; }
				// Saved tracks interpolate local transforms in different parent
				// hierarchies. Their helper/endpoint component positions need not be
				// identical between keys. Check the unchanged 0.01cm pin contract at
				// the engine's actual key, with no assumed sampling frame rate.
				int32 KeyFrame = INDEX_NONE, NumKeys = 0;
				float KeyTime = 0.0f;
				UAnimationBlueprintLibrary::GetFrameAtTime(Saved, static_cast<float>(Time), KeyFrame);
				UAnimationBlueprintLibrary::GetNumKeys(Saved, NumKeys);
				if (!Test.TestTrue(TEXT("Public saved key frame is in the actual key range"), KeyFrame >= 0 && KeyFrame < NumKeys)) { return false; }
				UAnimationBlueprintLibrary::GetTimeAtFrame(Saved, KeyFrame, KeyTime);
				TArray<FTransform> KeyPose;
				if (!AnimationPose(Test, Saved, TargetMesh, KeyTime, Target, KeyPose, KeyFrame)) { return false; }
				RecordBones(Output, Saved->GetPathName(), Fraction, KeyTime, TEXT("saved_private_key_component"), Target.BoneNames, KeyPose);
				if (!ObserveSavedHelpers(KeyPose, Saved->GetPathName(), Fraction, KeyTime, TEXT("key"))) { return false; }
				Meta(Output, TEXT("saved_key_frame_") + Saved->GetName() + TEXT("_") + FString::Printf(TEXT("%.2f"), Fraction),
					FString::Printf(TEXT("requested_time=%.12f actual_key=%d key_time=%.12f num_keys=%d"), Time, KeyFrame, static_cast<double>(KeyTime), NumKeys));
				const bool bKeyHelpersFollow = HelpersFollow(KeyPose);
				bSavedChecksPassed = bKeyHelpersFollow && bSavedChecksPassed;
				++SavedKeySamples;
				if (FirstSource.IsEmpty()) { FirstSource = Input; FirstSaved = Baked; }
				const FChange SourceChange = Changes(Input, FirstSource, &SourceMapped);
				const FChange SavedChange = Changes(Baked, FirstSaved);
				MaxSourceTranslation = FMath::Max(MaxSourceTranslation, SourceChange.Translation);
				MaxSourceRotation = FMath::Max(MaxSourceRotation, SourceChange.RotationDegrees);
				MaxSavedTranslation = FMath::Max(MaxSavedTranslation, SavedChange.Translation);
				MaxSavedRotation = FMath::Max(MaxSavedRotation, SavedChange.RotationDegrees);
				const int32 RootIndex = Target.FindBoneIndexByName(TEXT("root"));
				const bool bRootScaleMatches = Test.TestTrue(TEXT("Saved root retains private structural bind scale; processor unit scale is a separate contract"),
					Baked[RootIndex].GetScale3D().Equals(TargetBind[RootIndex].GetScale3D(), 1.e-4));
				bSavedChecksPassed = bRootScaleMatches && bSavedChecksPassed;
				// Compare the 61 preserved deformation bones and the animated
				// weapon, not generated virtual-bone transforms. Interpolated batch
				// samples and warm-started IK may differ slightly from this probe.
				TArray<int32> CompareIndices;
				for (int32 I = 0; I < 61; ++I) { CompareIndices.Add(Target.FindBoneIndexByName(B.GetBoneName(I))); }
				CompareIndices.Add(Target.FindBoneIndexByName(TEXT("weapon_r")));
				for (const int32 I : CompareIndices)
				{
					if (!Test.TestTrue(TEXT("Saved comparison bone is present"), Pose.IsValidIndex(I) && Baked.IsValidIndex(I))) { return false; }
					const double Position = FVector::Distance(Pose[I].GetTranslation(), Baked[I].GetTranslation());
					const double Angle = FMath::RadiansToDegrees(Pose[I].GetRotation().AngularDistance(Baked[I].GetRotation()));
					MaxSavedPositionError = FMath::Max(MaxSavedPositionError, Position);
					MaxSavedAngleErrorDegrees = FMath::Max(MaxSavedAngleErrorDegrees, Angle);
					++SavedBoneComparisons;
					const bool bPositionMatches = Test.TestTrue(TEXT("Saved joint position agrees with permanent processor: ") + Target.BoneNames[I].ToString(), Position <= 5.0);
					const bool bAngleMatches = Test.TestTrue(TEXT("Saved joint orientation agrees with permanent processor: ") + Target.BoneNames[I].ToString(), Angle <= 5.0);
					if (!bPositionMatches || !bAngleMatches) { ++SavedBodyFailures; }
					bSavedChecksPassed = bPositionMatches && bAngleMatches && bSavedChecksPassed;
				}
				++SavedSamples;
			}
			++Output.EvaluatedSamples;
		}
		if (Saved)
		{
			const bool bMeaningfulSourceMotion = MaxSourceTranslation > 0.1 || MaxSourceRotation > 0.1;
			const bool bSavedMoves = MaxSavedTranslation > 0.001 || MaxSavedRotation > 0.001;
			Meta(Output, TEXT("saved_clip_") + Saved->GetName(), FString::Printf(TEXT("samples=%d comparisons=%d source_translation=%.9f source_rotation_deg=%.9f saved_translation=%.9f saved_rotation_deg=%.9f"),
				SavedSamples, SavedSamples * 62, MaxSourceTranslation, MaxSourceRotation, MaxSavedTranslation, MaxSavedRotation));
			const bool bFiveSavedSamples = Test.TestEqual(TEXT("Exactly five saved samples per clip"), SavedSamples, 5);
			const bool bSavedMotionPreserved = Test.TestTrue(TEXT("Saved animation does not freeze while mapped source motion varies"), !bMeaningfulSourceMotion || bSavedMoves);
			bSavedChecksPassed = bFiveSavedSamples && bSavedMotionPreserved && bSavedChecksPassed;
		}
	}
	Meta(Output, TEXT("private_original_61_bones"), TEXT("names/parents/bind transforms unchanged"));
	Meta(Output, TEXT("private_target_bones"), TEXT("69 raw + 3 virtual"));
	Meta(Output, TEXT("private_saved_animation_check"), bSavedAssets ? TEXT("10 saved clips x 5 poses; target skeleton/duration; saved helper endpoints at public engine keys; raw 61+weapon comparison") : TEXT("not requested; memory/permanent-rig poses only"));
	Meta(Output, TEXT("saved_helper_policy"), TEXT("Live processor strict 0.01cm at every sampled time; saved helpers strict 0.01cm at actual engine key; interpolated helper offsets observed without a new numeric allowance"));
	Meta(Output, TEXT("saved_keyframe_samples"), FString::FromInt(SavedKeySamples));
	Meta(Output, TEXT("saved_body_comparison_failures"), FString::FromInt(SavedBodyFailures));
	Meta(Output, TEXT("saved_max_joint_position_error_cm"), FString::Printf(TEXT("%.9f"), MaxSavedPositionError));
	Meta(Output, TEXT("saved_max_joint_orientation_error_deg"), FString::Printf(TEXT("%.9f"), MaxSavedAngleErrorDegrees));
	Meta(Output, TEXT("saved_joint_comparisons"), FString::FromInt(SavedBoneComparisons));
	Meta(Output, TEXT("saved_bind_scale_policy"), TEXT("Root structural scale must match private bind pose; no custom data rewrite. Failure stops before activation; processor unit scales are not compared to stored scales"));
	if (bSavedAssets)
	{
		const bool bFiftyKeys = Test.TestEqual(TEXT("Saved mode exactly fifty engine key poses"), SavedKeySamples, ExpectedSampleCount);
		const bool bAllSavedBones = Test.TestEqual(TEXT("Saved mode exactly 3100 original61 plus weapon comparisons"), SavedBoneComparisons, ExpectedSampleCount * 62);
		bSavedChecksPassed = bFiftyKeys && bAllSavedBones && bSavedChecksPassed;
	}
	const bool bFiftyPoses = Test.TestEqual(TEXT("Private mode exactly fifty source poses"), Output.EvaluatedSamples, ExpectedSampleCount);
	const bool bClean = CheckClean(Test, TEXT("After private probe"));
	return bSavedChecksPassed && bFiftyPoses && bClean;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCInspectorRetargetPoseTest,
	"DreamCatcher.Editor.InspectorRetargetPose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCInspectorRetargetPoseTest::RunTest(const FString& Parameters)
{
	using namespace DCInspectorRetargetPreflight;
	// Refuse ordinary user-editor or broad test execution before loading a source.
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCInspectorRetargetPose")))
	{
		AddError(TEXT("Requires a separate unattended Editor with -DCInspectorRetargetPose. No source assets loaded."));
		return false;
	}
	FString ProjectDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(ProjectDirectory);
	FString ApprovedDirectory(ExpectedProject);
	FPaths::NormalizeDirectoryName(ApprovedDirectory);
	FString Run;
	if (!TestEqual(TEXT("Exact approved project directory"), ProjectDirectory, ApprovedDirectory)
		|| !TestTrue(TEXT("Dedicated run identifier supplied"), FParse::Value(FCommandLine::Get(), TEXT("DCInspectorPoseRun="), Run))) { return false; }
	bool bSafeRun = !Run.IsEmpty() && Run.Len() <= 40;
	for (const TCHAR Character : Run)
	{
		bSafeRun &= (Character >= TEXT('a') && Character <= TEXT('z')) || (Character >= TEXT('A') && Character <= TEXT('Z'))
			|| (Character >= TEXT('0') && Character <= TEXT('9')) || Character == TEXT('_');
	}
	if (!TestTrue(TEXT("Run identifier is 1-40 ASCII letters/digits/underscores"), bSafeRun) || !CheckClean(*this, TEXT("Before target load"))) { return false; }
	FOutput Output;
	const bool bPrivate = FParse::Param(FCommandLine::Get(), TEXT("DCInspectorRetargetPrivate"));
	// Automation's report may use the sibling Automation directory. This probe
	// only creates its own fresh leaf and never accepts an arbitrary output path.
	Output.Directory = FPaths::Combine(ProjectDirectory, TEXT("Saved/Diagnostics/InspectorBot20261023/Pose"), Run, TEXT("Probe"));
	if (!TestFalse(TEXT("Fresh probe directory required"), IFileManager::Get().DirectoryExists(*Output.Directory))
		|| !TestTrue(TEXT("Create only the approved diagnostics leaf"), IFileManager::Get().MakeDirectory(*Output.Directory, true))) { return false; }
	Meta(Output, TEXT("scope"), TEXT("Unsaved FK pose evaluation; not bot activation, animation-asset retargeting, skinning, contact, FootPlant, PIE or packaging validation"));
	Meta(Output, TEXT("source_mesh"), SourceMeshPath);
	Meta(Output, TEXT("target_mesh"), bPrivate ? TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector.Inspector") : TargetMeshPath);
	Meta(Output, TEXT("evaluation"), TEXT("Raw; OptionalMesh=Manny; ShouldRetarget=true; AdditiveAsFullPose=true; ExtractRootMotion=false; IncorporateRootMotionIntoPose=true; LOD=0"));
	Meta(Output, TEXT("root_policy"), TEXT("Original root-lock settings are read only; in-place zero displacement is valid"));
	Meta(Output, TEXT("input_scale_policy"), TEXT("Working copy only: apply native source scale once; derive original locals; bake original parent component scale into local translations; rebuild unit globals and verify every original joint position/rotation; scale-driven volume deformation is outside scope"));
	Meta(Output, TEXT("transient_setup"), TEXT("Two rigs; 21 body FK chains each; pelvis retarget root; root motion bone; explicit per-op FK mappings; zero-length root chain omitted; existing Root Motion directly drives root; only Run IK Rig disabled; no goals/solvers"));
	if (bPrivate)
	{
		Meta(Output, TEXT("mode_override"), TEXT("private permanent Inspector rig; 69 raw + 3 virtual bones; 22 FK chains; IK goals/solver/pins; separate Root Motion; no new transient rigs"));
		Meta(Output, TEXT("private_target_mesh"), TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector.Inspector"));
		Meta(Output, TEXT("limits"), TEXT("Numerical private rig/saved asset checks only; skinning, hand contact, FootPlant/live bot naturalness and packaging remain user verification"));
	}
	Meta(Output, TEXT("mapped_source_metrics"), TEXT("Body FK mapped bones plus the source root explicitly consumed by Root Motion"));
	Meta(Output, TEXT("asset_saves"), TEXT("0"));
	Meta(Output, TEXT("animation_duplicates"), TEXT("0"));
	const bool bEvaluated = Evaluate(*this, Output);
	const bool bClean = CheckClean(*this, TEXT("After preflight"));
	Meta(Output, TEXT("evaluated_animation_poses"), FString::FromInt(Output.EvaluatedSamples));
	Meta(Output, TEXT("scale_bearing_input_transforms"), FString::FromInt(Output.ScaleBearingInputTransforms));
	Meta(Output, TEXT("max_bake_translation_error_cm"), FString::Printf(TEXT("%.12f"), Output.MaxBakeTranslationError));
	Meta(Output, TEXT("max_bake_rotation_error_radians"), FString::Printf(TEXT("%.12f"), Output.MaxBakeRotationErrorRadians));
	Meta(Output, TEXT("max_baked_local_translation_compensation_cm"), FString::Printf(TEXT("%.12f"), Output.MaxLocalTranslationCompensation));
	const bool bSamplesWritten = FFileHelper::SaveStringArrayToFile(Output.Samples, *FPaths::Combine(Output.Directory, TEXT("samples.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool bBonesWritten = FFileHelper::SaveStringArrayToFile(Output.Bones, *FPaths::Combine(Output.Directory, TEXT("bones.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TestTrue(TEXT("Fixed sample and bone CSV evidence written"), bSamplesWritten && bBonesWritten);
	Meta(Output, TEXT("complete"), bEvaluated && bClean && !HasAnyErrors() ? TEXT("true") : TEXT("false"));
	const bool bMetadataWritten = FFileHelper::SaveStringArrayToFile(Output.Metadata, *FPaths::Combine(Output.Directory, TEXT("status.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TestTrue(TEXT("Final status CSV written"), bMetadataWritten);
	AddInfo(TEXT("FK numerical preflight only. Full additive poses do not reproduce live LinkedLayers/montage blending. Skinning, proportions, contact, FootPlant and live bots remain unverified. No assets saved or compiled."));
	UE_LOG(LogTemp, Display, TEXT("INSPECTOR_POSE_RESULT complete=%s samples=%d output=%s assets_saved=0 PIE=false"),
		bEvaluated && bClean && !HasAnyErrors() ? TEXT("true") : TEXT("false"), Output.EvaluatedSamples, *Output.Directory);
	return bEvaluated && bClean && !HasAnyErrors();
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
