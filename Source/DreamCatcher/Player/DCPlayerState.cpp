#include "Player/DCPlayerState.h"

#include "AbilitySystem/DCAbilitySet.h"
#include "AbilitySystem/DCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DCHealthSet.h"
#include "AbilitySystem/Attributes/DCCombatSet.h"
#include "AbilitySystem/Attributes/DCResourceSet.h"

ADCPlayerState::ADCPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// ASC는 PlayerState의 기본 Subobject로 생성.
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UDCAbilitySystemComponent>(
		this,TEXT("AbilitySystemComponent"));

	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed 모드는 플레이어 ASC에 적합한 복제 모드.
	// 소유자는 상세한 Effect 정보를 받고, 다른 클라이언트는 필요한 Tag와 Cue 중심으로 받음.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// ASC가 InitializeComponent를 실행할 때 소유 Actor의 AttributeSet Subobject를 자동으로 감지.
	HealthSet = ObjectInitializer.CreateDefaultSubobject<UDCHealthSet>(this,TEXT("HealthSet"));

	CombatSet = ObjectInitializer.CreateDefaultSubobject<UDCCombatSet>(this,TEXT("CombatSet"));

	ResourceSet = ObjectInitializer.CreateDefaultSubobject<UDCResourceSet>(this, TEXT("ResourceSet"));

	// ASC 상태가 빠르게 갱신될 수 있도록 설정합니다.
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* ADCPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADCPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	check(HealthSet);
	check(CombatSet);
	check(ResourceSet);

	/*
	 * 현재는 Pawn이 아직 없을 수 있음.
	 *
	 * Owner는 PlayerState로 설정하고, Avatar는 이후 Pawn 초기화 단계에서 다시 연결.
	 */
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	// Ability 부여는 Authority에서만 수행.
	if (!HasAuthority())
	{
		return;
	}

	for (const UDCAbilitySet* AbilitySet : PlayerStateAbilitySets)
	{
		if (AbilitySet)
		{
			// PlayerState AbilitySet은 PlayerState와 함께 유지하므로 제거용 Handle은 현재 저장하지 않음. 
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent);
		}
	}

	// Foundation 단계에서 Ability 부여 여부를 확인하는 로그.
	UE_LOG(LogTemp, Log,
	       TEXT("DCPlayerState [%s] ASC initialized. ""Granted Ability count: %d, ""Health: %.1f / %.1f, "
		       "BaseDamage: %.1f, ""Ultimate: %.1f / %.1f"),
	       *GetNameSafe(this), AbilitySystemComponent->GetActivatableAbilities().Num(), HealthSet->GetHealth(),
	       HealthSet->GetMaxHealth(), CombatSet->GetBaseDamage(), ResourceSet->GetUltimateCharge(),
	       ResourceSet->GetMaxUltimateCharge()
	);
}
