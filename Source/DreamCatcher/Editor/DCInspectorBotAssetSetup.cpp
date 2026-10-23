// Approved, path-limited Inspector bot asset preparation. No package save,
// source consolidation, world editing, PIE, build or generic migration API.
#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/BlendProfile.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "BlueprintEditorSettings.h"
#include "BoneContainer.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControlRigBlueprintLegacy.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Character.h"
#include "FileHelpers.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Hash/Blake3.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_EditablePinBase.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDevice.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "RetargetEditor/IKRetargeterPoseGenerator.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/PinBoneOp.h"
#include "Retargeter/RetargetOps/RootMotionGeneratorOp.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "Rig/IKRigDefinition.h"
#include "RigEditor/IKRigController.h"
#include "Rigs/RigHierarchy.h"
#include "Rigs/RigHierarchyController.h"
#include "RigVMModel/RigVMGraph.h"
#include "RigVMModel/RigVMClient.h"
#include "RigVMModel/RigVMController.h"
#include "RigVMModel/RigVMLink.h"
#include "RigVMModel/RigVMNode.h"
#include "RigVMModel/RigVMPin.h"
#include "RigVMModel/Nodes/RigVMRerouteNode.h"
#include "RigVMModel/Nodes/RigVMTemplateNode.h"
#include "RigVMCore/RigVMVariableDescription.h"
#include "SkeletonModifier.h"
#include "SkinnedAssetCompiler.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace DCInspectorBotAssets
{
constexpr TCHAR Root[] = TEXT("/Game/DreamCatcher/AI/Inspector/");
constexpr TCHAR MeshPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector.Inspector");
constexpr TCHAR SkeletonPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector_Skeleton.Inspector_Skeleton");
constexpr TCHAR PhysicsPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/Inspector_PhysicsAsset.Inspector_PhysicsAsset");
constexpr TCHAR OriginalMesh[] = TEXT("/Game/DreamCatcher/Main_Charcher/Inspector/Inspector/Inspector.Inspector");
constexpr TCHAR MannyMesh[] = TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny.SKM_Manny");
constexpr TCHAR MannySkeleton[] = TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin");
constexpr TCHAR SourceRigPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Rig/IK_DC_Inspector_Source.IK_DC_Inspector_Source");
constexpr TCHAR TargetRigPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Rig/IK_DC_Inspector_Target.IK_DC_Inspector_Target");
constexpr TCHAR RetargeterPath[] = TEXT("/Game/DreamCatcher/AI/Inspector/Rig/RTG_DC_Manny_Inspector.RTG_DC_Manny_Inspector");
constexpr TCHAR SourceFootPlant[] = TEXT("/Game/Characters/Heroes/Mannequin/Rig/CR_Mannequin_FootPlant.CR_Mannequin_FootPlant");
constexpr TCHAR PrivateFootPlant[] = TEXT("/Game/DreamCatcher/AI/Inspector/Assets/CR_Mannequin_FootPlant.CR_Mannequin_FootPlant");
constexpr TCHAR ManifestBlake3[] = TEXT("c730c851b440ada6d9f674261306cc0a60b539cbe4c68989b405f899365f1b81");

struct FRow { FString Source; FString Target; FString Class; };
struct FContext
{
	TArray<FRow> Rows;
	TSet<FString> Packages;
	FString Run;
	FString Phase;
	TArray<FString> Evidence{ TEXT("key,value") };
	FString Error;
	bool Require(bool bCondition, const FString& Message)
	{
		if (!bCondition && Error.IsEmpty()) { Error = Message; }
		return bCondition;
	}
	void Add(const FString& Key, const FString& Value)
	{
		auto Quote = [](const FString& S) { return TEXT("\"") + S.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\""); };
		Evidence.Add(Quote(Key) + TEXT(",") + Quote(Value));
	}
	bool Allowed(const UObject* Object) const
	{
		return Object && Packages.Contains(Object->GetOutermost()->GetName());
	}
};

FString ObjectPath(const FString& Package)
{
	return Package + TEXT(".") + FPackageName::GetLongPackageAssetName(Package);
}

bool CleanSources(FContext& C)
{
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyPackages(Dirty);
	for (const UPackage* P : Dirty)
	{
		if (P && P != GetTransientPackage() && !P->HasAnyFlags(RF_Transient) && !C.Packages.Contains(P->GetName()))
		{
			return C.Require(false, TEXT("Original package became dirty; discard without saving: ") + P->GetName());
		}
	}
	return true;
}

bool Guard(FContext& C)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(Project);
	if (!C.Require(Project == TEXT("E:/Epic Games/UEProjects/DreamCatcher"), TEXT("Wrong project"))
		|| !C.Require(FApp::IsUnattended() && FParse::Param(FCommandLine::Get(), TEXT("DCInspectorBotAssets")), TEXT("Requires unattended -DCInspectorBotAssets"))
		|| !C.Require(FString(FCommandLine::Get()).Contains(TEXT("-SCCProvider=None")), TEXT("SourceControl must be disabled"))
		|| !C.Require(GEditor && !GEditor->PlayWorld, TEXT("Requires an idle Editor; PIE is forbidden"))
		|| !C.Require(FParse::Value(FCommandLine::Get(), TEXT("DCInspectorBotRun="), C.Run), TEXT("Missing explicit run"))) { return false; }
	if (!C.Require(C.Run.Len() > 0 && C.Run.Len() <= 40, TEXT("Invalid run length"))) { return false; }
	for (const TCHAR Ch : C.Run)
	{
		if (!C.Require((FChar::IsAlnum(Ch) && Ch < 128) || Ch == TEXT('_'), TEXT("Run must be ASCII letters/digits/underscore"))) { return false; }
	}
	const FString CSV = FPaths::ProjectSavedDir() / TEXT("Diagnostics/InspectorBot20261023/Preflight/Validated_D/existing-asset-copy-plan.csv");
	TArray<uint8> Bytes;
	if (!C.Require(FFileHelper::LoadFileToArray(Bytes, *CSV), TEXT("Missing approved manifest"))) { return false; }
	const FString ActualHash = LexToString(FBlake3::HashBuffer(Bytes.GetData(), static_cast<uint64>(Bytes.Num())));
	C.Add(TEXT("manifest_hash_algorithm"), TEXT("BLAKE3-256; companion Python SHA256 guard retained"));
	C.Add(TEXT("manifest_blake3_256"), ActualHash);
	if (!C.Require(ActualHash.Equals(ManifestBlake3, ESearchCase::IgnoreCase), TEXT("Approved manifest BLAKE3-256 hash changed"))) { return false; }
	FString Text;
	if (!C.Require(FFileHelper::LoadFileToString(Text, *CSV), TEXT("Cannot read approved manifest"))) { return false; }
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, true);
	if (!C.Require(Lines.Num() == 305 && Lines[0].StartsWith(TEXT("source,target,class,")), TEXT("Expected exactly 304 approved copy rows"))) { return false; }
	TSet<FString> Sources;
	for (int32 Index = 1; Index < Lines.Num(); ++Index)
	{
		TArray<FString> Cells;
		Lines[Index].ParseIntoArray(Cells, TEXT(","), false);
		if (!C.Require(Cells.Num() >= 3 && Cells[0].StartsWith(TEXT("/Game/")) && Cells[1].StartsWith(Root)
			&& !Cells[1].Contains(TEXT("..")) && !Sources.Contains(Cells[0]) && !C.Packages.Contains(Cells[1]), TEXT("Unexpected manifest row"))) { return false; }
		Sources.Add(Cells[0]);
		C.Packages.Add(Cells[1]);
		C.Rows.Add({ Cells[0], Cells[1], Cells[2] });
	}
	C.Packages.Add(FPackageName::ObjectPathToPackageName(FString(SourceRigPath)));
	C.Packages.Add(FPackageName::ObjectPathToPackageName(FString(TargetRigPath)));
	C.Packages.Add(FPackageName::ObjectPathToPackageName(FString(RetargeterPath)));
	C.Add(TEXT("manifest_rows"), TEXT("304"));
	C.Add(TEXT("allowed_new_packages"), TEXT("307"));
	const UBlueprintEditorSettings* Settings = GetDefault<UBlueprintEditorSettings>();
	if (!C.Require(Settings != nullptr, TEXT("Blueprint Editor settings unavailable"))) { return false; }
	const ESaveOnCompile SaveValue = Settings->SaveOnCompile.GetValue();
	C.Add(TEXT("save_on_compile_value"), FString::FromInt(static_cast<int32>(SaveValue)));
	C.Add(TEXT("save_on_compile_name"), UEnum::GetValueAsString(SaveValue));
	return C.Require(C.Packages.Num() == 307, TEXT("Unexpected destination set"))
		&& C.Require(SaveValue == ESaveOnCompile::SoC_Never, TEXT("SaveOnCompile must already be Never; setting is not changed")) && CleanSources(C);
}

template <typename T> T* Private(FContext& C, const TCHAR* Path)
{
	T* Result = LoadObject<T>(nullptr, Path);
	C.Require(C.Allowed(Result), FString(TEXT("Missing or nonprivate object: ")) + Path);
	return C.Error.IsEmpty() ? Result : nullptr;
}

TArray<FTransform> GlobalPose(const FReferenceSkeleton& Ref)
{
	TArray<FTransform> Result;
	for (int32 I = 0; I < Ref.GetNum(); ++I)
	{
		const int32 Parent = Ref.GetParentIndex(I);
		Result.Add(Parent == INDEX_NONE ? Ref.GetRefBonePose()[I] : Ref.GetRefBonePose()[I] * Result[Parent]);
	}
	return Result;
}

bool OriginalMeshEqual(FContext& C, const USkeletalMesh* Original, const USkeletalMesh* Copy)
{
	if (!C.Require(Original && Copy && Original != Copy, TEXT("Expected original and private mesh"))) { return false; }
	const FReferenceSkeleton& A = Original->GetRefSkeleton();
	const FReferenceSkeleton& B = Copy->GetRefSkeleton();
	if (!C.Require(A.GetRawBoneNum() == 61 && B.GetRawBoneNum() >= 61, TEXT("Original 61 bones are required"))) { return false; }
	for (int32 I = 0; I < 61; ++I)
	{
		if (!C.Require(A.GetRawRefBoneInfo()[I].Name == B.GetRawRefBoneInfo()[I].Name
			&& A.GetRawRefBoneInfo()[I].ParentIndex == B.GetRawRefBoneInfo()[I].ParentIndex
			&& A.GetRawRefBonePose()[I].Equals(B.GetRawRefBonePose()[I], 0.0f), TEXT("An original bone changed"))) { return false; }
	}
	if (!C.Require(Original->GetMaterials().Num() == Copy->GetMaterials().Num(), TEXT("Material count changed"))) { return false; }
	if (!C.Require(Original->GetMorphTargets().Num() == Copy->GetMorphTargets().Num(), TEXT("Morph target count changed"))) { return false; }
	for (int32 I = 0; I < Original->GetMorphTargets().Num(); ++I)
	{
		if (!C.Require(Original->GetMorphTargets()[I]->GetFName() == Copy->GetMorphTargets()[I]->GetFName(), TEXT("Morph target name changed"))) { return false; }
	}
	for (int32 I = 0; I < Original->GetMaterials().Num(); ++I)
	{
		const FSkeletalMaterial& X = Original->GetMaterials()[I];
		const FSkeletalMaterial& Y = Copy->GetMaterials()[I];
		if (!C.Require(X.MaterialInterface == Y.MaterialInterface && X.MaterialSlotName == Y.MaterialSlotName
			&& X.ImportedMaterialSlotName == Y.ImportedMaterialSlotName, TEXT("Original material changed"))) { return false; }
	}
	const FSkeletalMeshModel* AM = Original->GetImportedModel();
	const FSkeletalMeshModel* BM = Copy->GetImportedModel();
	if (!C.Require(AM && BM && AM != BM && AM->LODModels.Num() == BM->LODModels.Num(), TEXT("Imported LOD ownership/count changed"))) { return false; }
	for (int32 L = 0; L < AM->LODModels.Num(); ++L)
	{
		const FSkeletalMeshLODModel& X = AM->LODModels[L];
		const FSkeletalMeshLODModel& Y = BM->LODModels[L];
		if (!C.Require(&X != &Y, TEXT("Private LOD model aliases source memory"))) { return false; }
		if (!C.Require(X.IndexBuffer == Y.IndexBuffer && X.Sections.Num() == Y.Sections.Num(), TEXT("Original topology changed"))) { return false; }
		for (int32 S = 0; S < X.Sections.Num(); ++S)
		{
			const FSkelMeshSection& XS = X.Sections[S];
			const FSkelMeshSection& YS = Y.Sections[S];
			if (!C.Require(XS.BoneMap == YS.BoneMap && XS.MaterialIndex == YS.MaterialIndex
				&& XS.SoftVertices.Num() == YS.SoftVertices.Num(), TEXT("Original section/bone map changed"))) { return false; }
			for (int32 V = 0; V < XS.SoftVertices.Num(); ++V)
			{
				const FSoftSkinVertex& XV = XS.SoftVertices[V];
				const FSoftSkinVertex& YV = YS.SoftVertices[V];
				if (!C.Require(XV.Position == YV.Position && XV.TangentX == YV.TangentX && XV.TangentY == YV.TangentY
					&& XV.TangentZ == YV.TangentZ && XV.Color == YV.Color
					&& FMemory::Memcmp(XV.UVs, YV.UVs, sizeof(XV.UVs)) == 0
					&& FMemory::Memcmp(XV.InfluenceBones, YV.InfluenceBones, sizeof(XV.InfluenceBones)) == 0
					&& FMemory::Memcmp(XV.InfluenceWeights, YV.InfluenceWeights, sizeof(XV.InfluenceWeights)) == 0,
					TEXT("Original geometry, UV, tangent or skin weights changed"))) { return false; }
			}
		}
	}
	return true;
}

FName InspectorBone(FName Name)
{
	return Name == TEXT("spine_04") || Name == TEXT("spine_05") ? FName(TEXT("spine_03"))
		: Name == TEXT("neck_02") ? FName(TEXT("neck_01")) : Name;
}

bool PrepareSkeleton(FContext& C, USkeletalMesh* Mesh, USkeleton* Skeleton, const USkeletalMesh* Original, const USkeletalMesh* Manny)
{
	if (!C.Require(Mesh->GetSkeleton() == Skeleton && Skeleton != Original->GetSkeleton(), TEXT("Bind private skeleton before Prepare"))
		|| !OriginalMeshEqual(C, Original, Mesh)
		|| !C.Require(Mesh->GetRefSkeleton().GetRawBoneNum() == 61 && Skeleton->GetVirtualBones().IsEmpty(), TEXT("Prepare requires a fresh 61-bone private copy"))) { return false; }
	const TArray<FTransform> Initial = GlobalPose(Mesh->GetRefSkeleton());
	const TArray<FTransform> Source = GlobalPose(Manny->GetRefSkeleton());
	auto TargetTransform = [&](FName Bone) { return Initial[Mesh->GetRefSkeleton().FindBoneIndex(Bone)]; };
	TStrongObjectPtr<USkeletonModifier> Modifier(NewObject<USkeletonModifier>(GetTransientPackage()));
	if (!C.Require(Modifier->SetSkeletalMesh(Mesh), TEXT("Cannot initialize private SkeletonModifier"))) { return false; }
	const TArray<TPair<FName, FName>> Helpers{
		{ TEXT("ik_foot_root"), TEXT("root") }, { TEXT("ik_foot_l"), TEXT("ik_foot_root") },
		{ TEXT("ik_foot_r"), TEXT("ik_foot_root") }, { TEXT("ik_hand_root"), TEXT("root") },
		{ TEXT("ik_hand_gun"), TEXT("ik_hand_root") }, { TEXT("ik_hand_r"), TEXT("ik_hand_gun") },
		{ TEXT("ik_hand_l"), TEXT("ik_hand_gun") }, { TEXT("weapon_r"), TEXT("hand_r") }
	};
	TMap<FName, FTransform> Globals;
	Globals.Add(TEXT("root"), TargetTransform(TEXT("root")));
	Globals.Add(TEXT("hand_r"), TargetTransform(TEXT("hand_r")));
	for (const TPair<FName, FName>& Pair : Helpers)
	{
		FTransform Transform = TargetTransform(TEXT("root"));
		if (Pair.Key == TEXT("ik_foot_l")) { Transform = TargetTransform(TEXT("foot_l")); }
		else if (Pair.Key == TEXT("ik_foot_r")) { Transform = TargetTransform(TEXT("foot_r")); }
		else if (Pair.Key == TEXT("ik_hand_l")) { Transform = TargetTransform(TEXT("hand_l")); }
		else if (Pair.Key == TEXT("ik_hand_r") || Pair.Key == TEXT("ik_hand_gun")) { Transform = TargetTransform(TEXT("hand_r")); }
		else if (Pair.Key == TEXT("weapon_r"))
		{
			const int32 Weapon = Manny->GetRefSkeleton().FindBoneIndex(TEXT("weapon_r"));
			const int32 Hand = Manny->GetRefSkeleton().FindBoneIndex(TEXT("hand_r"));
			if (!C.Require(Weapon != INDEX_NONE && Hand != INDEX_NONE, TEXT("Source weapon reference missing"))) { return false; }
			Transform = Source[Weapon].GetRelativeTransform(Source[Hand]) * TargetTransform(TEXT("hand_r"));
		}
		const FTransform Local = Transform.GetRelativeTransform(Globals.FindChecked(Pair.Value));
		if (!C.Require(Modifier->AddBone(Pair.Key, Pair.Value, Local), TEXT("Cannot add helper bone: ") + Pair.Key.ToString())) { return false; }
		Globals.Add(Pair.Key, Transform);
	}
	if (!C.Require(Modifier->CommitSkeletonToSkeletalMesh(), TEXT("Cannot commit private helper bones"))) { return false; }
	USkinnedAsset* Waiting[] = { Mesh };
	FSkinnedAssetCompilingManager::Get().FinishCompilation(MakeArrayView(Waiting));
	if (!OriginalMeshEqual(C, Original, Mesh)
		|| !C.Require(Mesh->GetRefSkeleton().GetRawBoneNum() == 69, TEXT("Expected 69 raw bones"))) { return false; }
	for (const TPair<FName, FName>& Pair : TArray<TPair<FName, FName>>{
		{ TEXT("spine_03"), TEXT("hand_r") }, { TEXT("spine_03"), TEXT("hand_l") }, { TEXT("weapon_r"), TEXT("hand_l") } })
	{
		const FName Name = Pair.Key == TEXT("weapon_r") ? FName(TEXT("VB IK_Hand_L_weaponSpace"))
			: Pair.Value == TEXT("hand_r") ? FName(TEXT("VB IK_Hand_R_chestSpace")) : FName(TEXT("VB IK_Hand_L_chestSpace"));
		if (!C.Require(Skeleton->AddNewNamedVirtualBone(Pair.Key, Pair.Value, Name), TEXT("Cannot add private virtual bone"))) { return false; }
	}
	Mesh->GetRefSkeleton().RebuildRefSkeleton(Skeleton, false);
	if (!C.Require(Mesh->GetRefSkeleton().GetNum() == 72, TEXT("Expected 69 physical plus 3 virtual bones"))) { return false; }
	for (FSkeletalMeshLODModel& LOD : Mesh->GetImportedModel()->LODModels)
	{
		for (int32 I = 61; I < 69; ++I) { LOD.ActiveBoneIndices.AddUnique(static_cast<FBoneIndexType>(I)); }
		for (int32 I = 61; I < 72; ++I) { LOD.RequiredBones.AddUnique(static_cast<FBoneIndexType>(I)); }
		LOD.ActiveBoneIndices.Sort();
		LOD.RequiredBones.Sort();
	}
	for (const FName Name : { FName(TEXT("FastFeet")), FName(TEXT("LeftFingersMask")), FName(TEXT("UpperBodyMask")), FName(TEXT("UpperBodyLowerBodySplitMask")) })
	{
		const UBlendProfile* OriginalProfile = nullptr;
		for (const UBlendProfile* Candidate : Manny->GetSkeleton()->BlendProfiles)
		{
			if (Candidate && Candidate->GetFName() == Name) { OriginalProfile = Candidate; break; }
		}
		UBlendProfile* Profile = Skeleton->CreateNewBlendProfile(Name);
		if (!C.Require(OriginalProfile && Profile && Profile->GetOuter() == Skeleton && C.Allowed(Profile), TEXT("Required source/private blend profile ownership missing"))) { return false; }
		Profile->SetSkeleton(Skeleton);
		Profile->Mode = OriginalProfile->GetMode();
		TMap<FName, float> Scales;
		for (int32 I = 0; I < OriginalProfile->GetNumBlendEntries(); ++I)
		{
			const FBlendProfileBoneEntry& Entry = OriginalProfile->GetEntry(I);
			const FName Bone = InspectorBone(Entry.BoneReference.BoneName);
			if (Mesh->GetRefSkeleton().FindBoneIndex(Bone) != INDEX_NONE)
			{
				float& Scale = Scales.FindOrAdd(Bone, Entry.BlendScale);
				Scale = FMath::Max(Scale, Entry.BlendScale);
			}
		}
		for (const TPair<FName, float>& Value : Scales) { Profile->SetBoneBlendScale(Value.Key, Value.Value, false, true); }
		Profile->RefreshBoneEntriesFromName();
		Profile->CleanupBoneEntries();
	}
	for (const FName Name : { FName(TEXT("weapon_r_muzzle")), FName(TEXT("weapon_r_preview")) })
	{
		const USkeletalMeshSocket* SourceSocket = Manny->GetSkeleton()->FindSocket(Name == TEXT("weapon_r_preview") ? FName(TEXT("weapon_r_muzzle")) : Name);
		if (!C.Require(SourceSocket != nullptr, TEXT("Source weapon muzzle socket is missing"))) { return false; }
		USkeletalMeshSocket* Socket = NewObject<USkeletalMeshSocket>(Skeleton, Name);
		Socket->SocketName = Name;
		Socket->BoneName = TEXT("weapon_r");
		Socket->SetSocketLocalTransform(Name == TEXT("weapon_r_preview") ? FTransform(FRotator(0, -90, 0)) : SourceSocket->GetSocketLocalTransform());
		Socket->bForceAlwaysAnimated = true;
		Skeleton->Sockets.Add(Socket);
	}
	Skeleton->MarkPackageDirty();
	Mesh->MarkPackageDirty();
	Mesh->PostEditChange();
	FSkinnedAssetCompilingManager::Get().FinishCompilation(MakeArrayView(Waiting));
	C.Add(TEXT("original_61_bones_geometry_weights_materials"), TEXT("preserved"));
	C.Add(TEXT("private_raw_virtual_bones"), TEXT("69+3"));
	C.Add(TEXT("private_profiles"), TEXT("FastFeet;LeftFingersMask;UpperBodyMask;UpperBodyLowerBodySplitMask"));
	return OriginalMeshEqual(C, Original, Mesh);
}

bool PreparePhysics(FContext& C, USkeletalMesh* Mesh, UPhysicsAsset* PA, const USkeletalMesh* Original)
{
	if (!C.Require(PA != Original->GetPhysicsAsset() && PA->SkeletalBodySetups.Num() == 13 && PA->FindBodyIndex(TEXT("head")) == INDEX_NONE, TEXT("Expected fresh Inspector PA with 13 bodies and no head"))) { return false; }
	UPhysicalMaterial* Normal = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Characters/Heroes/PhysMat_Player.PhysMat_Player"));
	UPhysicalMaterial* Weak = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Characters/Heroes/PhysMat_Player_WeakSpot.PhysMat_Player_WeakSpot"));
	if (!C.Require(Normal && Weak, TEXT("Existing Lyra physical materials must be available"))) { return false; }
	for (USkeletalBodySetup* Body : PA->SkeletalBodySetups)
	{
		if (!C.Require(Body && Body->GetOuter() == PA && C.Allowed(Body), TEXT("Refusing an external body setup mutation"))) { return false; }
		Body->PhysMaterial = Normal;
		Body->DefaultInstance.SetPhysMaterialOverride(Normal);
	}
	USkeletalBodySetup* Head = NewObject<USkeletalBodySetup>(PA, TEXT("InspectorHead"));
	Head->BoneName = TEXT("head");
	Head->PhysMaterial = Weak;
	Head->DefaultInstance.SetPhysMaterialOverride(Weak);
	Head->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	// Bound the added query body from vertices predominantly weighted to head.
	const int32 HeadIndex = Mesh->GetRefSkeleton().FindBoneIndex(TEXT("head"));
	const TArray<FTransform> Global = GlobalPose(Mesh->GetRefSkeleton());
	FBox HeadBounds(ForceInit);
	const FSkeletalMeshLODModel& LOD = Mesh->GetImportedModel()->LODModels[0];
	for (const FSkelMeshSection& Section : LOD.Sections)
	{
		for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
		{
			for (int32 I = 0; I < UE_ARRAY_COUNT(Vertex.InfluenceBones); ++I)
			{
				const int32 LocalBone = Vertex.InfluenceBones[I];
				if (Section.BoneMap.IsValidIndex(LocalBone) && Section.BoneMap[LocalBone] == HeadIndex && Vertex.InfluenceWeights[I] >= 32768)
				{
					HeadBounds += Global[HeadIndex].InverseTransformPosition(FVector(Vertex.Position));
					break;
				}
			}
		}
	}
	if (!C.Require(HeadBounds.IsValid != 0, TEXT("Cannot fit head hit body from existing skin weights"))) { return false; }
	FKSphereElem Sphere;
	Sphere.Center = HeadBounds.GetCenter();
	Sphere.Radius = static_cast<float>(FMath::Clamp(HeadBounds.GetExtent().GetMax() * 0.75, 3.0, 12.0));
	Head->AggGeom.SphereElems.Add(Sphere);
	const int32 NeckBodyIndex = PA->FindBodyIndex(TEXT("neck_01"));
	if (!C.Require(PA->SkeletalBodySetups.IsValidIndex(NeckBodyIndex), TEXT("Existing neck body is required"))) { return false; }
	USkeletalBodySetup* NeckBody = PA->SkeletalBodySetups[NeckBodyIndex];
	if (!C.Require(NeckBody && NeckBody->GetOuter() == PA && C.Allowed(NeckBody), TEXT("Refusing an external neck shape mutation"))) { return false; }
	const int32 NeckIndex = Mesh->GetRefSkeleton().FindBoneIndex(TEXT("neck_01"));
	const FVector HeadCenter = Global[HeadIndex].TransformPosition(Sphere.Center);
	const FVector NeckJoint = Global[NeckIndex].GetTranslation();
	const FVector Away = (NeckJoint - HeadCenter).GetSafeNormal();
	if (!C.Require(!Away.IsNearlyZero() && NeckBody->AggGeom.SphylElems.Num() == 1
		&& NeckBody->AggGeom.SphereElems.IsEmpty() && NeckBody->AggGeom.BoxElems.IsEmpty()
		&& NeckBody->AggGeom.ConvexElems.IsEmpty(), TEXT("Unexpected neck shape; cannot safely separate normal/weak hit volumes"))) { return false; }
	FKSphylElem& NeckShape = NeckBody->AggGeom.SphylElems[0];
	const double NeckRadius = FMath::Min(static_cast<double>(NeckShape.Radius), 3.0);
	const double HalfLength = 2.0;
	// Preserve all other thirteen-body data; only the oversized existing neck
	// capsule is fitted below the new head sphere, so normal geometry does not
	// mask the head's weak material. Keep both body materials explicitly normal.
	NeckShape.Radius = static_cast<float>(NeckRadius);
	NeckShape.Length = static_cast<float>(HalfLength * 2.0);
	const FVector LocalAxis = Global[NeckIndex].InverseTransformVectorNoScale(Away).GetSafeNormal();
	NeckShape.Rotation = FRotationMatrix::MakeFromZ(LocalAxis).Rotator();
	const double HeadRadiusComponent = Sphere.GetFinalScaled(Global[HeadIndex].GetScale3D(), FTransform::Identity).Radius;
	const double NeckHalfLengthComponent = NeckShape.GetScaledHalfLength(Global[NeckIndex].GetScale3D());
	const FVector NeckCenter = HeadCenter + Away * (HeadRadiusComponent + NeckHalfLengthComponent + 0.5);
	NeckShape.Center = Global[NeckIndex].InverseTransformPosition(NeckCenter);
	const double SurfaceDistance = NeckShape.GetShortestDistanceToPoint(HeadCenter, Global[NeckIndex]);
	C.Add(TEXT("head_neck_component_gap_cm"), FString::Printf(TEXT("%.9f"), SurfaceDistance - HeadRadiusComponent));
	if (!C.Require(SurfaceDistance >= HeadRadiusComponent + 0.25, TEXT("Scaled normal-neck and weak-head query volumes overlap"))) { return false; }
	PA->SkeletalBodySetups.Add(Head);
	UPhysicsConstraintTemplate* Neck = NewObject<UPhysicsConstraintTemplate>(PA, TEXT("InspectorHeadNeck"));
	Neck->DefaultInstance.JointName = TEXT("InspectorHeadNeck");
	Neck->DefaultInstance.ConstraintBone1 = TEXT("head");
	Neck->DefaultInstance.ConstraintBone2 = TEXT("neck_01");
	Neck->DefaultInstance.SetLinearXLimit(LCM_Locked, 0);
	Neck->DefaultInstance.SetLinearYLimit(LCM_Locked, 0);
	Neck->DefaultInstance.SetLinearZLimit(LCM_Locked, 0);
	Neck->DefaultInstance.SetAngularSwing1Limit(ACM_Limited, 35);
	Neck->DefaultInstance.SetAngularSwing2Limit(ACM_Limited, 35);
	Neck->DefaultInstance.SetAngularTwistLimit(ACM_Limited, 35);
	Neck->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
	Neck->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, Global[HeadIndex].GetRelativeTransform(Global[NeckIndex]));
	Neck->DefaultInstance.SetDisableCollision(true);
	PA->ConstraintSetup.Add(Neck);
	PA->UpdateBodySetupIndexMap();
	PA->PreviewSkeletalMesh = Mesh;
	Mesh->SetPhysicsAsset(PA);
	PA->MarkPackageDirty();
	C.Add(TEXT("private_physics"), TEXT("13 existing bodies retained; one head body and one head-neck constraint added; ordinary/weak materials reused"));
	return C.Require(PA->SkeletalBodySetups.Num() == 14 && PA->FindBodyIndex(TEXT("head")) != INDEX_NONE, TEXT("Private head physics setup failed"));
}

struct FChain
{
	FName Name, SourceStart, SourceEnd, TargetStart, TargetEnd, Goal;
};

TArray<FChain> Chains()
{
	TArray<FChain> Result{
		{ TEXT("Spine"), TEXT("spine_01"), TEXT("spine_05"), TEXT("spine_01"), TEXT("spine_03"), NAME_None },
		{ TEXT("Neck"), TEXT("neck_01"), TEXT("neck_02"), TEXT("neck_01"), TEXT("neck_01"), NAME_None },
		{ TEXT("Head"), TEXT("head"), TEXT("head"), TEXT("head"), TEXT("head"), NAME_None }
	};
	for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
	{
		auto Bone = [Side](const TCHAR* Part) { return FName(*(FString(Part) + TEXT("_") + Side)); };
		Result.Add({ Bone(TEXT("Clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")), Bone(TEXT("clavicle")), NAME_None });
		Result.Add({ Bone(TEXT("Arm")), Bone(TEXT("upperarm")), Bone(TEXT("hand")), Bone(TEXT("upperarm")), Bone(TEXT("hand")), Bone(TEXT("HandGoal")) });
		Result.Add({ Bone(TEXT("Leg")), Bone(TEXT("thigh")), Bone(TEXT("foot")), Bone(TEXT("thigh")), Bone(TEXT("foot")), Bone(TEXT("FootGoal")) });
		Result.Add({ Bone(TEXT("Foot")), Bone(TEXT("foot")), Bone(TEXT("ball")), Bone(TEXT("foot")), Bone(TEXT("ball")), NAME_None });
		for (const TCHAR* Digit : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
		{
			FString Name(Digit);
			Name[0] = FChar::ToUpper(Name[0]);
			Result.Add({ Bone(*Name), Bone(*(FString(Digit) + TEXT("_01"))), Bone(*(FString(Digit) + TEXT("_03"))),
				Bone(*(FString(Digit) + TEXT("_01"))), Bone(*(FString(Digit) + TEXT("_03"))), NAME_None });
		}
	}
	Result.Add({ TEXT("Weapon"), TEXT("weapon_r"), TEXT("weapon_r"), TEXT("weapon_r"), TEXT("weapon_r"), NAME_None });
	return Result;
}

bool PrepareRigs(FContext& C, USkeletalMesh* Manny, USkeletalMesh* Inspector)
{
	UIKRigDefinition* Source = Private<UIKRigDefinition>(C, SourceRigPath);
	UIKRigDefinition* Target = Private<UIKRigDefinition>(C, TargetRigPath);
	UIKRetargeter* Retargeter = Private<UIKRetargeter>(C, RetargeterPath);
	if (!Source || !Target || !Retargeter) { return false; }
	const TArray<FChain> Definitions = Chains();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		UIKRigDefinition* Rig = Side == 0 ? Source : Target;
		USkeletalMesh* Mesh = Side == 0 ? Manny : Inspector;
		const UIKRigController* Control = UIKRigController::GetController(Rig);
		if (!C.Require(Control && Control->GetRetargetChains().IsEmpty() && Control->GetNumSolvers() == 0, TEXT("Prepare requires new empty fixed-path rigs"))
			|| !C.Require(Control->SetSkeletalMesh(Mesh) && Control->SetRetargetRoot(TEXT("pelvis")) && Control->SetRootMotionBone(TEXT("root")), TEXT("Cannot set rig skeleton/root"))) { return false; }
		int32 Solver = INDEX_NONE;
		if (Side == 1)
		{
			Solver = Control->AddSolver(TEXT("/Script/IKRig.IKRigFullBodyIKSolver"));
			if (!C.Require(Solver != INDEX_NONE && Control->SetStartBone(TEXT("pelvis"), Solver), TEXT("Cannot configure private Full Body IK solver"))) { return false; }
		}
		for (const FChain& Chain : Definitions)
		{
			const FName Start = Side == 0 ? Chain.SourceStart : Chain.TargetStart;
			const FName End = Side == 0 ? Chain.SourceEnd : Chain.TargetEnd;
			if (!Chain.Goal.IsNone())
			{
				if (!C.Require(Control->AddNewGoal(Chain.Goal, End) == Chain.Goal, TEXT("Cannot add mapped limb goal"))
					|| (Side == 1 && !C.Require(Control->ConnectGoalToSolver(Chain.Goal, Solver), TEXT("Cannot connect mapped limb goal")))) { return false; }
			}
			if (!C.Require(Control->AddRetargetChain(Chain.Name, Start, End, Chain.Goal) == Chain.Name, TEXT("Cannot add required rig chain"))) { return false; }
		}
		Rig->MarkPackageDirty();
	}
	const UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
	if (!C.Require(Controller && Controller->GetNumRetargetOps() == 0, TEXT("Prepare requires a new empty fixed-path retargeter"))) { return false; }
	Controller->SetIKRig(ERetargetSourceOrTarget::Source, Source);
	Controller->SetIKRig(ERetargetSourceOrTarget::Target, Target);
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Source, Manny);
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Target, Inspector);
	Controller->AddDefaultOps();
	Controller->AssignIKRigToAllOps(ERetargetSourceOrTarget::Source, Source);
	Controller->AssignIKRigToAllOps(ERetargetSourceOrTarget::Target, Target);
	const int32 DefaultOpCount = Controller->GetNumRetargetOps();
	if (!C.Require(DefaultOpCount == 5, TEXT("Expected five retained default retarget operations"))) { return false; }
	int32 FKIndex = INDEX_NONE;
	for (int32 I = 0; I < Retargeter->GetRetargetOps().Num(); ++I)
	{
		if (Retargeter->GetRetargetOps()[I].GetScriptStruct() == FIKRetargetFKChainsOp::StaticStruct()) { FKIndex = I; }
		if (!C.Require(Controller->GetRetargetOpEnabled(I), TEXT("Permanent default retarget operations must remain enabled"))) { return false; }
	}
	if (!C.Require(FKIndex != INDEX_NONE, TEXT("Missing FK operation"))) { return false; }
	for (const FChain& Chain : Definitions)
	{
		if (!C.Require(Controller->SetSourceChain(Chain.Name, Chain.Name), TEXT("Cannot map private chain"))) { return false; }
	}
	// Establish a target retarget pose against the source proportions/reference
	// axes before baking. Numerical standing-pose gates run in the private probe.
	Controller->AutoAlignAllBones(ERetargetSourceOrTarget::Target, ERetargetAutoAlignMethod::ChainToChain);
	UIKRetargetFKChainsController* FKController = Cast<UIKRetargetFKChainsController>(const_cast<UIKRetargeterController*>(Controller)->GetOpController(FKIndex));
	if (!C.Require(FKController != nullptr, TEXT("Missing public FK settings controller"))) { return false; }
	FIKRetargetFKChainsOpSettings FKSettings = FKController->GetSettings();
	for (FRetargetFKChainSettings& Settings : FKSettings.ChainsToRetarget)
	{
		if (Settings.TargetChainName == TEXT("Weapon")) { Settings.RotationMode = EFKChainRotationMode::OneToOne; }
	}
	FKController->SetSettings(FKSettings);
	// The imported Inspector bind has inherited scale. Preserve its raw bones
	// and restore that scale in the actual exported-pose operation stack.
	const TArray<FTransform> Bind = GlobalPose(Inspector->GetRefSkeleton());
	for (const FName Bone : { FName(TEXT("pelvis")), FName(TEXT("root")) })
	{
		const int32 BoneIndex = Inspector->GetRefSkeleton().FindBoneIndex(Bone);
		if (!C.Require(Bind.IsValidIndex(BoneIndex), TEXT("Missing private bind-scale anchor"))) { return false; }
		const FVector BindScale = Bind[BoneIndex].GetScale3D();
		if (!C.Require(!BindScale.ContainsNaN() && BindScale.GetMin() > SMALL_NUMBER, TEXT("Invalid private bind scale"))) { return false; }
		const int32 ScaleIndex = Controller->AddRetargetOp(FIKRetargetPinBoneOp::StaticStruct());
		UIKRetargetPinBoneController* ScaleController = Cast<UIKRetargetPinBoneController>(const_cast<UIKRetargeterController*>(Controller)->GetOpController(ScaleIndex));
		if (!C.Require(ScaleController != nullptr, TEXT("Cannot configure private bind-scale self-pin"))) { return false; }
		FIKRetargetPinBoneOpSettings Scale = ScaleController->GetSettings();
		Scale.SkeletonToCopyFrom = ERetargetSourceOrTarget::Target;
		Scale.bCopyTranslation = false;
		Scale.bCopyRotation = false;
		Scale.bCopyScale = true;
		Scale.bPropagateToChildren = Bone == TEXT("pelvis");
		Scale.GlobalOffset = FTransform::Identity;
		Scale.LocalOffset = FTransform(FQuat::Identity, FVector::ZeroVector, BindScale);
		ScaleController->SetSettings(Scale);
		// Bone pairs are NotOverrideable setup data; SetSettings copies runtime
		// flags/offsets, while the dedicated controller API persists each pair.
		ScaleController->ClearAllBonePairs();
		ScaleController->SetBonePair(Bone, Bone);
		C.Add(TEXT("bind_scale_self_pin_") + Bone.ToString(), FString::Printf(TEXT("%.9f;%.9f;%.9f;propagate=%s"),
			BindScale.X, BindScale.Y, BindScale.Z, Scale.bPropagateToChildren ? TEXT("true") : TEXT("false")));
	}
	const int32 PinIndex = Controller->AddRetargetOp(FIKRetargetPinBoneOp::StaticStruct());
	UIKRetargetPinBoneController* PinController = Cast<UIKRetargetPinBoneController>(const_cast<UIKRetargeterController*>(Controller)->GetOpController(PinIndex));
	if (!C.Require(PinController != nullptr, TEXT("Cannot configure target helper-bone pins"))) { return false; }
	FIKRetargetPinBoneOpSettings Pins = PinController->GetSettings();
	Pins.SkeletonToCopyFrom = ERetargetSourceOrTarget::Target;
	Pins.bCopyScale = true;
	Pins.bPropagateToChildren = false;
	PinController->SetSettings(Pins);
	PinController->ClearAllBonePairs();
	const TArray<TPair<FName, FName>> HelperPairs{
		{ TEXT("root"), TEXT("ik_foot_root") }, { TEXT("root"), TEXT("ik_hand_root") },
		{ TEXT("foot_l"), TEXT("ik_foot_l") }, { TEXT("foot_r"), TEXT("ik_foot_r") },
		{ TEXT("hand_l"), TEXT("ik_hand_l") }, { TEXT("hand_r"), TEXT("ik_hand_r") },
		{ TEXT("weapon_r"), TEXT("ik_hand_gun") } };
	for (const TPair<FName, FName>& Pair : HelperPairs)
	{
		PinController->SetBonePair(Pair.Key, Pair.Value);
	}
	const int32 WeaponIndex = Controller->AddRetargetOp(FIKRetargetPinBoneOp::StaticStruct());
	UIKRetargetPinBoneController* WeaponController = Cast<UIKRetargetPinBoneController>(const_cast<UIKRetargeterController*>(Controller)->GetOpController(WeaponIndex));
	if (!C.Require(WeaponController != nullptr, TEXT("Cannot configure animated weapon relative translation"))) { return false; }
	FIKRetargetPinBoneOpSettings Weapon = WeaponController->GetSettings();
	Weapon.SkeletonToCopyFrom = ERetargetSourceOrTarget::Source;
	Weapon.TranslationMode = EPinBoneTranslationMode::CopyLocalPositionRelativeOffset;
	Weapon.bCopyRotation = false; // Rotation is retargeted by the dedicated FK chain.
	Weapon.bCopyScale = false;
	WeaponController->SetSettings(Weapon);
	WeaponController->ClearAllBonePairs();
	WeaponController->SetBonePair(TEXT("weapon_r"), TEXT("weapon_r"));
	// Weapon translation must run before the final helper pins.
	if (!C.Require(Controller->MoveRetargetOpInStack(WeaponIndex, PinIndex), TEXT("Cannot order weapon before helper pins"))) { return false; }
	if (!C.Require(Controller->GetNumRetargetOps() == DefaultOpCount + 4, TEXT("Unexpected bind-scale operation count"))) { return false; }
	auto VerifyStoredPairs = [&](int32 OpIndex, const TArray<TPair<FName, FName>>& Expected, const FString& Label)
	{
		UIKRetargetPinBoneController* PinControl = Cast<UIKRetargetPinBoneController>(const_cast<UIKRetargeterController*>(Controller)->GetOpController(OpIndex));
		const FIKRetargetPinBoneOp* Stored = Retargeter->GetRetargetOps()[OpIndex].GetPtr<FIKRetargetPinBoneOp>();
		if (!C.Require(PinControl && Stored, TEXT("Missing stored Pin operation: ") + Label)) { return false; }
		const TMap<FName, FName> GetterPairs = PinControl->GetAllBonePairs();
		const FIKRetargetPinBoneOpSettings& Actual = Stored->Settings;
		C.Add(TEXT("pin_pair_counts_") + Label, FString::Printf(TEXT("getter=%d asset=%d expected=%d"), GetterPairs.Num(), Actual.BonesToPin.Num(), Expected.Num()));
		const FVector LocalScale = Actual.LocalOffset.GetScale3D();
		C.Add(TEXT("pin_settings_") + Label, FString::Printf(TEXT("skeleton=%s translation=%d rotation=%d scale=%d propagate=%d local_scale=%.9f;%.9f;%.9f"),
			Actual.SkeletonToCopyFrom == ERetargetSourceOrTarget::Source ? TEXT("Source") : TEXT("Target"),
			Actual.bCopyTranslation, Actual.bCopyRotation, Actual.bCopyScale, Actual.bPropagateToChildren, LocalScale.X, LocalScale.Y, LocalScale.Z));
		if (!C.Require(GetterPairs.Num() == Expected.Num() && Actual.BonesToPin.Num() == Expected.Num(), TEXT("Empty or mismatched stored Pin pair count: ") + Label)) { return false; }
		for (const TPair<FName, FName>& Pair : Expected)
		{
			const FName* FromGetter = GetterPairs.Find(Pair.Value); // map is target -> source
			const bool bStored = Actual.BonesToPin.ContainsByPredicate([&Pair](const FPinBoneData& Entry)
			{
				return Entry.BoneToCopyFrom.BoneName == Pair.Key && Entry.BoneToCopyTo.BoneName == Pair.Value;
			});
			C.Add(TEXT("pin_pair_") + Label + TEXT("_") + Pair.Value.ToString(), FromGetter ? FromGetter->ToString() : TEXT("missing"));
			if (!C.Require(FromGetter && *FromGetter == Pair.Key && bStored, TEXT("Getter/asset Pin pair mismatch: ") + Label + TEXT("/") + Pair.Value.ToString())) { return false; }
		}
		return true;
	};
	if (!VerifyStoredPairs(DefaultOpCount, TArray<TPair<FName, FName>>{ { TEXT("pelvis"), TEXT("pelvis") } }, TEXT("pelvis"))
		|| !VerifyStoredPairs(DefaultOpCount + 1, TArray<TPair<FName, FName>>{ { TEXT("root"), TEXT("root") } }, TEXT("root"))
		|| !VerifyStoredPairs(DefaultOpCount + 2, TArray<TPair<FName, FName>>{ { TEXT("weapon_r"), TEXT("weapon_r") } }, TEXT("weapon"))
		|| !VerifyStoredPairs(DefaultOpCount + 3, HelperPairs, TEXT("helpers"))) { return false; }
	const FIKRetargetPinBoneOp* OrderedPelvis = Retargeter->GetRetargetOps()[DefaultOpCount].GetPtr<FIKRetargetPinBoneOp>();
	const FIKRetargetPinBoneOp* OrderedRoot = Retargeter->GetRetargetOps()[DefaultOpCount + 1].GetPtr<FIKRetargetPinBoneOp>();
	const FIKRetargetPinBoneOp* OrderedWeapon = Retargeter->GetRetargetOps()[DefaultOpCount + 2].GetPtr<FIKRetargetPinBoneOp>();
	const FIKRetargetPinBoneOp* OrderedHelpers = Retargeter->GetRetargetOps()[DefaultOpCount + 3].GetPtr<FIKRetargetPinBoneOp>();
	if (!C.Require(OrderedPelvis && OrderedRoot && OrderedWeapon && OrderedHelpers
		&& OrderedPelvis->Settings.BonesToPin.Num() == 1 && OrderedPelvis->Settings.BonesToPin[0].BoneToCopyTo.BoneName == TEXT("pelvis")
		&& OrderedPelvis->Settings.SkeletonToCopyFrom == ERetargetSourceOrTarget::Target
		&& !OrderedPelvis->Settings.bCopyTranslation && !OrderedPelvis->Settings.bCopyRotation && OrderedPelvis->Settings.bCopyScale && OrderedPelvis->Settings.bPropagateToChildren
		&& OrderedRoot->Settings.BonesToPin.Num() == 1 && OrderedRoot->Settings.BonesToPin[0].BoneToCopyTo.BoneName == TEXT("root")
		&& OrderedRoot->Settings.SkeletonToCopyFrom == ERetargetSourceOrTarget::Target
		&& !OrderedRoot->Settings.bCopyTranslation && !OrderedRoot->Settings.bCopyRotation && OrderedRoot->Settings.bCopyScale && !OrderedRoot->Settings.bPropagateToChildren
		&& OrderedWeapon->Settings.SkeletonToCopyFrom == ERetargetSourceOrTarget::Source
		&& OrderedHelpers->Settings.BonesToPin.Num() == 7 && OrderedHelpers->Settings.bCopyScale,
		TEXT("Required order is defaults, pelvis scale, root scale, weapon translation, helper pins"))) { return false; }
	for (int32 I = 0; I < Controller->GetNumRetargetOps(); ++I)
	{
		if (!C.Require(Controller->GetRetargetOpEnabled(I), TEXT("Every permanent retarget operation must remain enabled"))) { return false; }
	}
	Retargeter->MarkPackageDirty();
	C.Add(TEXT("permanent_rig_configuration"), TEXT("22 FK chains (21 body + animated weapon); five defaults retained; active Full Body IK; private bind-scale pelvis/root self-pins; weapon translation then seven scale-copy helper pins; target pose auto-alignment"));
	C.Add(TEXT("standing_pose_calibration_status"), TEXT("configured; requires private pose test before any save or activation"));
	return true;
}

using FReplacements = TMap<UObject*, UObject*>;

bool Replacements(FContext& C, FReplacements& Map)
{
	Map.Reset();
	for (const FRow& Row : C.Rows)
	{
		UObject* Source = LoadObject<UObject>(nullptr, *ObjectPath(Row.Source));
		UObject* Target = LoadObject<UObject>(nullptr, *ObjectPath(Row.Target));
		if (!C.Require(Source && C.Allowed(Target) && Source != Target, TEXT("Missing exact source/private pair: ") + Row.Target)) { return false; }
		Map.Add(Source, Target);
		if (const UBlueprint* SourceBP = Cast<UBlueprint>(Source))
		{
			UBlueprint* TargetBP = Cast<UBlueprint>(Target);
			if (!C.Require(TargetBP && SourceBP->GeneratedClass && TargetBP->GeneratedClass, TEXT("Blueprint pair has no generated class"))) { return false; }
			Map.Add(SourceBP->GeneratedClass, TargetBP->GeneratedClass);
			if (SourceBP->SkeletonGeneratedClass && TargetBP->SkeletonGeneratedClass) { Map.Add(SourceBP->SkeletonGeneratedClass, TargetBP->SkeletonGeneratedClass); }
			Map.Add(SourceBP->GeneratedClass->GetDefaultObject(), TargetBP->GeneratedClass->GetDefaultObject());
		}
	}
	USkeleton* Skeleton = Private<USkeleton>(C, SkeletonPath);
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	USkeleton* SourceSkeleton = LoadObject<USkeleton>(nullptr, MannySkeleton);
	if (!Skeleton || !Mesh || !C.Require(SourceSkeleton != nullptr, TEXT("Missing source skeleton"))) { return false; }
	Map.Add(SourceSkeleton, Skeleton);
	for (const FName Name : { FName(TEXT("FastFeet")), FName(TEXT("LeftFingersMask")), FName(TEXT("UpperBodyMask")), FName(TEXT("UpperBodyLowerBodySplitMask")) })
	{
		UBlendProfile* A = SourceSkeleton->GetBlendProfile(Name);
		UBlendProfile* B = Skeleton->GetBlendProfile(Name);
		if (!C.Require(A && B, TEXT("Missing Inspector profile counterpart"))) { return false; }
		Map.Add(A, B);
	}
	return true;
}

FString ReplacePaths(FString Text, const FReplacements& Map)
{
	TArray<UObject*> Keys;
	Map.GetKeys(Keys);
	Keys.Sort([](const UObject& A, const UObject& B) { return A.GetPathName().Len() > B.GetPathName().Len(); });
	for (const UObject* Key : Keys)
	{
		Text.ReplaceInline(*Key->GetPathName(), *Map.FindChecked(const_cast<UObject*>(Key))->GetPathName(), ESearchCase::CaseSensitive);
	}
	return Text;
}

bool RewriteValue(FContext& C, FProperty* Property, void* Data, const FReplacements& Map, bool bBones, bool bVerify, int32 Depth)
{
	if (!C.Require(Depth <= 32, TEXT("Unexpected deeply nested private property"))) { return false; }
	if (FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPtr Current = Soft->GetPropertyValue(Data);
		const FString Replaced = ReplacePaths(Current.ToSoftObjectPath().ToString(), Map);
		if (Replaced != Current.ToSoftObjectPath().ToString())
		{
			if (!C.Require(!bVerify, TEXT("Original soft reference remains: ") + Current.ToSoftObjectPath().ToString())) { return false; }
			Soft->SetPropertyValue(Data, FSoftObjectPtr(FSoftObjectPath(Replaced)));
		}
	}
	else if (FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property))
	{
		if (UObject* const* Replacement = Map.Find(Object->GetObjectPropertyValue(Data)))
		{
			if (!C.Require(!bVerify, TEXT("Original hard reference remains in ") + Property->GetName())) { return false; }
			Object->SetObjectPropertyValue(Data, *Replacement);
		}
	}
	else if (FStructProperty* Struct = CastField<FStructProperty>(Property))
	{
		for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
		{
			for (int32 I = 0; I < It->ArrayDim; ++I)
			{
				if (!RewriteValue(C, *It, It->ContainerPtrToValuePtr<void>(Data, I), Map, bBones, bVerify, Depth + 1)) { return false; }
			}
		}
	}
	else if (FArrayProperty* Array = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(Array, Data);
		// Orientation Warping distributes the same 0.75 alpha over the valid
		// three Inspector spine bones plus the existing hand IK root. Do not
		// retain duplicate spine_03 entries or disable the orientation node.
		if (bBones && Property->GetFName() == TEXT("SpineBones"))
		{
			FStructProperty* Item = CastField<FStructProperty>(Array->Inner);
			FNameProperty* Bone = Item ? FindFProperty<FNameProperty>(Item->Struct, TEXT("BoneName")) : nullptr;
			if (!C.Require(Bone != nullptr, TEXT("Unexpected SpineBones layout"))) { return false; }
			for (int32 I = Helper.Num() - 1; I >= 0; --I)
			{
				const FName Name = Bone->GetPropertyValue_InContainer(Helper.GetRawPtr(I));
				if (Name == TEXT("spine_04") || Name == TEXT("spine_05"))
				{
					if (!C.Require(!bVerify, TEXT("Unadapted five-bone spine distribution remains"))) { return false; }
					Helper.RemoveValues(I);
				}
			}
		}
		for (int32 I = 0; I < Helper.Num(); ++I)
		{
			if (!RewriteValue(C, Array->Inner, Helper.GetRawPtr(I), Map, bBones, bVerify, Depth + 1)) { return false; }
		}
	}
	else if (FMapProperty* PropertyMap = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper Helper(PropertyMap, Data);
		for (int32 I = 0; I < Helper.GetMaxIndex(); ++I)
		{
			if (Helper.IsValidIndex(I) && (!RewriteValue(C, PropertyMap->KeyProp, Helper.GetKeyPtr(I), Map, bBones, bVerify, Depth + 1)
				|| !RewriteValue(C, PropertyMap->ValueProp, Helper.GetValuePtr(I), Map, bBones, bVerify, Depth + 1))) { return false; }
		}
		if (!bVerify) { Helper.Rehash(); }
	}
	else if (FSetProperty* Set = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper Helper(Set, Data);
		for (int32 I = 0; I < Helper.GetMaxIndex(); ++I)
		{
			if (Helper.IsValidIndex(I) && !RewriteValue(C, Set->ElementProp, Helper.GetElementPtr(I), Map, bBones, bVerify, Depth + 1)) { return false; }
		}
		if (!bVerify) { Helper.Rehash(); }
	}
	else if (FStrProperty* String = CastField<FStrProperty>(Property))
	{
		const FString Original = String->GetPropertyValue(Data);
		FString Replaced = ReplacePaths(Original, Map);
		if (bBones && (Replaced == TEXT("spine_04") || Replaced == TEXT("spine_05") || Replaced == TEXT("neck_02"))) { Replaced = InspectorBone(FName(*Replaced)).ToString(); }
		if (Replaced != Original)
		{
			if (!C.Require(!bVerify, TEXT("Unadapted private path/bone string remains"))) { return false; }
			String->SetPropertyValue(Data, Replaced);
		}
	}
	else if (FNameProperty* Name = CastField<FNameProperty>(Property))
	{
		if (bBones && Property->GetFName() == TEXT("BoneName"))
		{
			const FName Original = Name->GetPropertyValue(Data);
			const FName NewName = InspectorBone(Original);
			if (Original != NewName)
			{
				if (!C.Require(!bVerify, TEXT("Unadapted body bone name remains"))) { return false; }
				Name->SetPropertyValue(Data, NewName);
			}
		}
	}
	return true;
}

FString AdaptBoneLiteral(FString Text)
{
	if (Text == TEXT("spine_04") || Text == TEXT("spine_05") || Text == TEXT("neck_02")) { return InspectorBone(FName(*Text)).ToString(); }
	// Only whole BoneReference / RigElementKey values are recognized. No
	// arbitrary substring replacement in prose, pin labels or graph comments.
	for (const TCHAR* Field : { TEXT("BoneName"), TEXT("Name") })
	{
		for (const TCHAR* Old : { TEXT("spine_04"), TEXT("spine_05"), TEXT("neck_02") })
		{
			const FString A = FString(Field) + TEXT("=\"") + Old + TEXT("\"");
			const FString B = FString(Field) + TEXT("=\"") + InspectorBone(FName(Old)).ToString() + TEXT("\"");
			Text.ReplaceInline(*A, *B, ESearchCase::CaseSensitive);
			const FString Plain = FString(Field) + TEXT("=") + Old;
			int32 Search = 0;
			for (int32 At = Text.Find(Plain, ESearchCase::CaseSensitive, ESearchDir::FromStart, Search); At != INDEX_NONE;
				At = Text.Find(Plain, ESearchCase::CaseSensitive, ESearchDir::FromStart, Search))
			{
				const int32 End = At + Plain.Len();
				const bool Left = At == 0 || Text[At - 1] == TEXT('(') || Text[At - 1] == TEXT(',');
				const bool Right = End == Text.Len() || Text[End] == TEXT(')') || Text[End] == TEXT(',');
				if (Left && Right)
				{
					const FString Replacement = FString(Field) + TEXT("=") + InspectorBone(FName(Old)).ToString();
					Text = Text.Left(At) + Replacement + Text.Mid(End);
					Search = At + Replacement.Len();
				}
				else { Search = End; }
			}
		}
	}
	return Text;
}

bool RewritePinType(FContext& C, FEdGraphPinType& Type, const FReplacements& Map, bool bVerify)
{
	// Pin types are not limited to their weak subcategory object: delegate
	// member references and map value types can also name a copied class.
	for (TFieldIterator<FProperty> It(FEdGraphPinType::StaticStruct()); It; ++It)
	{
		for (int32 I = 0; I < It->ArrayDim; ++I)
		{
			if (!RewriteValue(C, *It, It->ContainerPtrToValuePtr<void>(&Type, I), Map, false, bVerify, 0)) { return false; }
		}
	}
	return true;
}

bool RewriteGraphNode(FContext& C, UEdGraphNode* Node, const FReplacements& Map, bool bBones, bool bVerify)
{
	if (!C.Require(C.Allowed(Node) && Node->GetGraph() && Node->GetGraph()->GetOutermost() == Node->GetOutermost(), TEXT("Original or externally owned graph node refused"))) { return false; }
	for (TFieldIterator<FProperty> It(Node->GetClass()); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient)) { continue; }
		for (int32 I = 0; I < It->ArrayDim; ++I)
		{
			if (!RewriteValue(C, *It, It->ContainerPtrToValuePtr<void>(Node, I), Map, bBones, bVerify, 0)) { return false; }
		}
	}
	if (UK2Node_EditablePinBase* Editable = Cast<UK2Node_EditablePinBase>(Node))
	{
		TArray<TSharedPtr<FUserPinInfo>> Independent;
		for (const TSharedPtr<FUserPinInfo>& Original : Editable->UserDefinedPins)
		{
			if (!C.Require(Original.IsValid(), TEXT("Missing private user-declared pin description"))) { return false; }
			// Native UserDefinedPins is serialized outside UPROPERTY. A copied
			// Blueprint must never edit a possibly shared source description.
			TSharedPtr<FUserPinInfo> Info = Original;
			if (!bVerify) { Info = MakeShared<FUserPinInfo>(*Original); }
			if (!RewritePinType(C, Info->PinType, Map, bVerify)) { return false; }
			const FString NewDefault = ReplacePaths(Info->PinDefaultValue, Map);
			if (NewDefault != Info->PinDefaultValue)
			{
				if (!C.Require(!bVerify, TEXT("Original user-declared pin default reference remains"))) { return false; }
				Info->PinDefaultValue = NewDefault;
			}
			if (!C.Require(Info->PinName == Original->PinName && Info->DesiredPinDirection == Original->DesiredPinDirection,
				TEXT("User-declared pin name/direction changed"))) { return false; }
			if (Node->GetGraph()->GetFName() == TEXT("GetMainAnimBPThreadSafe") && Info->PinName == TEXT("ReturnValue"))
			{
				const UObject* Type = Info->PinType.PinSubCategoryObject.Get();
				C.Add(TEXT("private_getmain_declared_return"), Node->GetPathName() + TEXT("=") + (Type ? Type->GetPathName() : FString(TEXT("none")))
					+ (bVerify ? TEXT(" verified") : TEXT(" rewritten")));
				if (bVerify)
				{
					const UAnimBlueprint* Main = Private<UAnimBlueprint>(C, TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.ABP_Mannequin_Base"));
					if (!C.Require(Main && Type == Main->GeneratedClass, TEXT("GetMainAnimBP declared return must name the current private Main class"))) { return false; }
				}
			}
			if (!bVerify) { Independent.Add(MoveTemp(Info)); }
		}
		if (!bVerify) { Editable->UserDefinedPins = MoveTemp(Independent); }
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin) { continue; }
		if (!C.Require(Pin->GetOwningNodeUnchecked() == Node, TEXT("Private graph pin aliases another node")) || !RewritePinType(C, Pin->PinType, Map, bVerify)) { return false; }
		if (bVerify && Node->IsA<UK2Node_EditablePinBase>() && Node->GetGraph()->GetFName() == TEXT("GetMainAnimBPThreadSafe") && Pin->PinName == TEXT("ReturnValue"))
		{
			const UAnimBlueprint* Main = Private<UAnimBlueprint>(C, TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.ABP_Mannequin_Base"));
			const UObject* Type = Pin->PinType.PinSubCategoryObject.Get();
			C.Add(TEXT("private_getmain_live_return"), Node->GetPathName() + TEXT("=") + (Type ? Type->GetPathName() : FString(TEXT("none"))));
			if (!C.Require(Main && Type == Main->GeneratedClass, TEXT("GetMainAnimBP live return must name the current private Main class"))) { return false; }
		}
		if (UObject* const* Replacement = Map.Find(Pin->DefaultObject.Get()))
		{
			if (!C.Require(!bVerify, TEXT("Original graph pin object remains"))) { return false; }
			Pin->DefaultObject = *Replacement;
		}
		const FString Paths = ReplacePaths(Pin->DefaultValue, Map);
		const FString NewDefault = bBones ? AdaptBoneLiteral(Paths) : Paths;
		if (NewDefault != Pin->DefaultValue)
		{
			if (!C.Require(!bVerify, TEXT("Original graph pin default path remains"))) { return false; }
			Pin->DefaultValue = NewDefault;
		}
	}
	return true;
}

bool RewriteBlueprintGraphNodes(FContext& C, UBlueprint* Blueprint, const FReplacements& Map, bool bVerify)
{
	if (!C.Require(C.Allowed(Blueprint), TEXT("Original Blueprint graph operation refused"))) { return false; }
	TArray<UObject*> Owned;
	GetObjectsWithOuter(Blueprint->GetOutermost(), Owned, EGetObjectsFlags::IncludeNestedObjects);
	for (UObject* Object : Owned)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			if (!RewriteGraphNode(C, Node, Map, Blueprint->IsA<UAnimBlueprint>(), bVerify)) { return false; }
		}
	}
	return true;
}

bool RewritePackage(FContext& C, UObject* Asset, const FReplacements& Map, bool bVerify)
{
	if (!C.Require(C.Allowed(Asset), TEXT("Refusing a source package rewrite"))) { return false; }
	TArray<UObject*> Owned;
	GetObjectsWithOuter(Asset->GetOutermost(), Owned, EGetObjectsFlags::IncludeNestedObjects);
	const bool bBones = Asset->IsA<UAnimBlueprint>();
	for (UObject* Object : Owned)
	{
		// UClass/UStruct internals (notably SuperStruct) are never serialized or
		// pointer-patched. Engine ReparentBlueprint/Compile own class lifetime.
		if (!Object || Object->IsA<UStruct>() || Object->IsA<UPackage>()) { continue; }
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			if (!RewriteGraphNode(C, Node, Map, bBones, bVerify)) { return false; }
			continue;
		}
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient)) { continue; }
			for (int32 I = 0; I < It->ArrayDim; ++I)
			{
				if (!RewriteValue(C, *It, It->ContainerPtrToValuePtr<void>(Object, I), Map, bBones, bVerify, 0)) { return false; }
			}
		}
	}
	if (!bVerify) { Asset->MarkPackageDirty(); }
	return true;
}

struct FOverride
{
	FName Variable;
	UClass* NewOwner = nullptr;
	TStrongObjectPtr<UActorComponent> Snapshot;
};

bool BlueprintOwnership(FContext& C, UBlueprint* Blueprint)
{
	if (!C.Require(C.Allowed(Blueprint), TEXT("Original Blueprint operation refused"))) { return false; }
	UPackage* Package = Blueprint->GetOutermost();
	if (Blueprint->SimpleConstructionScript && !C.Require(Blueprint->SimpleConstructionScript->GetOutermost() == Package, TEXT("Private Blueprint aliases an original SCS"))) { return false; }
	if (UInheritableComponentHandler* Handler = Blueprint->GetInheritableComponentHandler(false))
	{
		if (!C.Require(Handler->GetOutermost() == Package, TEXT("Private Blueprint aliases an original inherited handler"))) { return false; }
	}
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (const UEdGraph* Graph : Graphs)
	{
		if (!C.Require(Graph && Graph->GetOutermost() == Package, TEXT("Private Blueprint aliases an original graph"))) { return false; }
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (!C.Require(Node && Node->GetOutermost() == Package, TEXT("Private graph aliases an original node"))) { return false; }
		}
	}
	return C.Require(Blueprint->GeneratedClass && Blueprint->GeneratedClass->GetOutermost() == Package
		&& Blueprint->GeneratedClass->ClassGeneratedBy == Blueprint, TEXT("Private Blueprint class ownership differs"));
}

bool Compile(FContext& C, UBlueprint* Blueprint)
{
	if (!BlueprintOwnership(C, Blueprint)) { return false; }
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FCompilerResultsLog Log;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Log);
	return C.Require(Log.NumErrors == 0 && Blueprint->Status != BS_Error && Blueprint->GeneratedClass,
		TEXT("Private Blueprint compilation failed: ") + Blueprint->GetPathName());
}

bool Reparent(FContext& C, UBlueprint* Blueprint, const FReplacements& Map)
{
	if (!BlueprintOwnership(C, Blueprint)) { return false; }
	UObject* const* Found = Map.Find(Blueprint->ParentClass);
	UClass* Parent = Found ? Cast<UClass>(*Found) : nullptr;
	if (!Parent || Parent == Blueprint->ParentClass) { return true; }
	if (!C.Require(C.Allowed(Parent), TEXT("New parent must be private"))) { return false; }
	TArray<FOverride> Overrides;
	UInheritableComponentHandler* Handler = Blueprint->GetInheritableComponentHandler(false);
	if (Handler)
	{
		if (!C.Require(Handler->GetOutermost() == Blueprint->GetOutermost(), TEXT("Inherited handler is not owned by the private Blueprint"))) { return false; }
		TArray<UActorComponent*> Templates;
		Handler->GetAllTemplates(Templates);
		for (UActorComponent* Template : Templates)
		{
			const FComponentKey Key = Handler->FindKey(Template);
			UObject* const* NewOwner = Map.Find(Key.GetComponentOwner());
			if (!NewOwner) { continue; }
			if (!C.Require(Key.IsSCSKey() && Template && !Template->GetClass()->ClassGeneratedBy, TEXT("Unsupported inherited override; preserve original and stop"))) { return false; }
			FOverride& Override = Overrides.AddDefaulted_GetRef();
			Override.Variable = Key.GetSCSVariableName();
			Override.NewOwner = CastChecked<UClass>(*NewOwner);
			Override.Snapshot.Reset(DuplicateObject<UActorComponent>(Template, GetTransientPackage()));
		}
	}
	// Invoke the engine's reflected editor operation. This avoids a hard link
	// to an extra module and never rewrites UStruct::SuperStruct directly.
	FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("BlueprintEditorLibrary"));
	UClass* Library = LoadClass<UObject>(nullptr, TEXT("/Script/BlueprintEditorLibrary.BlueprintEditorLibrary"));
	UFunction* Function = Library ? Library->FindFunctionByName(TEXT("ReparentBlueprint")) : nullptr;
	if (!C.Require(Function != nullptr, TEXT("Engine ReparentBlueprint API unavailable"))) { return false; }
	FStructOnScope Parameters(Function);
	FObjectPropertyBase* BlueprintParam = FindFProperty<FObjectPropertyBase>(Function, TEXT("Blueprint"));
	FObjectPropertyBase* ParentParam = FindFProperty<FObjectPropertyBase>(Function, TEXT("NewParentClass"));
	if (!C.Require(BlueprintParam && ParentParam, TEXT("Engine reparent signature changed"))) { return false; }
	BlueprintParam->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Blueprint);
	ParentParam->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Parent);
	Library->GetDefaultObject()->ProcessEvent(Function, Parameters.GetStructMemory());
	if (!C.Require(Blueprint->ParentClass == Parent, TEXT("Engine reparent did not apply"))) { return false; }
	Handler = Blueprint->GetInheritableComponentHandler(true);
	if (!C.Require(Handler && Handler->GetOutermost() == Blueprint->GetOutermost(), TEXT("Cannot create private inherited handler"))) { return false; }
	for (FOverride& Override : Overrides)
	{
		UBlueprint* OwnerBP = Cast<UBlueprint>(Override.NewOwner->ClassGeneratedBy);
		USCS_Node* NewNode = nullptr;
		if (OwnerBP && OwnerBP->SimpleConstructionScript)
		{
			for (USCS_Node* Node : OwnerBP->SimpleConstructionScript->GetAllNodes())
			{
				if (Node && Node->GetVariableName() == Override.Variable) { NewNode = Node; break; }
			}
		}
		if (!C.Require(NewNode != nullptr, TEXT("New parent lacks inherited SCS override key"))) { return false; }
		const FComponentKey NewKey(NewNode);
		UActorComponent* Template = Handler->GetOverridenComponentTemplate(NewKey);
		if (!Template) { Template = Handler->CreateOverridenComponentTemplate(NewKey); }
		if (!C.Require(Template && Template->GetOutermost() == Blueprint->GetOutermost()
			&& Template->GetClass() == Override.Snapshot->GetClass(), TEXT("Inherited template ownership/class changed"))) { return false; }
		UEngine::FCopyPropertiesForUnrelatedObjectsParams Copy;
		Copy.bDoDelta = false;
		Copy.bNotifyObjectReplacement = false;
		UEngine::CopyPropertiesForUnrelatedObjects(Override.Snapshot.Get(), Template, Copy);
		for (TFieldIterator<FProperty> It(Template->GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient)) { continue; }
			FString Expected, Actual;
			It->ExportText_InContainer(0, Expected, Override.Snapshot.Get(), nullptr, Override.Snapshot.Get(), PPF_None);
			It->ExportText_InContainer(0, Actual, Template, nullptr, Template, PPF_None);
			if (!C.Require(Expected == Actual, TEXT("Inherited native override value was not preserved: ") + It->GetName())) { return false; }
		}
	}
	C.Add(TEXT("reparented"), Blueprint->GetPathName());
	return Compile(C, Blueprint);
}

// Fixed FootPlant copy contract. No source controller, graph edit or compile is
// used: only the seven qualified root cosmetic reroutes may be normalized.
struct FFootPlantLink
{
	FString Graph, From, To;
	FString Key() const { return Graph + TEXT("|") + From + TEXT("->") + To; }
};
struct FFootPlantSnapshot
{
	TMap<FString, FString> Models, Nodes, Pins;
	TMap<FString, FFootPlantLink> Links;
	TMap<FString, const URigVMGraph*> GraphObjects;
	TMap<FString, const URigVMNode*> NodeObjects;
	TMap<FString, const URigVMPin*> PinObjects;
	TMap<FString, FString> VariableIdentities;
};

FString FootPlantText(FContext& C, FString Value, bool bExpected, bool bAdapted)
{
	if (!bExpected) { return Value; }
	Value.ReplaceInline(SourceFootPlant, PrivateFootPlant, ESearchCase::CaseSensitive);
	if (bAdapted)
	{
		for (const FRow& Row : C.Rows)
		{
			Value.ReplaceInline(*ObjectPath(Row.Source), *ObjectPath(Row.Target), ESearchCase::CaseSensitive);
		}
		Value.ReplaceInline(MannySkeleton, SkeletonPath, ESearchCase::CaseSensitive);
		Value = AdaptBoneLiteral(Value);
	}
	return Value;
}

bool FootPlantField(FContext& C, const UObject* Object, const FName Name, FString& Out)
{
	FProperty* Property = Object->GetClass()->FindPropertyByName(Name);
	if (!C.Require(Property != nullptr, TEXT("Required FootPlant field unavailable: ") + Name.ToString())) { return false; }
	Property->ExportText_InContainer(0, Out, Object, nullptr, nullptr, PPF_None);
	return true;
}

bool SnapshotFootPlant(FContext& C, const UControlRigBlueprint* Rig, bool bExpected, bool bAdapted, FFootPlantSnapshot& Out)
{
	const UPackage* Package = Rig->GetOutermost();
	const FRigVMClient* Client = Rig->URigVMBlueprint::GetRigVMClient();
	if (!C.Require(Client != nullptr, TEXT("FootPlant RigVM client missing"))) { return false; }
	FString Variables;
	for (FBPVariableDescription Variable : Rig->NewVariables)
	{
		const FString Key = TEXT("<asset variables>|") + Variable.VarName.ToString();
		Out.VariableIdentities.Add(Key, Variable.VarGuid.ToString());
		// Duplication regenerates declaration identity. Normalize only this
		// descriptor field in an independent copy, never a GUID in user data.
		Variable.VarGuid.Invalidate();
		FString Value;
		FBPVariableDescription::StaticStruct()->ExportText(Value, &Variable, nullptr, nullptr, PPF_None, nullptr);
		Variables += Value + TEXT("\n");
	}
	Out.Models.Add(TEXT("<asset variables>"), FootPlantText(C, Variables, bExpected, bAdapted));
	for (const URigVMGraph* Graph : Client->GetAllModels(true, true))
	{
		if (!C.Require(Graph && Graph->GetOutermost() == Package && Graph->IsIn(Rig), TEXT("FootPlant graph ownership differs"))) { return false; }
		const FString GraphKey = Graph->GetPathName(Rig);
		if (Out.GraphObjects.Contains(GraphKey))
		{
			if (!C.Require(Out.GraphObjects.FindChecked(GraphKey) == Graph, TEXT("Duplicate FootPlant graph identity"))) { return false; }
			continue;
		}
		Out.GraphObjects.Add(GraphKey, Graph);
		FString Locals;
		for (FRigVMGraphVariableDescription Variable : Graph->GetLocalVariables(false))
		{
			Out.VariableIdentities.Add(GraphKey + TEXT("|") + Variable.Name.ToString(), Variable.Guid.ToString());
			Variable.Guid.Invalidate();
			FString Value;
			FRigVMGraphVariableDescription::StaticStruct()->ExportText(Value, &Variable, nullptr, nullptr, PPF_None, nullptr);
			Locals += Value + TEXT("\n");
		}
		Out.Models.Add(GraphKey, Graph->GetClass()->GetPathName() + TEXT("\n") + FootPlantText(C, Locals, bExpected, bAdapted));
		for (const URigVMNode* Node : Graph->GetNodes())
		{
			if (!C.Require(Node && Node->GetGraph() == Graph && Node->GetOutermost() == Package && Node->IsIn(Graph), TEXT("FootPlant node ownership differs: ") + GraphKey)) { return false; }
			const FString NodeKey = GraphKey + TEXT("|") + Node->GetName();
			if (!C.Require(!Out.Nodes.Contains(NodeKey), TEXT("Duplicate FootPlant node path: ") + NodeKey)) { return false; }
			FString Signature = Node->GetClass()->GetPathName();
			if (const URigVMTemplateNode* Template = Cast<URigVMTemplateNode>(Node))
			{
				const UScriptStruct* Struct = Template->GetScriptStruct();
				Signature += TEXT("\nstruct=") + (Struct ? Struct->GetPathName() : FString()) + TEXT("\nmethod=") + Template->GetMethodName().ToString()
					+ TEXT("\nnotation=") + Template->GetNotation().ToString();
			}
			Out.Nodes.Add(NodeKey, FootPlantText(C, Signature, bExpected, bAdapted));
			Out.NodeObjects.Add(NodeKey, Node);
			TArray<URigVMPin*> Pins = Node->GetPins();
			for (int32 I = 0; I < Pins.Num(); ++I)
			{
				const URigVMPin* Pin = Pins[I];
				if (!C.Require(Pin && Pin->GetNode() == Node && Pin->GetGraph() == Graph && Pin->GetOutermost() == Package
					&& Pin->IsIn(Node), TEXT("FootPlant pin ownership differs: ") + NodeKey)) { return false; }
				const FString PinPath = Pin->GetPinPath();
				const FString PinKey = GraphKey + TEXT("|") + PinPath;
				if (!C.Require(!Out.Pins.Contains(PinKey) && Graph->FindPin(PinPath) == Pin, TEXT("FootPlant pin lookup differs: ") + PinKey)) { return false; }
				FString TypePath;
				if (!FootPlantField(C, Pin, TEXT("CPPTypeObjectPath"), TypePath)) { return false; }
				const UObject* Type = Pin->GetCPPTypeObject();
				const FString DefaultValue = FootPlantText(C, Pin->GetDefaultValue(), bExpected, bAdapted);
				Signature = Pin->GetCPPType() + TEXT("\nobject=") + (Type ? Type->GetPathName() : FString()) + TEXT("\nstored=") + TypePath
					+ TEXT("\ndirection=") + FString::FromInt(static_cast<int32>(Pin->GetDirection())) + TEXT("\ndefault=") + DefaultValue;
				Out.Pins.Add(PinKey, FootPlantText(C, Signature, bExpected, bAdapted));
				Out.PinObjects.Add(PinKey, Pin);
				Pins.Append(Pin->GetSubPins());
			}
		}
		for (const URigVMLink* Link : Graph->GetLinks())
		{
			if (!C.Require(Link && Link->GetGraph() == Graph && Link->GetOutermost() == Package && Link->IsIn(Graph), TEXT("FootPlant link ownership differs: ") + GraphKey)) { return false; }
			const URigVMPin* From = Link->GetSourcePin();
			const URigVMPin* To = Link->GetTargetPin();
			const FFootPlantLink Entry{ GraphKey, Link->GetSourcePinPath(), Link->GetTargetPinPath() };
			if (!C.Require(From && To && From->GetGraph() == Graph && To->GetGraph() == Graph && From->GetOutermost() == Package && To->GetOutermost() == Package
				&& Graph->FindPin(Entry.From) == From && Graph->FindPin(Entry.To) == To && From->GetPinPath() == Entry.From && To->GetPinPath() == Entry.To
				&& From->GetLinks().Contains(Link) && To->GetLinks().Contains(Link), TEXT("FootPlant active link/pin membership differs: ") + Rig->GetPathName() + TEXT(":") + Entry.Key()))
			{
				C.Add(TEXT("footplant_failed_link"), Rig->GetPathName() + TEXT(":") + Entry.Key());
				C.Add(TEXT("footplant_failed_source_pin"), From ? From->GetPathName() : TEXT("missing"));
				C.Add(TEXT("footplant_failed_target_pin"), To ? To->GetPathName() : TEXT("missing"));
				C.Add(TEXT("footplant_failed_source_backref"), From && From->GetLinks().Contains(Link) ? TEXT("true") : TEXT("false"));
				C.Add(TEXT("footplant_failed_target_backref"), To && To->GetLinks().Contains(Link) ? TEXT("true") : TEXT("false"));
				return false;
			}
			if (!C.Require(!Out.Links.Contains(Entry.Key()), TEXT("Duplicate FootPlant active link: ") + Entry.Key())) { return false; }
			Out.Links.Add(Entry.Key(), Entry);
		}
	}
	for (const TPair<FString, FString>& Identity : Out.VariableIdentities)
	{
		C.Add(Rig->GetPathName() == SourceFootPlant ? TEXT("footplant_source_variable_identity") : TEXT("footplant_private_variable_identity"), Identity.Key + TEXT("=") + Identity.Value);
	}
	return C.Require(Out.GraphObjects.Num() == 13, TEXT("Expected all thirteen FootPlant models including recursive function library"));
}

bool FootPlantMapEqual(FContext& C, const TMap<FString, FString>& Expected, const TMap<FString, FString>& Actual, const FString& Kind)
{
	if (!C.Require(Expected.Num() == Actual.Num(), TEXT("FootPlant ") + Kind + TEXT(" count differs"))) { return false; }
	for (const TPair<FString, FString>& Pair : Expected)
	{
		const FString* Value = Actual.Find(Pair.Key);
		if (!C.Require(Value && *Value == Pair.Value, TEXT("FootPlant ") + Kind + TEXT(" differs: ") + Pair.Key))
		{
			C.Add(TEXT("footplant_expected_") + Kind, Pair.Key + TEXT("=") + Pair.Value);
			C.Add(TEXT("footplant_actual_") + Kind, Value ? *Value : TEXT("missing"));
			return false;
		}
	}
	return true;
}

bool ExpectedFootPlant(FContext& C, FFootPlantSnapshot& Expected, bool bAdapted, TArray<FFootPlantLink>& Bypasses)
{
	const UControlRigBlueprint* Source = LoadObject<UControlRigBlueprint>(nullptr, SourceFootPlant);
	if (!C.Require(Source && !Source->GetOutermost()->IsDirty(), TEXT("Original FootPlant must exist and remain clean"))
		|| !SnapshotFootPlant(C, Source, true, bAdapted, Expected)) { return false; }
	if (!C.Require(Expected.Nodes.Num() == 330 && Expected.Links.Num() == 376, TEXT("Original FootPlant topology changed"))) { return false; }
	const FString RootGraph = TEXT("RigVMModel");
	TSet<FString> Reroutes;
	TMap<FString, FFootPlantLink> Outgoing;
	for (const TCHAR* Name : { TEXT("RerouteNode_2"), TEXT("RerouteNode_4"), TEXT("RerouteNode_7"), TEXT("RerouteNode_8"), TEXT("RerouteNode_9"), TEXT("RerouteNode_10"), TEXT("RerouteNode_11") })
	{
		const FString NodeKey = RootGraph + TEXT("|") + Name;
		const URigVMNode* const* Found = Expected.NodeObjects.Find(NodeKey);
		const URigVMRerouteNode* Node = Found ? Cast<URigVMRerouteNode>(*Found) : nullptr;
		if (!C.Require(Node && !Node->IsLiteral() && Node->GetPins().Num() == 1 && Node->GetPins()[0]->GetName() == TEXT("Value")
			&& Node->GetPins()[0]->GetDirection() == ERigVMPinDirection::IO, TEXT("Root reroute is not the approved single-value passthrough: ") + NodeKey)) { return false; }
		const FString ValuePath = FString(Name) + TEXT(".Value");
		int32 Inputs = 0, Outputs = 0;
		for (const TPair<FString, FFootPlantLink>& Pair : Expected.Links)
		{
			const FFootPlantLink& Link = Pair.Value;
			if (Link.Graph != RootGraph) { continue; }
			if (Link.To == ValuePath)
			{
				++Inputs;
				const URigVMPin* Incoming = Expected.PinObjects.FindRef(RootGraph + TEXT("|") + Link.From);
				const URigVMPin* Value = Node->GetPins()[0];
				const bool bSameType = Incoming && Incoming->GetCPPType() == Value->GetCPPType() && Incoming->GetCPPTypeObject() == Value->GetCPPTypeObject();
				const bool bOriginalWidening = Incoming && NodeKey == TEXT("RigVMModel|RerouteNode_2")
					&& Link.From == TEXT("AlphaInterp_1.Result") && Link.To == TEXT("RerouteNode_2.Value")
					&& Incoming->GetCPPType() == TEXT("float") && Value->GetCPPType() == TEXT("double")
					&& Incoming->GetCPPTypeObject() == nullptr && Value->GetCPPTypeObject() == nullptr;
				if (bOriginalWidening) { C.Add(TEXT("footplant_original_widening"), Link.Key() + TEXT(" types=float->double; original source conversion retained without a cast node")); }
				if (!C.Require(bSameType || bOriginalWidening, TEXT("Root reroute changes its input value type: ") + NodeKey
					+ TEXT(" types=") + (Incoming ? Incoming->GetCPPType() : FString(TEXT("missing"))) + TEXT("->") + Value->GetCPPType())) { return false; }
			}
			if (Link.From == ValuePath) { ++Outputs; Outgoing.Add(NodeKey, Link); }
		}
		if (!C.Require(Inputs == 1 && Outputs == 1, TEXT("Root reroute input/output arity changed: ") + NodeKey)) { return false; }
		Reroutes.Add(NodeKey);
	}
	auto NodeKeyOfPin = [](const FFootPlantLink& Link, bool bFrom)
	{
		FString Name, Tail;
		(bFrom ? Link.From : Link.To).Split(TEXT("."), &Name, &Tail);
		return Link.Graph + TEXT("|") + Name;
	};
	TArray<FString> RemoveLinks;
	for (const TPair<FString, FFootPlantLink>& Pair : Expected.Links)
	{
		const bool bFrom = Reroutes.Contains(NodeKeyOfPin(Pair.Value, true));
		const bool bTo = Reroutes.Contains(NodeKeyOfPin(Pair.Value, false));
		if (bFrom || bTo) { RemoveLinks.Add(Pair.Key); }
		if (bFrom || !bTo) { continue; }
		FFootPlantLink Direct = Pair.Value;
		TSet<FString> Visited;
		FString Next = NodeKeyOfPin(Direct, false);
		while (Reroutes.Contains(Next))
		{
			if (!C.Require(!Visited.Contains(Next) && Outgoing.Contains(Next), TEXT("Invalid root reroute chain"))) { return false; }
			Visited.Add(Next);
			Direct.To = Outgoing.FindChecked(Next).To;
			Next = NodeKeyOfPin(Direct, false);
		}
		Bypasses.Add(Direct);
	}
	TSet<FString> Required{
		TEXT("RigVMModel|AlphaInterp.Result->MathFloatAdd.B"),
		TEXT("RigVMModel|AlphaInterp_1.Result->MathTransformRotateVector_1.Vector.X"),
		TEXT("RigVMModel|GetTransform_8_3.Transform->TwoBoneIKSimplePerItem.Effector") };
	if (!C.Require(RemoveLinks.Num() == 10 && Bypasses.Num() == 3, TEXT("Unexpected root reroute chain closure"))) { return false; }
	for (const FFootPlantLink& Link : Bypasses)
	{
		if (!C.Require(Required.Remove(Link.Key()) == 1, TEXT("Unexpected source-derived FootPlant direct link"))) { return false; }
	}
	for (const FString& Key : RemoveLinks) { Expected.Links.Remove(Key); }
	for (const FFootPlantLink& Link : Bypasses) { Expected.Links.Add(Link.Key(), Link); }
	for (const FString& Key : Reroutes)
	{
		Expected.Nodes.Remove(Key);
		TArray<FString> RemovePins;
		for (const TPair<FString, FString>& Pin : Expected.Pins) { if (Pin.Key.StartsWith(Key + TEXT("."))) { RemovePins.Add(Pin.Key); } }
		for (const FString& Pin : RemovePins) { Expected.Pins.Remove(Pin); }
	}
	return C.Require(Expected.Nodes.Num() == 323 && Expected.Links.Num() == 369, TEXT("Normalized FootPlant topology count differs")) && CleanSources(C);
}

bool CheckFootPlantTopology(FContext& C, bool bAdapted, bool bMissingDirect, TArray<FFootPlantLink>* OutBypasses = nullptr)
{
	FFootPlantSnapshot Expected, Actual;
	TArray<FFootPlantLink> Bypasses;
	UControlRigBlueprint* Rig = Private<UControlRigBlueprint>(C, PrivateFootPlant);
	if (!Rig || !ExpectedFootPlant(C, Expected, bAdapted, Bypasses) || !SnapshotFootPlant(C, Rig, false, bAdapted, Actual)) { return false; }
	if (!FootPlantMapEqual(C, Expected.Models, Actual.Models, TEXT("models/variables"))
		|| !FootPlantMapEqual(C, Expected.Nodes, Actual.Nodes, TEXT("nodes/unit/method/notation"))
		|| !FootPlantMapEqual(C, Expected.Pins, Actual.Pins, TEXT("recursive pin types/defaults"))) { return false; }
	if (bMissingDirect) { for (const FFootPlantLink& Link : Bypasses) { Expected.Links.Remove(Link.Key()); } }
	if (!C.Require(Expected.Links.Num() == Actual.Links.Num(), TEXT("FootPlant active link count differs"))) { return false; }
	for (const TPair<FString, FFootPlantLink>& Link : Expected.Links)
	{
		if (!C.Require(Actual.Links.Contains(Link.Key), TEXT("FootPlant active link differs: ") + Link.Key)) { return false; }
	}
	C.Add(TEXT("footplant_topology"), FString::Printf(TEXT("models=13 nodes=323 links=%d; complete recursive pins/unit/method/notation and private linked-pin ownership/membership checked; adapted=%s"),
		Actual.Links.Num(), bAdapted ? TEXT("true") : TEXT("false")));
	if (OutBypasses) { *OutBypasses = Bypasses; }
	return CleanSources(C);
}

bool RepairFootPlantCopy(FContext& C)
{
	const FString Package = FPackageName::ObjectPathToPackageName(FString(PrivateFootPlant));
	if (!C.Require(!FPaths::FileExists(FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension())), TEXT("FootPlant repair requires an unsaved new private package"))) { return false; }
	const FString SourceFile = FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(FString(SourceFootPlant)), FPackageName::GetAssetPackageExtension());
	TArray<uint8> BeforeBytes;
	if (!C.Require(FFileHelper::LoadFileToArray(BeforeBytes, *SourceFile), TEXT("Cannot protect original FootPlant file"))) { return false; }
	const FBlake3Hash BeforeHash = FBlake3::HashBuffer(BeforeBytes.GetData(), static_cast<uint64>(BeforeBytes.Num()));
	C.Add(TEXT("original_footplant_file_blake3_256"), LexToString(BeforeHash));
	const UControlRigBlueprint* Original = LoadObject<UControlRigBlueprint>(nullptr, SourceFootPlant);
	FFootPlantSnapshot SourceBefore;
	if (!C.Require(Original != nullptr, TEXT("Original FootPlant unavailable")) || !SnapshotFootPlant(C, Original, false, false, SourceBefore)) { return false; }
	TArray<FFootPlantLink> Bypasses;
	if (!CheckFootPlantTopology(C, false, true, &Bypasses)) { return false; }
	UControlRigBlueprint* Rig = Private<UControlRigBlueprint>(C, PrivateFootPlant);
	URigVMGraph* RootModel = Rig ? Rig->GetModel(TEXT("RigVMModel")) : nullptr;
	URigVMController* Controller = RootModel ? Rig->GetController(RootModel) : nullptr; // private only
	if (!C.Require(Rig && RootModel && C.Allowed(RootModel) && Controller && Controller->GetGraph() == RootModel
		&& !Controller->IsIn(Original), TEXT("Private FootPlant root controller unavailable"))) { return false; }
	for (const FFootPlantLink& Link : Bypasses)
	{
		const URigVMPin* From = RootModel->FindPin(Link.From);
		const URigVMPin* To = RootModel->FindPin(Link.To);
		if (!C.Require(From && To && From->GetGraph() == RootModel && To->GetGraph() == RootModel && C.Allowed(From) && C.Allowed(To), TEXT("Private FootPlant repair endpoint missing"))) { return false; }
		C.Add(TEXT("footplant_repair_link"), Link.Key() + TEXT(" types=") + From->GetCPPType() + TEXT("->") + To->GetCPPType());
		// Preserve the source's float-to-double terminal link. Do not create a
		// cast node or change global controller type-casting settings.
		if (!C.Require(Controller->AddLink(Link.From, Link.To, false, false, ERigVMPinDirection::Output, false), TEXT("Cannot restore source-derived FootPlant link: ") + Link.Key())) { return false; }
	}
	Rig->RecompileVM();
	if (!C.Require(Rig->GetCompileLog().NumErrors == 0, TEXT("Repaired FootPlant VM compile failed")) || !Compile(C, Rig)
		|| !C.Require(Rig->GetCompileLog().NumErrors == 0, TEXT("Repaired FootPlant final compile failed")) || !CheckFootPlantTopology(C, false, false)) { return false; }
	TArray<uint8> AfterBytes;
	if (!C.Require(FFileHelper::LoadFileToArray(AfterBytes, *SourceFile)
		&& FBlake3::HashBuffer(AfterBytes.GetData(), static_cast<uint64>(AfterBytes.Num())) == BeforeHash, TEXT("Original FootPlant file changed during repair"))) { return false; }
	FFootPlantSnapshot SourceAfter;
	if (!SnapshotFootPlant(C, Original, false, false, SourceAfter)
		|| !FootPlantMapEqual(C, SourceBefore.Models, SourceAfter.Models, TEXT("original models"))
		|| !FootPlantMapEqual(C, SourceBefore.Nodes, SourceAfter.Nodes, TEXT("original nodes"))
		|| !FootPlantMapEqual(C, SourceBefore.Pins, SourceAfter.Pins, TEXT("original pins"))
		|| !FootPlantMapEqual(C, SourceBefore.VariableIdentities, SourceAfter.VariableIdentities, TEXT("original declaration identities"))
		|| !C.Require(SourceBefore.Links.Num() == SourceAfter.Links.Num(), TEXT("Original FootPlant link count changed"))) { return false; }
	for (const TPair<FString, FFootPlantLink>& Link : SourceBefore.Links)
	{
		if (!C.Require(SourceAfter.Links.Contains(Link.Key), TEXT("Original FootPlant link changed: ") + Link.Key)) { return false; }
	}
	C.Add(TEXT("footplant_copy_repair"), TEXT("exact three source-derived direct links restored; seven root cosmetic reroutes normalized; source controller/graph untouched; models=13 nodes=323 links=369; saves=0"));
	return CleanSources(C);
}

bool FootPlantFootAnchor(FContext& C, bool bCreate)
{
	UControlRigBlueprint* Rig = Private<UControlRigBlueprint>(C, PrivateFootPlant);
	URigHierarchy* Hierarchy = Rig ? Rig->GetHierarchy() : nullptr;
	if (!C.Require(Hierarchy && C.Allowed(Hierarchy) && Hierarchy->GetOutermost() == Rig->GetOutermost(), TEXT("Private FootPlant anchor hierarchy unavailable"))) { return false; }
	const FRigElementKey Key(TEXT("VB ik_foot_root_foot_l"), ERigElementType::Bone);
	const FRigElementKey Parent(TEXT("ik_foot_root"), ERigElementType::Bone);
	const FRigElementKey Anchor(TEXT("foot_l"), ERigElementType::Bone);
	if (!C.Require(Hierarchy->GetIndex(Parent) != INDEX_NONE && Hierarchy->GetIndex(Anchor) != INDEX_NONE, TEXT("Private FootPlant foot anchor parent/endpoint missing"))) { return false; }
	const FTransform Expected = Hierarchy->GetInitialGlobalTransform(Anchor);
	if (Hierarchy->GetIndex(Key) == INDEX_NONE)
	{
		if (!C.Require(bCreate, TEXT("Saved private FootPlant foot anchor missing"))) { return false; }
		URigHierarchyController* Controller = Hierarchy->GetController(true);
		if (!C.Require(Controller && Controller->GetHierarchy() == Hierarchy, TEXT("Private FootPlant anchor controller ownership differs"))) { return false; }
		// This is a private ControlRig user bone used by the original FootTrace
		// graph, not another virtual bone in the mesh or its skeleton asset.
		if (!C.Require(Controller->AddBone(Key.Name, Parent, Expected, true, ERigBoneType::User, false, false) == Key,
			TEXT("Cannot create the exact missing private FootPlant foot anchor"))) { return false; }
	}
	const FRigBoneElement* Bone = Hierarchy->Find<FRigBoneElement>(Key);
	const FTransform Actual = Hierarchy->GetInitialGlobalTransform(Key);
	const double PositionError = FVector::Distance(Actual.GetTranslation(), Expected.GetTranslation());
	const double AngleError = Actual.GetRotation().AngularDistance(Expected.GetRotation());
	const double ScaleError = (Actual.GetScale3D() - Expected.GetScale3D()).GetAbsMax();
	C.Add(TEXT("footplant_private_foot_anchor"), FString::Printf(TEXT("Bone=VB ik_foot_root_foot_l parent=%s initialAnchor=foot_l positionErrorCm=%.9f rotationErrorRad=%.9f scaleError=%.9f; hierarchy user bone only, mesh/skeleton counts unchanged"),
		*Hierarchy->GetFirstParent(Key).Name.ToString(), PositionError, AngleError, ScaleError));
	return C.Require(Bone && Bone->BoneType == ERigBoneType::User && Hierarchy->GetFirstParent(Key) == Parent
		&& !Actual.ContainsNaN() && FMath::IsFinite(PositionError) && FMath::IsFinite(AngleError) && FMath::IsFinite(ScaleError)
		&& PositionError <= 1.e-3 && AngleError <= 1.e-3 && ScaleError <= 1.e-3,
		TEXT("Private FootPlant user-bone type/parent/initial foot anchor differs")) && CleanSources(C);
}

bool AdaptFootPlant(FContext& C)
{
	if (!CheckFootPlantTopology(C, true, false)) { return false; }
	UControlRigBlueprint* Rig = Private<UControlRigBlueprint>(C, TEXT("/Game/DreamCatcher/AI/Inspector/Assets/CR_Mannequin_FootPlant.CR_Mannequin_FootPlant"));
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	if (!Rig || !Mesh) { return false; }
	URigHierarchy* Hierarchy = Rig->GetHierarchy();
	if (!C.Require(Hierarchy && Hierarchy->GetOutermost() == Rig->GetOutermost(), TEXT("Private FootPlant hierarchy ownership unavailable"))) { return false; }
	URigHierarchyController* Controller = Hierarchy->GetController(true);
	if (!C.Require(Controller != nullptr, TEXT("Private FootPlant hierarchy controller unavailable"))) { return false; }
	const USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, MannyMesh);
	if (!C.Require(Manny != nullptr, TEXT("Source reference mesh is missing"))) { return false; }
	struct FRigOnly { FRigElementKey Key, Parent; FTransform Local; };
	TArray<FRigOnly> RigOnly;
	for (const FRigElementKey& Key : Hierarchy->GetAllKeys(true, ERigElementType::Bone))
	{
		if (Manny->GetRefSkeleton().FindBoneIndex(Key.Name) == INDEX_NONE)
		{
			RigOnly.Add({ Key, Hierarchy->GetFirstParent(Key), Hierarchy->GetInitialLocalTransform(Key) });
		}
	}
	const int32 ModelsBefore = Rig->GetAllModels().Num();
	int32 NodesBefore = 0;
	for (const URigVMGraph* Model : Rig->GetAllModels())
	{
		if (!C.Require(Model && Model->GetOutermost() == Rig->GetOutermost(), TEXT("Private FootPlant aliases an original VM model"))) { return false; }
		for (const URigVMNode* Node : Model->GetNodes())
		{
			if (!C.Require(Node && Node->GetOutermost() == Rig->GetOutermost(), TEXT("Private FootPlant aliases an original VM node"))) { return false; }
		}
		NodesBefore += Model->GetNodes().Num();
	}
	const TArray<FRigElementKey> Imported = Controller->ImportBones(Mesh, NAME_None, true, true, false, false);
	if (!C.Require(!Imported.IsEmpty(), TEXT("Cannot import private Inspector bone transforms into FootPlant"))) { return false; }
	for (const FRigOnly& Helper : RigOnly)
	{
		FRigElementKey Parent = Helper.Parent;
		Parent.Name = InspectorBone(Parent.Name);
		if (!C.Require(!Parent.IsValid() || Hierarchy->GetIndex(Parent) != INDEX_NONE, TEXT("Rig-only helper lost its parent"))) { return false; }
		FTransform Global = Parent.IsValid() ? Helper.Local * Hierarchy->GetInitialGlobalTransform(Parent) : Helper.Local;
		FName Anchor = NAME_None;
		if (Helper.Key.Name == TEXT("ik_ball_l")) { Anchor = TEXT("ball_l"); }
		if (Helper.Key.Name == TEXT("ik_ball_r")) { Anchor = TEXT("ball_r"); }
		if (Helper.Key.Name == TEXT("VB ik_foot_root_foot_l")) { Anchor = TEXT("foot_l"); }
		if (!Anchor.IsNone()) { Global = Hierarchy->GetInitialGlobalTransform(FRigElementKey(Anchor, ERigElementType::Bone)); }
		if (Hierarchy->GetIndex(Helper.Key) == INDEX_NONE)
		{
			if (!C.Require(Controller->AddBone(Helper.Key.Name, Parent, Global, true, ERigBoneType::User, false, false) == Helper.Key, TEXT("Cannot preserve Rig-only helper"))) { return false; }
		}
		else { Hierarchy->SetInitialGlobalTransform(Helper.Key, Global, false, false); }
	}
	if (!FootPlantFootAnchor(C, true)) { return false; }
	TArray<URigVMPin*> Pins;
	for (const URigVMGraph* Model : Rig->GetAllModels())
	{
		for (const URigVMNode* Node : Model->GetNodes()) { Pins.Append(Node->GetPins()); }
	}
	int32 LiteralKeys = 0;
	for (int32 I = 0; I < Pins.Num(); ++I)
	{
		const URigVMPin* Pin = Pins[I];
		Pins.Append(Pin->GetSubPins());
		if (Pin->GetCPPType() != TEXT("FRigElementKey")) { continue; }
		FRigElementKey Key;
		const FString Value = Pin->GetDefaultValue();
		if (Value.IsEmpty()) { continue; }
		if (!C.Require(FRigElementKey::StaticStruct()->ImportText(*Value, &Key, Rig, PPF_None, GWarn, TEXT("FRigElementKey")) != nullptr, TEXT("Cannot read original FootPlant key literal"))) { return false; }
		if (Key.IsValid())
		{
			++LiteralKeys;
			if (!C.Require(Hierarchy->GetIndex(Key) != INDEX_NONE, TEXT("FootPlant literal key no longer exists: ") + Key.Name.ToString())) { return false; }
		}
	}
	Rig->SetPreviewMesh(Mesh, true);
	Rig->GetSourceHierarchyImport() = Mesh;
	Rig->RecompileVM();
	if (!C.Require(Rig->GetCompileLog().NumErrors == 0, TEXT("Adapted FootPlant VM compile failed"))) { return false; }
	int32 NodesAfter = 0;
	for (const URigVMGraph* Model : Rig->GetAllModels()) { NodesAfter += Model->GetNodes().Num(); }
	if (!C.Require(Rig->GetAllModels().Num() == ModelsBefore && NodesBefore == NodesAfter && LiteralKeys > 0, TEXT("FootPlant models/nodes or literal bone keys changed unexpectedly"))) { return false; }
	for (const FName Bone : { FName(TEXT("ik_foot_root")), FName(TEXT("ik_foot_l")), FName(TEXT("ik_foot_r")), FName(TEXT("spine_03")) })
	{
		if (!C.Require(Hierarchy->GetIndex(FRigElementKey(Bone, ERigElementType::Bone)) != INDEX_NONE, TEXT("Required FootPlant bone missing"))) { return false; }
	}
	C.Add(TEXT("footplant"), TEXT("original VM models retained; private Inspector hierarchy/initial transforms imported; required IK bones present"));
	C.Add(TEXT("footplant_rig_only_helpers_preserved"), FString::FromInt(RigOnly.Num()));
	return Compile(C, Rig) && C.Require(Rig->GetCompileLog().NumErrors == 0, TEXT("Adapted FootPlant final compile failed"))
		&& FootPlantFootAnchor(C, false) && CheckFootPlantTopology(C, true, false);
}

struct FPawnMeshRoles
{
	ACharacter* RuntimePawn = nullptr;
	USkeletalMeshComponent* Runtime = nullptr;
	USkeletalMeshComponent* Schema = nullptr;

	bool Resolve(FContext& C, UBlueprint* Blueprint)
	{
		if (!C.Require(Blueprint != nullptr && C.Allowed(Blueprint), TEXT("Private pawn Blueprint required for component roles"))) { return false; }
		UClass* RuntimeClass = Blueprint->GeneratedClass.Get();
		if (!C.Require(RuntimeClass != nullptr && C.Allowed(RuntimeClass) && RuntimeClass->ClassGeneratedBy == Blueprint,
			TEXT("Current private pawn generated class identity differs"))) { return false; }
		RuntimePawn = Cast<ACharacter>(RuntimeClass->GetDefaultObject(false));
		Runtime = RuntimePawn ? RuntimePawn->GetMesh() : nullptr;
		if (!C.Require(RuntimePawn != nullptr && RuntimePawn->GetClass() == RuntimeClass && RuntimePawn->HasAnyFlags(RF_ClassDefaultObject)
			&& C.Allowed(RuntimePawn) && Runtime != nullptr && C.Allowed(Runtime) && Runtime->GetOuter() == RuntimePawn
			&& Runtime->GetFName() == TEXT("CharacterMesh0"), TEXT("Current private generated CDO/GetMesh identity unavailable"))) { return false; }
		// The incomplete, transient skeleton class is an editor schema, not the
		// gameplay class. Observe its existing CDO only; never create one here.
		FString SchemaIdentity = TEXT("no current skeleton class");
		if (UClass* SchemaClass = Blueprint->SkeletonGeneratedClass.Get())
		{
			SchemaIdentity = SchemaClass->GetPathName() + TEXT(" existingCDO=none");
			if (!C.Require(SchemaClass != RuntimeClass && C.Allowed(SchemaClass) && SchemaClass->ClassGeneratedBy == Blueprint,
				TEXT("Current private pawn skeleton class identity differs"))) { return false; }
			if (UObject* ExistingCDO = SchemaClass->GetDefaultObject(false))
			{
				SchemaIdentity = SchemaClass->GetPathName() + TEXT(" existingCDO=") + ExistingCDO->GetPathName();
				ACharacter* SchemaPawn = Cast<ACharacter>(ExistingCDO);
				if (!C.Require(SchemaPawn != nullptr && SchemaPawn != RuntimePawn && SchemaPawn->GetClass() == SchemaClass
					&& SchemaPawn->HasAnyFlags(RF_ClassDefaultObject) && C.Allowed(SchemaPawn), TEXT("Existing private schema CDO identity differs"))) { return false; }
				Schema = SchemaPawn->GetMesh();
				if (Schema && !C.Require(Schema != Runtime && C.Allowed(Schema) && Schema->GetOuter() == SchemaPawn,
					TEXT("Existing private schema mesh identity/ownership differs"))) { return false; }
			}
		}
		C.Add(TEXT("private_pawn_mesh_roles"), Blueprint->GetPathName() + TEXT(" runtimeClass=") + RuntimeClass->GetPathName()
			+ TEXT(" runtimeCDO=") + RuntimePawn->GetPathName() + TEXT(" runtimeMesh=") + Runtime->GetPathName()
			+ TEXT(" schema=") + SchemaIdentity + TEXT(" existingSchemaMesh=") + (Schema ? Schema->GetPathName() : FString(TEXT("none"))));
		return true;
	}

	bool Identify(FContext& C, const USkeletalMeshComponent* Component, bool& bIsRuntime) const
	{
		if (Component == Runtime) { bIsRuntime = true; return true; }
		if (Schema && Component == Schema) { bIsRuntime = false; return true; }
		const FString Actual = Component ? Component->GetPathName() : FString(TEXT("none"));
		const FString Outer = Component && Component->GetOuter() ? Component->GetOuter()->GetPathName() : FString(TEXT("none"));
		const FString Class = Component ? Component->GetClass()->GetPathName() : FString(TEXT("none"));
		const FString Expected = TEXT(" runtime=") + (Runtime ? Runtime->GetPathName() : FString(TEXT("none")))
			+ TEXT(" schema=") + (Schema ? Schema->GetPathName() : FString(TEXT("none")));
		C.Add(TEXT("unknown_private_pawn_mesh_template"), Actual + TEXT(" class=") + Class + TEXT(" outer=") + Outer + Expected);
		return C.Require(false, TEXT("Unknown CharacterMesh0 template: ") + Actual + TEXT(" class=") + Class + TEXT(" outer=") + Outer + Expected);
	}
};

bool PawnBodyAliases(FContext& C, UObject* Asset, const FReplacements& Base, FReplacements& PawnMap)
{
	if (!C.Require(&Base != &PawnMap, TEXT("Pawn body aliases require an independent local map"))) { return false; }
	if (!C.Require(Asset != nullptr && C.Allowed(Asset) && Asset->IsA<UBlueprint>()
		&& (Asset->GetPathName() == TEXT("/Game/DreamCatcher/AI/Inspector/Assets/BP_DC_RiflePawn.BP_DC_RiflePawn")
			|| Asset->GetPathName() == TEXT("/Game/DreamCatcher/AI/Inspector/Assets/BP_DC_LyraInventoryPawn.BP_DC_LyraInventoryPawn")),
		TEXT("Body mesh aliases are restricted to the two exact private pawn Blueprints"))) { return false; }
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	USkeleton* Skeleton = Private<USkeleton>(C, SkeletonPath);
	UPhysicsAsset* Physics = Private<UPhysicsAsset>(C, PhysicsPath);
	if (!Mesh || !Skeleton || !Physics) { return false; }
	// Only this local map gains both original body styles. In particular the
	// retarget source rigs and every other package keep the generic base map.
	PawnMap = Base;
	for (const TCHAR* SourcePath : { MannyMesh, TEXT("/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU.SKM_HARU") })
	{
		USkeletalMesh* SourceMesh = LoadObject<USkeletalMesh>(nullptr, SourcePath);
		USkeleton* SourceSkeleton = SourceMesh ? SourceMesh->GetSkeleton() : nullptr;
		UPhysicsAsset* SourcePhysics = SourceMesh ? SourceMesh->GetPhysicsAsset() : nullptr;
		if (!C.Require(SourceMesh != nullptr && SourceSkeleton != nullptr && SourcePhysics != nullptr,
			TEXT("Original pawn body mesh/skeleton/physics alias unavailable: ") + FString(SourcePath))) { return false; }
		PawnMap.Add(SourceMesh, Mesh);
		PawnMap.Add(SourceSkeleton, Skeleton);
		PawnMap.Add(SourcePhysics, Physics);
		C.Add(TEXT("private_pawn_body_aliases"), Asset->GetPathName() + TEXT(" mesh=") + SourceMesh->GetPathName() + TEXT("->") + Mesh->GetPathName()
			+ TEXT(" skeleton=") + SourceSkeleton->GetPathName() + TEXT("->") + Skeleton->GetPathName()
			+ TEXT(" physics=") + SourcePhysics->GetPathName() + TEXT("->") + Physics->GetPathName());
	}
	return CleanSources(C);
}

bool BindPawnMeshes(FContext& C, UObject* Asset, const FReplacements& Map, bool bRewriteGraph = true)
{
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	UPhysicsAsset* PA = Private<UPhysicsAsset>(C, PhysicsPath);
	UBlueprint* Anim = Private<UBlueprint>(C, TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.ABP_Mannequin_Base"));
	if (!Mesh || !PA || !Anim) { return false; }
	UBlueprint* PawnBP = Cast<UBlueprint>(Asset);
	FPawnMeshRoles Roles;
	if (!Roles.Resolve(C, PawnBP)) { return false; }
	const ACharacter* Pawn = Roles.RuntimePawn;
	if (!C.Require(Pawn && Pawn->GetCapsuleComponent(), TEXT("Private pawn capsule template unavailable"))) { return false; }
	const double HalfHeight = Pawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FReplacements PawnMap;
	if (!PawnBodyAliases(C, Asset, Map, PawnMap)) { return false; }
	TArray<UObject*> Owned;
	GetObjectsWithOuter(Asset->GetOutermost(), Owned, EGetObjectsFlags::IncludeNestedObjects);
	int32 RuntimeMeshes = 0;
	for (UObject* Object : Owned)
	{
		USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(Object);
		if (!Component || Component->GetFName() != TEXT("CharacterMesh0")) { continue; }
		bool bRuntime = false;
		if (!Roles.Identify(C, Component, bRuntime)) { return false; }
		if (!bRuntime)
		{
			C.Add(TEXT("private_schema_mesh_no_runtime_setters"), Component->GetPathName());
			continue;
		}
		++RuntimeMeshes;
		Component->SetSkeletalMesh(Mesh, false);
		Component->SetPhysicsAsset(PA, false);
		Component->EmptyOverrideMaterials();
		const FTransform ShapeTransform(Component->GetRelativeRotation(), FVector::ZeroVector, Component->GetRelativeScale3D());
		double SoleZ = TNumericLimits<double>::Max();
		for (const FSkelMeshSection& Section : Mesh->GetImportedModel()->LODModels[0].Sections)
		{
			for (const FSoftSkinVertex& Vertex : Section.SoftVertices) { SoleZ = FMath::Min(SoleZ, ShapeTransform.TransformPosition(FVector(Vertex.Position)).Z); }
		}
		if (!C.Require(FMath::IsFinite(SoleZ), TEXT("Cannot measure Inspector sole vertices"))) { return false; }
		FVector Position = Component->GetRelativeLocation();
		Position.Z = -HalfHeight - SoleZ;
		Component->SetRelativeLocation(Position);
		C.Add(TEXT("private_sole_alignment_") + Asset->GetName(), FString::Printf(TEXT("capsuleHalfHeight=%.6f meshMinZ=%.6f meshRelativeZ=%.6f"), HalfHeight, SoleZ, Position.Z));
		FObjectPropertyBase* AnimClass = FindFProperty<FObjectPropertyBase>(Component->GetClass(), TEXT("AnimClass"));
		if (!C.Require(AnimClass != nullptr, TEXT("Native pawn AnimClass field unavailable"))) { return false; }
		AnimClass->SetObjectPropertyValue_InContainer(Component, Anim->GeneratedClass);
	}
	if (!C.Require(RuntimeMeshes == 1, TEXT("Expected exactly one current generated-CDO runtime mesh"))) { return false; }
	return !bRewriteGraph || RewritePackage(C, Asset, PawnMap, false);
}

bool Adapt(FContext& C)
{
	FReplacements Map;
	if (!Replacements(C, Map)) { return false; }
	TArray<UBlueprint*> Blueprints;
	for (const FRow& Row : C.Rows)
	{
		UObject* Asset = Map.FindChecked(LoadObject<UObject>(nullptr, *ObjectPath(Row.Source)));
		if (UBlueprint* BP = Cast<UBlueprint>(Asset)) { Blueprints.Add(BP); }
	}
	// A provider compile can also rebuild dependents. Redirect all six private
	// animation declarations and live node pins before the first such compile.
	// This deliberately does not rewrite Blueprint.ParentClass or class internals.
	int32 AnimationPrepass = 0;
	for (UBlueprint* BP : Blueprints)
	{
		if (!BP->IsA<UAnimBlueprint>()) { continue; }
		if (!BlueprintOwnership(C, BP) || !RewriteBlueprintGraphNodes(C, BP, Map, false)
			|| !RewriteBlueprintGraphNodes(C, BP, Map, true)) { return false; }
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP); // No structural/skeleton compile in this prepass.
		++AnimationPrepass;
	}
	if (!C.Require(AnimationPrepass == 6, TEXT("Expected exactly six private animation graph prepasses")) || !CleanSources(C)) { return false; }
	C.Add(TEXT("private_animation_graph_prepass"), TEXT("6 private Blueprints: independent native declarations, complete live pin types and node references redirected/verified before any provider compile; ParentClass untouched"));
	// This fixed group has provider dependencies beyond class ancestry:
	// FootPlant -> Main -> Item -> linked children. Other Blueprints retain
	// parent-depth order. A total key avoids pairwise ordering exceptions.
	Blueprints.Sort([](const UBlueprint& A, const UBlueprint& B)
	{
		auto Rank = [](const UBlueprint& Blueprint)
		{
			const FString Path = Blueprint.GetPathName();
			if (Path == PrivateFootPlant) { return 0; }
			if (Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.ABP_Mannequin_Base")) { return 1; }
			if (Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_ItemAnimLayersBase.ABP_ItemAnimLayersBase")) { return 2; }
			if (Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_RifleAnimLayers.ABP_RifleAnimLayers")
				|| Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_RifleAnimLayers_Feminine.ABP_RifleAnimLayers_Feminine")
				|| Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_UnarmedAnimLayers.ABP_UnarmedAnimLayers")
				|| Path == TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_UnarmedAnimLayers_Feminine.ABP_UnarmedAnimLayers_Feminine")) { return 3; }
			return 4;
		};
		const int32 RankA = Rank(A), RankB = Rank(B);
		if (RankA != RankB) { return RankA < RankB; }
		auto Depth = [](const UClass* Class) { int32 N = 0; for (; Class; Class = Class->GetSuperClass()) { ++N; } return N; };
		const int32 DepthA = Depth(A.ParentClass), DepthB = Depth(B.ParentClass);
		return DepthA != DepthB ? DepthA < DepthB : A.GetPathName() < B.GetPathName();
	});
	for (UBlueprint* BP : Blueprints)
	{
		if (!Replacements(C, Map)) { return false; }
		if (BP->GetPathName() == PrivateFootPlant)
		{
			// Finish the actual Inspector hierarchy and VM before the main
			// AnimBP consumes this provider's generated class.
			if (!RewritePackage(C, BP, Map, false) || !AdaptFootPlant(C) || !Replacements(C, Map)
				|| !RewritePackage(C, BP, Map, true) || !CleanSources(C)) { return false; }
			C.Add(TEXT("private_provider_compile_order"), BP->GetPathName());
			continue;
		}
		// Reparent can compile internally. Fix graph declarations and live
		// references first, without directly patching Blueprint.ParentClass.
		if (!RewriteBlueprintGraphNodes(C, BP, Map, false) || !Reparent(C, BP, Map) || !Replacements(C, Map)
			|| !RewritePackage(C, BP, Map, false)) { return false; }
		if (UAnimBlueprint* Animation = Cast<UAnimBlueprint>(BP))
		{
			Animation->TargetSkeleton = Private<USkeleton>(C, SkeletonPath);
			if (!BlueprintOwnership(C, BP)) { return false; }
			FBlueprintEditorUtils::RefreshAllNodes(BP);
			if (!Replacements(C, Map) || !RewritePackage(C, BP, Map, false) || !RewritePackage(C, BP, Map, true)) { return false; }
		}
		if (!Compile(C, BP) || !CleanSources(C)) { return false; }
		C.Add(TEXT("private_provider_compile_order"), BP->GetPathName());
	}
	if (!Replacements(C, Map)) { return false; }
	for (const FRow& Row : C.Rows)
	{
		UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath(Row.Target));
		if (!RewritePackage(C, Asset, Map, Asset->GetPathName() == PrivateFootPlant)) { return false; }
		if (Row.Target.EndsWith(TEXT("/BP_DC_RiflePawn")) || Row.Target.EndsWith(TEXT("/BP_DC_LyraInventoryPawn")))
		{
			if (!BindPawnMeshes(C, Asset, Map)) { return false; }
		}
	}
	for (UBlueprint* BP : Blueprints)
	{
		if (BP->GetPathName() == PrivateFootPlant)
		{
			// The provider was already finally adapted and compiled before Main.
			// Do not trigger a late provider compile while consumers are rebuilt.
			if (!FootPlantFootAnchor(C, false) || !CheckFootPlantTopology(C, true, false)) { return false; }
			continue;
		}
		if (!Replacements(C, Map) || !RewritePackage(C, BP, Map, false)) { return false; }
		if ((BP->GetName() == TEXT("BP_DC_RiflePawn") || BP->GetName() == TEXT("BP_DC_LyraInventoryPawn")) && !BindPawnMeshes(C, BP, Map)) { return false; }
		if (BP->IsA<UAnimBlueprint>())
		{
			if (!BlueprintOwnership(C, BP)) { return false; }
			FBlueprintEditorUtils::RefreshAllNodes(BP);
			if (!Replacements(C, Map) || !RewritePackage(C, BP, Map, false) || !RewritePackage(C, BP, Map, true)) { return false; }
		}
		if (!Compile(C, BP) || !CleanSources(C)) { return false; }
	}
	if (!Replacements(C, Map)) { return false; }
	for (UBlueprint* BP : Blueprints)
	{
		if (BP->GetName() == TEXT("BP_DC_RiflePawn") || BP->GetName() == TEXT("BP_DC_LyraInventoryPawn"))
		{
			if (!BindPawnMeshes(C, BP, Map, false)) { return false; }
		}
		if (!RewritePackage(C, BP, Map, true)) { return false; }
	}
	C.Add(TEXT("private_reference_rewrite"), TEXT("304 approved packages only; FootPlant/Main/Item/linked-child provider order, other Blueprint parent-first engine reparent/compile; native user-declared and complete live pin types rewritten/verified; inherited native SCS overrides restored; source controllers and parent internals untouched"));
	return CleanSources(C);
}

bool StandingPose(FContext& C)
{
	USkeletalMesh* SourceMesh = LoadObject<USkeletalMesh>(nullptr, MannyMesh);
	USkeletalMesh* TargetMesh = Private<USkeletalMesh>(C, MeshPath);
	UIKRetargeter* Retargeter = Private<UIKRetargeter>(C, RetargeterPath);
	if (!SourceMesh || !TargetMesh || !Retargeter) { return false; }
	FIKRetargetProcessor Processor;
	FRetargetInitParameters Init;
	Init.SourceSkeletalMesh = SourceMesh;
	Init.TargetSkeletalMesh = TargetMesh;
	Init.RetargeterAsset = Retargeter;
	Init.CustomProfile = Retargeter->GetCurrentProfile();
	Init.bSuppressWarnings = false;
	Processor.Initialize(Init);
	if (!C.Require(Processor.IsInitialized() && Processor.Log.GetErrors().IsEmpty(), TEXT("Permanent retarget processor did not initialize"))) { return false; }
	const FRetargetSkeleton& Source = Processor.GetSkeleton(ERetargetSourceOrTarget::Source);
	const FRetargetSkeleton& Target = Processor.GetSkeleton(ERetargetSourceOrTarget::Target);
	const TArray<FTransform> Global = GlobalPose(SourceMesh->GetRefSkeleton());
	TArray<FTransform> Input;
	for (const FName Name : Source.BoneNames)
	{
		const int32 I = SourceMesh->GetRefSkeleton().FindBoneIndex(Name);
		if (!C.Require(I != INDEX_NONE, TEXT("Missing source reference transform"))) { return false; }
		Input.Add(Global[I]);
	}
	Processor.ApplySourceScaleToPose(Input);
	TArray<FTransform> Unit;
	Unit.SetNum(Input.Num());
	for (int32 I = 0; I < Input.Num(); ++I)
	{
		const int32 Parent = Source.ParentIndices[I];
		FTransform Local = Source.GetLocalTransformOfSingleBone(I, Input);
		if (Parent != INDEX_NONE) { Local.ScaleTranslation(Input[Parent].GetScale3D()); }
		Local.SetScale3D(FVector::OneVector);
		Unit[I] = Parent == INDEX_NONE ? Local : Local * Unit[Parent];
		if (!C.Require(Unit[I].GetTranslation().Equals(Input[I].GetTranslation(), 1.e-4)
			&& Unit[I].GetRotation().AngularDistance(Input[I].GetRotation()) <= 1.e-5, TEXT("Unit-scale bake changed standing bone origins/orientations"))) { return false; }
	}
	FRetargetRunParameters Parameters;
	Parameters.SourceGlobalPose = &Unit;
	Parameters.Profile = Retargeter->GetCurrentProfile();
	Parameters.OverrideSetsToApply = Retargeter->GetOverrideSetsToApply();
	Parameters.DeltaTime = 0;
	Parameters.LOD = 0;
	const TArray<FTransform> Converted = Processor.RunRetargeter(Parameters);
	if (!C.Require(Converted.Num() == 72 && Processor.Log.GetErrors().IsEmpty(), TEXT("Permanent standing retarget failed"))) { return false; }
	// Run exactly the same input again: self-pins must not accumulate scale.
	const TArray<FTransform> Repeated = Processor.RunRetargeter(Parameters);
	if (!C.Require(Repeated.Num() == Converted.Num() && Processor.Log.GetErrors().IsEmpty(), TEXT("Repeated permanent standing retarget failed"))) { return false; }
	const TArray<FTransform> Bind = GlobalPose(TargetMesh->GetRefSkeleton());
	const TArray<FTransform>& RawLocal = TargetMesh->GetRefSkeleton().GetRefBonePose();
	const TArray<FTransform>& ResolvedLocal = Target.RetargetPoses.GetLocalRetargetPose();
	const TArray<FTransform>& ResolvedGlobal = Target.RetargetPoses.GetGlobalRetargetPose();
	if (!C.Require(ResolvedLocal.Num() == Converted.Num() && ResolvedGlobal.Num() == Converted.Num(), TEXT("Resolved target reference pose count differs"))) { return false; }
	auto RecordTransform = [&](const FString& Space, const FName Bone, const FTransform& Transform)
	{
		const FVector Position = Transform.GetTranslation();
		const FVector Scale = Transform.GetScale3D();
		C.Add(TEXT("standing_") + Space + TEXT("_") + Bone.ToString(), FString::Printf(TEXT("T=%.9f;%.9f;%.9f S=%.9f;%.9f;%.9f"),
			Position.X, Position.Y, Position.Z, Scale.X, Scale.Y, Scale.Z));
	};
	for (const FName Bone : { FName(TEXT("root")), FName(TEXT("pelvis")), FName(TEXT("spine_01")), FName(TEXT("spine_03")),
		FName(TEXT("thigh_l")), FName(TEXT("calf_l")), FName(TEXT("foot_l")), FName(TEXT("foot_r")), FName(TEXT("head")) })
	{
		const int32 Out = Target.FindBoneIndexByName(Bone);
		const int32 Original = TargetMesh->GetRefSkeleton().FindBoneIndex(Bone);
		if (!C.Require(Out != INDEX_NONE && Original != INDEX_NONE, TEXT("Standing diagnostic anchor is missing"))) { return false; }
		RecordTransform(TEXT("raw_bind_local"), Bone, RawLocal[Original]);
		RecordTransform(TEXT("raw_bind_global"), Bone, Bind[Original]);
		RecordTransform(TEXT("resolved_target_local"), Bone, ResolvedLocal[Out]);
		RecordTransform(TEXT("resolved_target_global"), Bone, ResolvedGlobal[Out]);
		RecordTransform(TEXT("converted_first_global"), Bone, Converted[Out]);
		RecordTransform(TEXT("converted_second_global"), Bone, Repeated[Out]);
	}
	for (const TPair<FName, FName>& Pair : TArray<TPair<FName, FName>>{
		{ TEXT("pelvis"), TEXT("spine_01") }, { TEXT("spine_01"), TEXT("spine_02") },
		{ TEXT("thigh_l"), TEXT("calf_l") }, { TEXT("calf_l"), TEXT("foot_l") },
		{ TEXT("upperarm_r"), TEXT("lowerarm_r") }, { TEXT("lowerarm_r"), TEXT("hand_r") } })
	{
		const int32 A = Target.FindBoneIndexByName(Pair.Key);
		const int32 B = Target.FindBoneIndexByName(Pair.Value);
		const int32 RawA = TargetMesh->GetRefSkeleton().FindBoneIndex(Pair.Key);
		const int32 RawB = TargetMesh->GetRefSkeleton().FindBoneIndex(Pair.Value);
		if (!C.Require(A != INDEX_NONE && B != INDEX_NONE && RawA != INDEX_NONE && RawB != INDEX_NONE, TEXT("Standing length anchor is missing"))) { return false; }
		C.Add(TEXT("standing_edge_") + Pair.Key.ToString() + TEXT("_") + Pair.Value.ToString(),
			FString::Printf(TEXT("bind=%.9f resolved=%.9f first=%.9f second=%.9f"),
				FVector::Distance(Bind[RawA].GetTranslation(), Bind[RawB].GetTranslation()),
				FVector::Distance(ResolvedGlobal[A].GetTranslation(), ResolvedGlobal[B].GetTranslation()),
				FVector::Distance(Converted[A].GetTranslation(), Converted[B].GetTranslation()),
				FVector::Distance(Repeated[A].GetTranslation(), Repeated[B].GetTranslation())));
	}
	bool bHeightsValid = true;
	for (const FName Bone : { FName(TEXT("foot_l")), FName(TEXT("foot_r")), FName(TEXT("head")), FName(TEXT("pelvis")) })
	{
		const int32 Out = Target.FindBoneIndexByName(Bone);
		const int32 Original = TargetMesh->GetRefSkeleton().FindBoneIndex(Bone);
		if (!C.Require(Out != INDEX_NONE && Original != INDEX_NONE, TEXT("Standing quality anchor is missing"))) { return false; }
		const double Delta = FMath::Abs(Converted[Out].GetTranslation().Z - Bind[Original].GetTranslation().Z);
		C.Add(TEXT("standing_height_delta_") + Bone.ToString(), FString::Printf(TEXT("%.9f"), Delta));
		const double Tolerance = Bone == TEXT("head") ? 12.0 : Bone == TEXT("pelvis") ? 8.0 : 5.0;
		bHeightsValid = C.Require(Delta <= Tolerance, TEXT("Reference pose remains miscalibrated: ") + Bone.ToString() + TEXT("; do not save or activate")) && bHeightsValid;
	}
	double MaxBindScaleError = 0;
	double MaxOriginalBindScaleError = 0;
	int32 OriginalScaleCount = 0;
	double MaxRepeatPositionError = 0;
	double MaxRepeatScaleError = 0;
	double MaxRepeatRotationError = 0;
	bool bTransformsValid = true;
	bool bOriginalScalesValid = true;
	// Original 61-bone scale is a skinning/physics-volume preservation contract.
	// The eight unweighted helpers and three virtual bones are computation frames;
	// their bind-scale equality has no established contract, so record it separately.
	for (int32 I = 0; I < Converted.Num(); ++I)
	{
		const int32 Raw = TargetMesh->GetRefSkeleton().FindBoneIndex(Target.BoneNames[I]);
		if (!C.Require(Bind.IsValidIndex(Raw), TEXT("Standing scale probe cannot map a target bone"))) { return false; }
		const FVector FirstScale = Converted[I].GetScale3D();
		const FVector SecondScale = Repeated[I].GetScale3D();
		const FVector BindScale = Bind[Raw].GetScale3D();
		const double FirstError = (FirstScale - BindScale).GetAbsMax();
		const double SecondError = (SecondScale - BindScale).GetAbsMax();
		const double BindError = FMath::Max(FirstError, SecondError);
		const TCHAR* Group = Raw < 61 ? TEXT("original61") : Raw < 69 ? TEXT("unweighted_helper") : TEXT("virtual");
		const FString ScaleEvidence = FString::Printf(TEXT("group=%s first=%.9f;%.9f;%.9f second=%.9f;%.9f;%.9f bind=%.9f;%.9f;%.9f first_error=%.9f second_error=%.9f"),
			Group, FirstScale.X, FirstScale.Y, FirstScale.Z, SecondScale.X, SecondScale.Y, SecondScale.Z,
			BindScale.X, BindScale.Y, BindScale.Z, FirstError, SecondError);
		C.Add(TEXT("standing_bone_global_scale_") + Target.BoneNames[I].ToString(), ScaleEvidence);
		bTransformsValid = C.Require(!Converted[I].ContainsNaN() && Converted[I].GetRotation().IsNormalized()
			&& !Repeated[I].ContainsNaN() && Repeated[I].GetRotation().IsNormalized()
			&& FirstScale.GetMin() > SMALL_NUMBER && SecondScale.GetMin() > SMALL_NUMBER,
			TEXT("Invalid permanent standing transform: ") + Target.BoneNames[I].ToString() + TEXT(" ") + ScaleEvidence) && bTransformsValid;
		MaxBindScaleError = FMath::Max(MaxBindScaleError, BindError);
		if (Raw < 61)
		{
			++OriginalScaleCount;
			MaxOriginalBindScaleError = FMath::Max(MaxOriginalBindScaleError, BindError);
			bOriginalScalesValid = C.Require(BindError <= 1.e-3, TEXT("Original bind scale mismatch: ") + Target.BoneNames[I].ToString()
				+ TEXT(" ") + ScaleEvidence) && bOriginalScalesValid;
		}
		MaxRepeatPositionError = FMath::Max(MaxRepeatPositionError, FVector::Distance(Converted[I].GetTranslation(), Repeated[I].GetTranslation()));
		MaxRepeatScaleError = FMath::Max(MaxRepeatScaleError, (Converted[I].GetScale3D() - Repeated[I].GetScale3D()).GetAbsMax());
		MaxRepeatRotationError = FMath::Max(MaxRepeatRotationError, Converted[I].GetRotation().AngularDistance(Repeated[I].GetRotation()));
	}
	C.Add(TEXT("standing_max_bind_global_scale_error"), FString::Printf(TEXT("%.9f"), MaxBindScaleError));
	C.Add(TEXT("standing_max_original61_bind_global_scale_error"), FString::Printf(TEXT("%.9f"), MaxOriginalBindScaleError));
	C.Add(TEXT("standing_original_scale_count"), FString::FromInt(OriginalScaleCount));
	C.Add(TEXT("standing_bind_scale_contract"), TEXT("Original61 skinning/physics-volume scale must match bind within 1e-3; all72 positive/finite/repeat checks remain mandatory; helper/virtual bind-scale differences are diagnostic"));
	C.Add(TEXT("standing_weapon_attachment_validation"), TEXT("weapon_r actual scale is recorded; weapon attachment size and hand contact require user play/visual confirmation"));
	C.Add(TEXT("standing_repeat_max_position_error_cm"), FString::Printf(TEXT("%.9f"), MaxRepeatPositionError));
	C.Add(TEXT("standing_repeat_max_scale_error"), FString::Printf(TEXT("%.9f"), MaxRepeatScaleError));
	C.Add(TEXT("standing_repeat_max_rotation_error_radians"), FString::Printf(TEXT("%.9f"), MaxRepeatRotationError));
	const bool bScaleValid = C.Require(OriginalScaleCount == 61 && MaxOriginalBindScaleError <= 1.e-3,
		TEXT("Permanent standing output does not preserve all original 61 global bind scales")) && bOriginalScalesValid;
	const bool bRepeatValid = C.Require(MaxRepeatPositionError <= 1.e-3 && MaxRepeatScaleError <= 1.e-3 && MaxRepeatRotationError <= 1.e-3,
		TEXT("Permanent standing self-pins accumulated scale or changed the repeated pose"));
	bool bHelpersValid = true;
	for (const TPair<FName, FName>& Pair : TArray<TPair<FName, FName>>{
		{ TEXT("foot_l"), TEXT("ik_foot_l") }, { TEXT("foot_r"), TEXT("ik_foot_r") },
		{ TEXT("hand_l"), TEXT("ik_hand_l") }, { TEXT("hand_r"), TEXT("ik_hand_r") },
		{ TEXT("weapon_r"), TEXT("ik_hand_gun") } })
	{
		const int32 A = Target.FindBoneIndexByName(Pair.Key);
		const int32 B = Target.FindBoneIndexByName(Pair.Value);
		const FString Label = Pair.Key.ToString() + TEXT("_") + Pair.Value.ToString();
		if (A == INDEX_NONE || B == INDEX_NONE)
		{
			C.Add(TEXT("standing_helper_endpoint_") + Label, TEXT("missing_endpoint"));
			bHelpersValid = C.Require(false, TEXT("Standing helper endpoint missing: ") + Label) && bHelpersValid;
			continue;
		}
		const FVector FirstSource = Converted[A].GetTranslation();
		const FVector FirstHelper = Converted[B].GetTranslation();
		const FVector SecondSource = Repeated[A].GetTranslation();
		const FVector SecondHelper = Repeated[B].GetTranslation();
		C.Add(TEXT("standing_helper_endpoint_") + Label, FString::Printf(TEXT("first_source=%.9f;%.9f;%.9f first_helper=%.9f;%.9f;%.9f second_source=%.9f;%.9f;%.9f second_helper=%.9f;%.9f;%.9f first_distance=%.9f second_distance=%.9f"),
			FirstSource.X, FirstSource.Y, FirstSource.Z, FirstHelper.X, FirstHelper.Y, FirstHelper.Z,
			SecondSource.X, SecondSource.Y, SecondSource.Z, SecondHelper.X, SecondHelper.Y, SecondHelper.Z,
			FVector::Distance(FirstSource, FirstHelper), FVector::Distance(SecondSource, SecondHelper)));
		bHelpersValid = C.Require(FirstSource.Equals(FirstHelper, 1.e-3) && SecondSource.Equals(SecondHelper, 1.e-3),
			TEXT("Helper bone did not follow its retargeted endpoint: ") + Label) && bHelpersValid;
	}
	if (!bHeightsValid || !bTransformsValid || !bScaleValid || !bRepeatValid || !bHelpersValid) { return false; }
	C.Add(TEXT("standing_pose_calibration_status"), TEXT("actual permanent outputs passed height bounds, original61 global bind scales, all72 finite/positive scales, identical-input repeat and endpoint helper pins; visual naturalness/contact remains user validation"));
	return CleanSources(C);
}

bool CompiledReferences(FContext& C, const UBlueprint* Blueprint, const FReplacements& Map)
{
	if (!C.Require(Blueprint && Blueprint->GeneratedClass, TEXT("Compiled reference probe requires a generated class"))) { return false; }
	TArray<UObject*> References;
	FReferenceFinder Finder(References, nullptr, false, true, false, false);
	Finder.FindReferences(Blueprint->GeneratedClass);
	for (const UObject* Reference : References)
	{
		if (!Reference) { continue; }
		if (!C.Require(!Reference->GetName().StartsWith(TEXT("REINST_")) && !Reference->GetName().StartsWith(TEXT("TRASHCLASS_")),
			TEXT("Private compiled class retains a reinstancing object: ") + Reference->GetPathName())) { return false; }
		if (!C.Require(!Map.Contains(const_cast<UObject*>(Reference)), TEXT("Original compiled reference remains: ") + Reference->GetPathName())) { return false; }
		if (const UFunction* Function = Cast<UFunction>(Reference))
		{
			if (!C.Require(!Map.Contains(Function->GetOuter()), TEXT("Compiled function still belongs to a copied source parent"))) { return false; }
		}
	}
	return true;
}

bool Verify(FContext& C)
{
	USkeletalMesh* Original = LoadObject<USkeletalMesh>(nullptr, OriginalMesh);
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	USkeleton* Skeleton = Private<USkeleton>(C, SkeletonPath);
	UPhysicsAsset* PA = Private<UPhysicsAsset>(C, PhysicsPath);
	if (!Mesh || !Skeleton || !PA || !OriginalMeshEqual(C, Original, Mesh)
		|| !C.Require(Mesh->GetSkeleton() == Skeleton && Mesh->GetPhysicsAsset() == PA
			&& Mesh->GetRefSkeleton().GetRawBoneNum() == 69 && Mesh->GetRefSkeleton().GetNum() == 72
			&& Skeleton->GetVirtualBones().Num() == 3, TEXT("Private mesh/skeleton/physics binding differs"))) { return false; }
	for (const FSkeletalMeshLODModel& LOD : Mesh->GetImportedModel()->LODModels)
	{
		for (int32 I = 61; I < 69; ++I)
		{
			if (!C.Require(LOD.RequiredBones.Contains(static_cast<FBoneIndexType>(I)), TEXT("Unweighted helper is missing from LOD required bones"))) { return false; }
		}
	}
	for (const FName Name : { FName(TEXT("FastFeet")), FName(TEXT("LeftFingersMask")), FName(TEXT("UpperBodyMask")), FName(TEXT("UpperBodyLowerBodySplitMask")) })
	{
		UBlendProfile* Profile = Skeleton->GetBlendProfile(Name);
		if (!C.Require(Profile && Profile->GetSkeleton() == Skeleton && C.Allowed(Profile), TEXT("Private profile ownership differs"))) { return false; }
		for (int32 I = 0; I < Profile->GetNumBlendEntries(); ++I)
		{
			if (!C.Require(Skeleton->GetReferenceSkeleton().FindBoneIndex(Profile->GetEntry(I).BoneReference.BoneName) != INDEX_NONE, TEXT("Invalid private profile bone"))) { return false; }
		}
	}
	if (!C.Require(PA->SkeletalBodySetups.Num() == 14 && PA->ConstraintSetup.Num() == Original->GetPhysicsAsset()->ConstraintSetup.Num() + 1, TEXT("Unexpected private physics counts"))) { return false; }
	for (int32 I = 0; I < 13; ++I)
	{
		USkeletalBodySetup* SourceBody = Original->GetPhysicsAsset()->SkeletalBodySetups[I];
		USkeletalBodySetup* Body = PA->SkeletalBodySetups[I];
		if (!C.Require(SourceBody->BoneName == Body->BoneName, TEXT("Existing physics body order/name changed"))) { return false; }
		if (Body->BoneName != TEXT("neck_01"))
		{
			FProperty* Geometry = FindFProperty<FProperty>(Body->GetClass(), TEXT("AggGeom"));
			FString A, B;
			Geometry->ExportText_InContainer(0, A, SourceBody, nullptr, SourceBody, PPF_None);
			Geometry->ExportText_InContainer(0, B, Body, nullptr, Body, PPF_None);
			if (!C.Require(A == B, TEXT("Non-neck existing physics shape changed"))) { return false; }
		}
		if (!C.Require(Body->PhysMaterial && Body->PhysMaterial->GetPathName() == TEXT("/Game/Characters/Heroes/PhysMat_Player.PhysMat_Player")
			&& Body->DefaultInstance.GetPhysMaterialOverride() == Body->PhysMaterial, TEXT("Existing body must keep normal hit material"))) { return false; }
	}
	const USkeletalBodySetup* Head = PA->SkeletalBodySetups[PA->FindBodyIndex(TEXT("head"))];
	if (!C.Require(Head->PhysMaterial && Head->PhysMaterial->GetPathName() == TEXT("/Game/Characters/Heroes/PhysMat_Player_WeakSpot.PhysMat_Player_WeakSpot")
		&& Head->DefaultInstance.GetPhysMaterialOverride() == Head->PhysMaterial, TEXT("Only added head uses weak material"))) { return false; }
	const USkeletalBodySetup* Neck = PA->SkeletalBodySetups[PA->FindBodyIndex(TEXT("neck_01"))];
	if (!C.Require(Head->AggGeom.SphereElems.Num() == 1 && Neck->AggGeom.SphylElems.Num() == 1, TEXT("Head/neck shape layout differs"))) { return false; }
	const TArray<FTransform> BindPhysics = GlobalPose(Mesh->GetRefSkeleton());
	const FTransform& HeadTM = BindPhysics[Mesh->GetRefSkeleton().FindBoneIndex(TEXT("head"))];
	const FTransform& NeckTM = BindPhysics[Mesh->GetRefSkeleton().FindBoneIndex(TEXT("neck_01"))];
	const FKSphereElem& Sphere = Head->AggGeom.SphereElems[0];
	const double SphereRadius = Sphere.GetFinalScaled(HeadTM.GetScale3D(), FTransform::Identity).Radius;
	const double NeckDistance = Neck->AggGeom.SphylElems[0].GetShortestDistanceToPoint(HeadTM.TransformPosition(Sphere.Center), NeckTM);
	if (!C.Require(NeckDistance >= SphereRadius + 0.25, TEXT("Reloaded scaled head/neck query volumes overlap"))) { return false; }
	FReplacements Map;
	if (!Replacements(C, Map)) { return false; }
	int32 AnimBlueprints = 0;
	for (const FRow& Row : C.Rows)
	{
		UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath(Row.Target));
		if (!C.Require(C.Allowed(Asset) && Asset->GetClass()->GetPathName() == Row.Class, TEXT("Private asset class/path differs"))
			|| !RewritePackage(C, Asset, Map, true)) { return false; }
		if (const UBlueprint* BP = Cast<UBlueprint>(Asset))
		{
			if (!C.Require(BP->Status != BS_Error && BP->GeneratedClass, TEXT("Private compiled Blueprint unavailable"))) { return false; }
			if (!CompiledReferences(C, BP, Map)) { return false; }
		}
		if (const UAnimBlueprint* BP = Cast<UAnimBlueprint>(Asset))
		{
			++AnimBlueprints;
			if (!C.Require(BP->TargetSkeleton == Skeleton, TEXT("Character AnimBP targets the wrong skeleton"))) { return false; }
		}
		if (Row.Target.EndsWith(TEXT("/BP_DC_RiflePawn")) || Row.Target.EndsWith(TEXT("/BP_DC_LyraInventoryPawn")))
		{
			const UBlueprint* OriginalBP = LoadObject<UBlueprint>(nullptr, *ObjectPath(Row.Source));
			const ACharacter* OriginalPawn = OriginalBP && OriginalBP->GeneratedClass ? Cast<ACharacter>(OriginalBP->GeneratedClass->GetDefaultObject(false)) : nullptr;
			UBlueprint* NewBP = CastChecked<UBlueprint>(Asset);
			FPawnMeshRoles Roles;
			if (!Roles.Resolve(C, NewBP)) { return false; }
			const ACharacter* NewPawn = Roles.RuntimePawn;
			if (!C.Require(OriginalPawn && NewPawn && OriginalPawn->GetMesh() && NewPawn->GetCapsuleComponent(), TEXT("Cannot compare original/private pawn component geometry"))
				|| !C.Require(OriginalPawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == NewPawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()
					&& OriginalPawn->GetCapsuleComponent()->GetUnscaledCapsuleRadius() == NewPawn->GetCapsuleComponent()->GetUnscaledCapsuleRadius()
					&& OriginalPawn->GetActorScale3D() == NewPawn->GetActorScale3D(), TEXT("Pawn capsule or Actor scale changed"))) { return false; }
			FReplacements PawnMap;
			if (!PawnBodyAliases(C, Asset, Map, PawnMap)) { return false; }
			if (!RewritePackage(C, Asset, PawnMap, true) || !CompiledReferences(C, NewBP, PawnMap)) { return false; }
			TArray<UObject*> Owned;
			GetObjectsWithOuter(Asset->GetOutermost(), Owned, EGetObjectsFlags::IncludeNestedObjects);
			int32 RuntimeMeshes = 0;
			for (const UObject* Object : Owned)
			{
				const USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(Object);
				if (!Component || Component->GetFName() != TEXT("CharacterMesh0")) { continue; }
				bool bRuntime = false;
				if (!Roles.Identify(C, Component, bRuntime)) { return false; }
				if (!bRuntime)
				{
					C.Add(TEXT("private_schema_mesh_no_runtime_geometry"), Component->GetPathName());
					continue;
				}
				++RuntimeMeshes;
				if (!C.Require(Component->GetSkeletalMeshAsset() == Mesh && Component->GetPhysicsAsset() == PA, TEXT("New pawn template still overrides Inspector with HARU"))) { return false; }
				const UBlueprint* Animation = Private<UBlueprint>(C, TEXT("/Game/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.ABP_Mannequin_Base"));
				FObjectPropertyBase* AnimClass = FindFProperty<FObjectPropertyBase>(Component->GetClass(), TEXT("AnimClass"));
				if (!C.Require(Animation != nullptr && AnimClass != nullptr && AnimClass->GetObjectPropertyValue_InContainer(Component) == Animation->GeneratedClass,
					TEXT("Runtime pawn mesh must use the current private Main AnimBP"))) { return false; }
				const USkeletalMeshComponent* OriginalComponent = OriginalPawn->GetMesh();
				const FVector PositionDelta = Component->GetRelativeLocation() - OriginalComponent->GetRelativeLocation();
				const FRotator RotationDelta = Component->GetRelativeRotation() - OriginalComponent->GetRelativeRotation();
				const FVector ScaleDelta = Component->GetRelativeScale3D() - OriginalComponent->GetRelativeScale3D();
				C.Add(TEXT("private_runtime_mesh_transform"), Component->GetPathName() + TEXT(" source=") + OriginalComponent->GetPathName()
					+ FString::Printf(TEXT(" xyDelta=(%.17g,%.17g) rawRotationDelta=(%.17g,%.17g,%.17g) scaleDelta=(%.17g,%.17g,%.17g) actualZ=%.17g"),
						PositionDelta.X, PositionDelta.Y, RotationDelta.Pitch, RotationDelta.Yaw, RotationDelta.Roll,
						ScaleDelta.X, ScaleDelta.Y, ScaleDelta.Z, Component->GetRelativeLocation().Z));
				if (!C.Require(Component->GetRelativeLocation().X == OriginalComponent->GetRelativeLocation().X
					&& Component->GetRelativeLocation().Y == OriginalComponent->GetRelativeLocation().Y
					&& Component->GetRelativeRotation().Equals(OriginalComponent->GetRelativeRotation(), 0.0f)
					&& Component->GetRelativeScale3D() == OriginalComponent->GetRelativeScale3D(), TEXT("Private mesh XY/rotation/scale changed"))) { return false; }
				const FTransform Shape(Component->GetRelativeRotation(), FVector::ZeroVector, Component->GetRelativeScale3D());
				double Sole = TNumericLimits<double>::Max();
				for (const FSkelMeshSection& Section : Mesh->GetImportedModel()->LODModels[0].Sections)
				{
					for (const FSoftSkinVertex& Vertex : Section.SoftVertices) { Sole = FMath::Min(Sole, Shape.TransformPosition(FVector(Vertex.Position)).Z); }
				}
				if (!C.Require(FMath::Abs(Component->GetRelativeLocation().Z + Sole + NewPawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) <= 1.e-4, TEXT("Private vertex sole does not align to capsule bottom"))) { return false; }
				const TArray<FTransform> Ref = GlobalPose(Mesh->GetRefSkeleton());
				FVector Toes = FVector::ZeroVector;
				for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
				{
					const int32 Foot = Mesh->GetRefSkeleton().FindBoneIndex(FName(*(FString(TEXT("foot_")) + Side)));
					const int32 Ball = Mesh->GetRefSkeleton().FindBoneIndex(FName(*(FString(TEXT("ball_")) + Side)));
					Toes += Shape.TransformVectorNoScale(Ref[Ball].GetTranslation() - Ref[Foot].GetTranslation());
				}
				Toes.Z = 0;
				if (!C.Require(FVector::DotProduct(Toes.GetSafeNormal(), FVector::ForwardVector) >= 0.75, TEXT("Inspector forward axis is inconsistent with original pawn yaw"))) { return false; }
			}
			if (!C.Require(RuntimeMeshes == 1, TEXT("Expected exactly one verified current generated-CDO runtime mesh"))) { return false; }
		}
	}
	C.Add(TEXT("private_anim_blueprints"), FString::FromInt(AnimBlueprints));
	return C.Require(AnimBlueprints == 6, TEXT("Expected exactly six private character AnimBPs")) && FootPlantFootAnchor(C, false)
		&& CheckFootPlantTopology(C, true, false) && StandingPose(C) && CleanSources(C);
}

bool Prepare(FContext& C)
{
	USkeletalMesh* Original = LoadObject<USkeletalMesh>(nullptr, OriginalMesh);
	USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, MannyMesh);
	USkeletalMesh* Mesh = Private<USkeletalMesh>(C, MeshPath);
	USkeleton* Skeleton = Private<USkeleton>(C, SkeletonPath);
	UPhysicsAsset* PA = Private<UPhysicsAsset>(C, PhysicsPath);
	if (!Original || !Manny || !Mesh || !Skeleton || !PA || !CleanSources(C)) { return false; }
	USkinnedAsset* Assets[] = { Original, Manny, Mesh };
	FSkinnedAssetCompilingManager::Get().FinishCompilation(MakeArrayView(Assets));
	Mesh->SetSkeleton(Skeleton);
	Mesh->SetPhysicsAsset(PA);
	return PrepareSkeleton(C, Mesh, Skeleton, Original, Manny) && PreparePhysics(C, Mesh, PA, Original)
		&& PrepareRigs(C, Manny, Mesh) && StandingPose(C) && CleanSources(C);
}

void Run(const TCHAR* Phase)
{
	struct FErrors : FOutputDevice
	{
		TArray<FString> Errors;
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if ((Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::Error) { Errors.Add(Category.ToString() + TEXT(": ") + V); }
		}
	};
	FContext C;
	C.Phase = Phase;
	bool bSuccess = Guard(C);
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Diagnostics/InspectorBot20261023/Integration") / C.Run;
	const FString Receipt = Directory / (TEXT("native_") + C.Phase + TEXT(".csv"));
	if (bSuccess) { bSuccess = C.Require(!FPaths::FileExists(Receipt), TEXT("Use a fresh native phase/run; prior evidence is preserved")); }
	if (bSuccess)
	{
		FErrors Errors;
		GLog->AddOutputDevice(&Errors);
		if (C.Phase == TEXT("Prepare")) { bSuccess = Prepare(C); }
		else if (C.Phase == TEXT("RepairFootPlantCopy")) { bSuccess = RepairFootPlantCopy(C); }
		else if (C.Phase == TEXT("Adapt")) { bSuccess = Adapt(C); }
		else if (C.Phase == TEXT("Verify")) { bSuccess = Verify(C); }
		else { bSuccess = C.Require(false, TEXT("Unknown fixed phase")); }
		GLog->RemoveOutputDevice(&Errors);
		if (!Errors.Errors.IsEmpty()) { bSuccess = C.Require(false, TEXT("Editor operation emitted errors: ") + FString::Join(Errors.Errors, TEXT(" | "))); }
	}
	if (!C.Packages.IsEmpty() && !CleanSources(C)) { bSuccess = false; }
	C.Add(TEXT("phase"), C.Phase);
	C.Add(TEXT("success"), bSuccess ? TEXT("true") : TEXT("false"));
	C.Add(TEXT("asset_saves"), TEXT("0"));
	C.Add(TEXT("PIE"), TEXT("false"));
	C.Add(TEXT("error"), C.Error);
	bool bSafeRun = C.Run.Len() > 0 && C.Run.Len() <= 40;
	for (const TCHAR Ch : C.Run) { bSafeRun &= Ch < 128 && (FChar::IsAlnum(Ch) || Ch == TEXT('_')); }
	if (bSafeRun && !FPaths::FileExists(Receipt))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
		if (!FFileHelper::SaveStringArrayToFile(C.Evidence, *Receipt, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) { bSuccess = false; }
	}
	if (bSuccess) { UE_LOG(LogTemp, Display, TEXT("INSPECTOR_BOT_PHASE_OK %s saves=0 PIE=false"), Phase); }
	else { UE_LOG(LogTemp, Error, TEXT("INSPECTOR_BOT_PHASE_FAILED %s: %s. Discard without saving."), Phase, *C.Error); }
}

FAutoConsoleCommand PrepareCommand(TEXT("DC.InspectorBot.Prepare"), TEXT("Approved Inspector private assets only; unattended dedicated flag; never saves."), FConsoleCommandDelegate::CreateLambda([] { Run(TEXT("Prepare")); }));
FAutoConsoleCommand RepairFootPlantCommand(TEXT("DC.InspectorBot.RepairFootPlantCopy"), TEXT("Restore exactly three verified FootPlant copy links in the unsaved private rig only; never saves."), FConsoleCommandDelegate::CreateLambda([] { Run(TEXT("RepairFootPlantCopy")); }));
FAutoConsoleCommand AdaptCommand(TEXT("DC.InspectorBot.Adapt"), TEXT("Approved Inspector private selectors/graphs only; never saves."), FConsoleCommandDelegate::CreateLambda([] { Run(TEXT("Adapt")); }));
FAutoConsoleCommand VerifyCommand(TEXT("DC.InspectorBot.Verify"), TEXT("Verify the fixed Inspector assets and original preservation; never saves."), FConsoleCommandDelegate::CreateLambda([] { Run(TEXT("Verify")); }));

}

#endif // WITH_EDITOR
