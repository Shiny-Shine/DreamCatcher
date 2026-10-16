// One-off editor support for restoring Lyra's rifle rig on the HARU mesh.
// Assets are changed only in memory; the guarded editor script validates and saves them.
#if WITH_EDITOR

#include "Animation/Skeleton.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "Equipment/Lyra/DCLyraEquipmentDefinition.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectGlobals.h"

namespace DCHaruRifleRigRepair
{
void RestoreVirtualBonesAndPreview()
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCHaruRigRepair")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR requires the unattended -DCHaruRigRepair editor script."));
		return;
	}

	USkeletalMesh* Haru = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU.SKM_HARU"));
	USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny.SKM_Manny"));
	USkeletalMesh* Rifle = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Weapons/Rifle/Mesh/SK_Rifle.SK_Rifle"));
	UClass* EquipmentClass = LoadClass<UDCLyraEquipmentDefinition>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/WID_Rifle.WID_Rifle_C"));
	const UDCLyraEquipmentDefinition* Equipment = EquipmentClass ? GetDefault<UDCLyraEquipmentDefinition>(EquipmentClass) : nullptr;
	USkeleton* Target = Haru ? Haru->GetSkeleton() : nullptr;
	const USkeleton* Source = Manny ? Manny->GetSkeleton() : nullptr;
	if (!Target || !Source || !Rifle || Target == Source || !Equipment || Equipment->ActorsToSpawn.Num() != 1 ||
		Target->GetPathName() != TEXT("/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton.하루리깅_Skeleton") ||
		Haru->GetRefSkeleton().FindBoneIndex(TEXT("weapon_r")) == INDEX_NONE ||
		Target->FindSocket(TEXT("weapon_r")) || Target->FindSocket(TEXT("weapon_r_preview")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR asset/bone/socket preflight failed; nothing changed."));
		return;
	}
	const FDCLyraEquipmentActorToSpawn& Attachment = Equipment->ActorsToSpawn[0];
	const USkeletalMeshSocket* SourceMuzzle = Source->FindSocket(TEXT("weapon_r_muzzle"));
	if (Attachment.AttachSocket != TEXT("weapon_r") || !Attachment.AttachTransform.Equals(FTransform(FRotator(0.0, -90.0, 0.0)), 0.001) ||
		!SourceMuzzle || SourceMuzzle->BoneName != TEXT("weapon_r") || Target->FindSocket(TEXT("weapon_r_muzzle")))
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR original rifle attachment preflight failed; nothing changed."));
		return;
	}

	const TArray<FVirtualBone>& VirtualBones = Source->GetVirtualBones();
	if (VirtualBones.Num() != 3 || !Target->GetVirtualBones().IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR expected exactly three source virtual bones and an empty target."));
		return;
	}
	for (const FVirtualBone& Bone : VirtualBones)
	{
		const bool bExpected =
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_R_chestSpace") && Bone.SourceBoneName == TEXT("spine_05") && Bone.TargetBoneName == TEXT("hand_r")) ||
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_L_chestSpace") && Bone.SourceBoneName == TEXT("spine_05") && Bone.TargetBoneName == TEXT("hand_l")) ||
			(Bone.VirtualBoneName == TEXT("VB IK_Hand_L_weaponSpace") && Bone.SourceBoneName == TEXT("weapon_r") && Bone.TargetBoneName == TEXT("hand_l"));
		if (!bExpected || Target->GetReferenceSkeleton().FindBoneIndex(Bone.SourceBoneName) == INDEX_NONE ||
			Target->GetReferenceSkeleton().FindBoneIndex(Bone.TargetBoneName) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR virtual-bone preflight failed; nothing changed."));
			return;
		}
	}

	Target->Modify();
	for (const FVirtualBone& Bone : VirtualBones)
	{
		if (!Target->AddNewNamedVirtualBone(Bone.SourceBoneName, Bone.TargetBoneName, Bone.VirtualBoneName))
		{
			UE_LOG(LogTemp, Error, TEXT("HARU_RIG_REPAIR virtual bone failed; discard this process without saving."));
			return;
		}
	}
	Target->Sockets.Add(DuplicateObject<USkeletalMeshSocket>(SourceMuzzle, Target,
		MakeUniqueObjectName(Target, USkeletalMeshSocket::StaticClass())));

	// Preview assets do not apply WID_Rifle's attachment transform themselves.
	USkeletalMeshSocket* PreviewSocket = NewObject<USkeletalMeshSocket>(Target);
	PreviewSocket->SocketName = TEXT("weapon_r_preview");
	PreviewSocket->BoneName = TEXT("weapon_r");
	PreviewSocket->SetSocketLocalTransform(Attachment.AttachTransform);
	PreviewSocket->bForceAlwaysAnimated = true;
	Target->Sockets.Add(PreviewSocket);
	for (int32 Index = Target->PreviewAttachedAssetContainer.Num() - 1; Index >= 0; --Index)
	{
		if (Target->PreviewAttachedAssetContainer[Index].AttachedTo == TEXT("weapon_r"))
		{
			Target->PreviewAttachedAssetContainer.RemoveAtSwap(Index);
		}
	}
	Target->PreviewAttachedAssetContainer.AddUniqueAttachedObject(Rifle, PreviewSocket->SocketName);
	Target->MarkPackageDirty();
	Target->PostEditChange();
	UE_LOG(LogTemp, Display, TEXT("HARU_RIG_REPAIR_IN_MEMORY_OK virtual_bones=3 muzzle=1 preview=1 saves=0"));
}

FAutoConsoleCommand RestoreCommand(
	TEXT("DC.HaruRig.RestoreLyraSupport"),
	TEXT("Restore the three original Lyra virtual bones and rifle preview on HARU; requires -DCHaruRigRepair. Never saves assets."),
	FConsoleCommandDelegate::CreateStatic(&RestoreVirtualBonesAndPreview));

// Keep the original weapon Blueprint's skeletal socket/animation contract while
// presenting the designer's static SMG. Only the private driver copy is edited.
void ConfigureSMGPresentation()
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("DCSMGVisualSetup")))
	{
		UE_LOG(LogTemp, Error, TEXT("SMG_VISUAL_SETUP requires the unattended -DCSMGVisualSetup script."));
		return;
	}
	USkeletalMesh* Driver = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/SK_Rifle_SMGSocketDriver.SK_Rifle_SMGSocketDriver"));
	USkeletalMesh* Original = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Weapons/Rifle/Mesh/SK_Rifle.SK_Rifle"));
	USkeletalMesh* Haru = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU.SKM_HARU"));
	UStaticMesh* SMG = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_SMG/SM_GUN_SMG.SM_GUN_SMG"));
	UBlueprintGeneratedClass* WeaponClass = LoadObject<UBlueprintGeneratedClass>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle.B_Rifle_C"));
	UStaticMeshComponent* Visual = WeaponClass ? FindObject<UStaticMeshComponent>(WeaponClass, TEXT("SMGVisual_GEN_VARIABLE")) : nullptr;
	UClass* EquipmentClass = LoadClass<UDCLyraEquipmentDefinition>(nullptr, TEXT("/Game/DreamCatcher/Lyra/Weapons/Rifle/WID_Rifle.WID_Rifle_C"));
	const UDCLyraEquipmentDefinition* Equipment = EquipmentClass ? GetDefault<UDCLyraEquipmentDefinition>(EquipmentClass) : nullptr;
	USkeleton* HaruSkeleton = Haru ? Haru->GetSkeleton() : nullptr;
	USkeletalMeshSocket* Preview = HaruSkeleton ? HaruSkeleton->FindSocket(TEXT("weapon_r_preview")) : nullptr;
	if (!Driver || !Original || Driver == Original || Driver->GetSkeleton() != Original->GetSkeleton() ||
		!Visual || Visual->GetStaticMesh() != SMG || !SMG || !Preview || Preview->BoneName != TEXT("weapon_r") ||
		!Equipment || Equipment->ActorsToSpawn.Num() != 1 || Equipment->ActorsToSpawn[0].AttachSocket != TEXT("weapon_r"))
	{
		UE_LOG(LogTemp, Error, TEXT("SMG_VISUAL_SETUP asset/component preflight failed; nothing changed."));
		return;
	}
	const FReferenceSkeleton& RefSkeleton = Driver->GetRefSkeleton();
	const int32 RootIndex = RefSkeleton.FindBoneIndex(TEXT("Root"));
	if (RootIndex != 0 || RefSkeleton.GetParentIndex(RootIndex) != INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("SMG_VISUAL_SETUP expected the original Root bone."));
		return;
	}
	const FTransform VisualTransform = Visual->GetRelativeTransform();
	const FTransform RootTransform = RefSkeleton.GetRefBonePose()[RootIndex];
	const FName Names[] = { TEXT("Muzzle"), TEXT("ShellEject"), TEXT("ShellEject_0") };
	// Initial markers in SM_GUN_SMG asset space: measured barrel tip and right receiver.
	// The resulting mesh-only sockets remain editable on the private driver asset.
	const FVector Positions[] = {
		FVector(97.641319, 0.0, 32.743649),
		FVector(23.0, 5.2, 32.743649),
		FVector(23.0, 5.2, 32.743649)
	};
	Driver->Modify();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		USkeletalMeshSocket* Socket = nullptr;
		for (USkeletalMeshSocket* Existing : Driver->GetMeshOnlySocketList())
		{
			if (Existing && Existing->SocketName == Names[Index])
			{
				Socket = Existing;
				break;
			}
		}
		if (!Socket)
		{
			Socket = NewObject<USkeletalMeshSocket>(Driver);
			Socket->SocketName = Names[Index];
			Driver->GetMeshOnlySocketList().Add(Socket);
		}
		Socket->BoneName = TEXT("Root");
		const FTransform MeshSpace(VisualTransform.GetRotation(), VisualTransform.TransformPosition(Positions[Index]), FVector::OneVector);
		Socket->SetSocketLocalTransform(MeshSpace.GetRelativeTransform(RootTransform));
		Socket->bForceAlwaysAnimated = true;
	}
	Driver->MarkPackageDirty();
	Driver->PostEditChange();
	HaruSkeleton->Modify();
	Preview->SetSocketLocalTransform(VisualTransform * Equipment->ActorsToSpawn[0].AttachTransform);
	for (int32 Index = HaruSkeleton->PreviewAttachedAssetContainer.Num() - 1; Index >= 0; --Index)
	{
		if (HaruSkeleton->PreviewAttachedAssetContainer[Index].AttachedTo == TEXT("weapon_r_preview"))
		{
			HaruSkeleton->PreviewAttachedAssetContainer.RemoveAtSwap(Index);
		}
	}
	HaruSkeleton->PreviewAttachedAssetContainer.AddUniqueAttachedObject(SMG, TEXT("weapon_r_preview"));
	HaruSkeleton->MarkPackageDirty();
	HaruSkeleton->PostEditChange();
	UE_LOG(LogTemp, Display, TEXT("SMG_VISUAL_SETUP_IN_MEMORY_OK sockets=3 preview=1 saves=0"));
}

FAutoConsoleCommand SMGPresentationCommand(
	TEXT("DC.HaruRig.ConfigureSMG"),
	TEXT("Align private rifle socket driver and HARU preview to B_Rifle.SMGVisual; requires -DCSMGVisualSetup. Never saves assets."),
	FConsoleCommandDelegate::CreateStatic(&ConfigureSMGPresentation));
}

#endif // WITH_EDITOR
