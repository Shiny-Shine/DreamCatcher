// Copyright Epic Games, Inc. All Rights Reserved.

#include "DCGameplayEffectContext.h"

#include "AbilitySystem/DCAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serialization/GameplayEffectContextNetSerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCGameplayEffectContext)

class FArchive;

FDCGameplayEffectContext* FDCGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FDCGameplayEffectContext::StaticStruct()))
	{
		return (FDCGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FDCGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

namespace UE::Net
{
	// Forward to FGameplayEffectContextNetSerializer
	// Note: If FDCGameplayEffectContext::NetSerialize() is modified, a custom NetSerializer must be implemented as the current fallback will no longer be sufficient.
	UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(DCGameplayEffectContext, FGameplayEffectContextNetSerializer);
}

void FDCGameplayEffectContext::SetAbilitySource(const IDCAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IDCAbilitySourceInterface* FDCGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IDCAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FDCGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

