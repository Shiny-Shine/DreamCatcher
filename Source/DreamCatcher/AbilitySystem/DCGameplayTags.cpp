#include "AbilitySystem/DCGameplayTags.h"

namespace DCGameplayTags
{
	// Native Input
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Character movement input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look, "InputTag.Look", "Camera look input.");

	// Ability Input
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Aim, "InputTag.Aim", "Aim input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "Dodge input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ultimate, "InputTag.Ultimate", "Ultimate input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Fire, "InputTag.Weapon.Fire", "Primary weapon fire input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Reload, "InputTag.Weapon.Reload", "Weapon reload input.");

	// Ability
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Aim, "Ability.Action.Aim", "Aim ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Dodge, "Ability.Action.Dodge", "Dodge ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Ultimate, "Ability.Action.Ultimate", "Ultimate ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_WeaponFire, "Ability.Action.WeaponFire", "Weapon fire ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Reload, "Ability.Action.Reload", "Reload ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Death, "Ability.Action.Death", "Death ability.");

	// State
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aim_Shoulder, "State.Aim.Shoulder", "Shoulder aim is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aim_Scope, "State.Aim.Scope", "Scope aim is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "Dodge is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Firing, "State.Firing", "Weapon firing is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Reloading, "State.Reloading", "Weapon reload is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Actor is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attack_Intent, "State.Attack.Intent", "Enemy attack intent.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attack_Windup, "State.Attack.Windup", "Enemy attack windup.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attack_Active, "State.Attack.Active", "Enemy attack hit window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attack_Recovery, "State.Attack.Recovery", "Enemy attack recovery.");

	// Gameplay
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_DamageImmunity, "Gameplay.DamageImmunity",
	                               "Actor ignores normal incoming damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked",
	                               "Ability input processing is blocked.");

	// SetByCaller
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage",
	                               "Damage magnitude supplied when creating a GameplayEffect spec.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Healing, "SetByCaller.Healing",
	                               "Healing magnitude supplied when creating a GameplayEffect spec.");

	// Event
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Death, "GameplayEvent.Death", "Death event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Dodge_Success, "GameplayEvent.Dodge.Success",
	                               "Damage was successfully avoided during dodge.");

	// Cooldown
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dodge, "Cooldown.Dodge", "Dodge cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ultimate, "Cooldown.Ultimate", "Ultimate cooldown.");

	// Camera
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Hip, "Camera.Type.Hip", "Default third-person hip camera.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Shoulder, "Camera.Type.Shoulder",
	                               "Third-person shoulder aiming camera.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Scope, "Camera.Type.Scope", "Scoped aiming camera.");
}
