// One-off editor support for HARU's approved Lyra skeleton extension.
// SkeletonModifier commits raw bones, but its in-memory mesh can omit existing
// virtual bones until PostLoad. Reuse that same engine rebuild before validation.
#if WITH_EDITOR

#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ReferenceSkeleton.h"
#include "UObject/UObjectGlobals.h"

namespace DCHaruLyraRigRepair
{
void RefreshVirtualBones()
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCHaruLyraRigRepair")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH requires unattended -DCHaruLyraRigRepair; nothing changed."));
		return;
	}

	USkeletalMesh* Haru = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU.SKM_HARU"));
	USkeleton* Skeleton = Haru ? Haru->GetSkeleton() : nullptr;
	if (!Skeleton || Skeleton->GetPathName() != TEXT("/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton.하루리깅_Skeleton") ||
		Haru->GetRefSkeleton().GetRawBoneNum() != 208 || Skeleton->GetVirtualBones().Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH expected HARU's 208 raw bones and three existing virtual bones; nothing changed."));
		return;
	}

	for (const FVirtualBone& Bone : Skeleton->GetVirtualBones())
	{
		const bool bExpected =
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_R_chestSpace") && Bone.SourceBoneName == TEXT("spine_05") && Bone.TargetBoneName == TEXT("hand_r")) ||
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_L_chestSpace") && Bone.SourceBoneName == TEXT("spine_05") && Bone.TargetBoneName == TEXT("hand_l")) ||
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_L_weaponSpace") && Bone.SourceBoneName == TEXT("weapon_r") && Bone.TargetBoneName == TEXT("hand_l"));
		if (!bExpected)
		{
			UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH virtual-bone contract changed; nothing changed."));
			return;
		}
	}

	FReferenceSkeleton& Reference = Haru->GetRefSkeleton();
	const TArray<FMeshBoneInfo> RawInfoBefore = Reference.GetRawRefBoneInfo();
	const TArray<FTransform> RawPoseBefore = Reference.GetRawRefBonePose();
	// USkeletalMesh::PostLoad performs this exact call. It rebuilds derived virtual
	// bone data; authored raw bones, skin weights, sockets and morphs are unchanged.
	Reference.RebuildRefSkeleton(Skeleton, false);
	if (Reference.GetRawBoneNum() != RawInfoBefore.Num() || Reference.GetNum() != 211)
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH unexpected bone counts; discard this process without saving."));
		return;
	}
	for (int32 BoneIndex = 0; BoneIndex < RawInfoBefore.Num(); ++BoneIndex)
	{
		const FMeshBoneInfo& Before = RawInfoBefore[BoneIndex];
		const FMeshBoneInfo& After = Reference.GetRawRefBoneInfo()[BoneIndex];
		if (Before.Name != After.Name || Before.ParentIndex != After.ParentIndex ||
			!RawPoseBefore[BoneIndex].Equals(Reference.GetRawRefBonePose()[BoneIndex], 0.0f))
		{
			UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH authored raw bone changed; discard this process without saving."));
			return;
		}
	}
	for (const FVirtualBone& Bone : Skeleton->GetVirtualBones())
	{
		if (Reference.FindBoneIndex(Bone.VirtualBoneName) < Reference.GetRawBoneNum())
		{
			UE_LOG(LogTemp, Error, TEXT("HARU_LYRA_RIG_REFRESH missing virtual bone; discard this process without saving."));
			return;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("HARU_LYRA_RIG_REFRESH_OK raw=208 final=211 virtual=3 saves=0"));
}

FAutoConsoleCommand RefreshCommand(
	TEXT("DC.HaruRig.RefreshLyraVirtualBones"),
	TEXT("Refresh HARU's existing virtual bones after the approved Lyra extension; requires -DCHaruLyraRigRepair. Never saves assets."),
	FConsoleCommandDelegate::CreateStatic(&RefreshVirtualBones));
}

#endif // WITH_EDITOR
