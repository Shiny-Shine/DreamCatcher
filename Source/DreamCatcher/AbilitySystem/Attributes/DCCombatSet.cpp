#include "AbilitySystem/Attributes/DCCombatSet.h"

#include "Net/UnrealNetwork.h"

UDCCombatSet::UDCCombatSet() : BaseDamage(0.0f), BaseHeal(0.0f)
{
}

void UDCCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 공격력과 회복력은 현재 소유 플레이어의 HUD나 계산에만 필요하므로 Owner에게만 복제.
	// BaseDamage는 무기 자체의 최종 데미지는 아니지만. 이후 Damage Execution에서
	// 최종 데미지 = BaseDamage × 무기 배율 × 거리 배율 × 부위 배율 × 버프·디버프 배율 처럼 사용 가능
	DOREPLIFETIME_CONDITION_NOTIFY(UDCCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UDCCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UDCCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCCombatSet, BaseDamage, OldValue);
}

void UDCCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDCCombatSet, BaseHeal, OldValue);
}
