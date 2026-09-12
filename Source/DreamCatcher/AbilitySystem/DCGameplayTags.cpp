// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/DCGameplayTags.h"

#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"
#include "DCLogChannels.h"

namespace DCGameplayTags
{
	// Native tags from Lyra.

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_IsDead, "Ability.ActivateFail.IsDead", "Ability failed to activate because its owner is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Cooldown, "Ability.ActivateFail.Cooldown", "Ability failed to activate because it is on cool down.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Cost, "Ability.ActivateFail.Cost", "Ability failed to activate because it did not pass the cost checks.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_TagsBlocked, "Ability.ActivateFail.TagsBlocked", "Ability failed to activate because tags are blocking it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_TagsMissing, "Ability.ActivateFail.TagsMissing", "Ability failed to activate because tags are missing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Networking, "Ability.ActivateFail.Networking", "Ability failed to activate because it did not pass the network checks.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_ActivationGroup, "Ability.ActivateFail.ActivationGroup", "Ability failed to activate because of its activation group.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Behavior_SurvivesDeath, "Ability.Behavior.SurvivesDeath", "An ability with this type tag should not be canceled due to death.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "InputTag.Look.Stick", "Look (stick) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "Crouch input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AutoRun, "InputTag.AutoRun", "Auto-run input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "1: Actor/component has initially spawned and can be extended");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable, "InitState.DataAvailable", "2: All required data has been loaded/replicated and is ready for initialization");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized, "InitState.DataInitialized", "3: The available data has been initialized for this actor/component, but it is not ready for full gameplay");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady", "4: The actor/component is fully ready for active gameplay");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Death, "GameplayEvent.Death", "Event that fires on death. This event only fires on the server.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reset, "GameplayEvent.Reset", "Event that fires once a player reset is executed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_RequestReset, "GameplayEvent.RequestReset", "Event to request a player's pawn to be instantly replaced with a new one at a valid spawn location.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "SetByCaller tag used by damage gameplay effects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal, "SetByCaller.Heal", "SetByCaller tag used by healing gameplay effects.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cheat_GodMode, "Cheat.GodMode", "GodMode cheat is active on the owner.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cheat_UnlimitedHealth, "Cheat.UnlimitedHealth", "UnlimitedHealth cheat is active on the owner.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Crouching, "Status.Crouching", "Target is crouching.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_AutoRunning, "Status.AutoRunning", "Target is auto-running.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death, "Status.Death", "Target has the death status.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dying, "Status.Death.Dying", "Target has begun the death process.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dead, "Status.Death.Dead", "Target has finished the death process.");

	// These are mapped to the movement modes inside GetMovementModeTagMap()
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Walking, "Movement.Mode.Walking", "Default Character movement tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_NavWalking, "Movement.Mode.NavWalking", "Default Character movement tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Falling, "Movement.Mode.Falling", "Default Character movement tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Swimming, "Movement.Mode.Swimming", "Default Character movement tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Flying, "Movement.Mode.Flying", "Default Character movement tag");

	// When extending Lyra, you can create your own movement modes but you need to update GetCustomMovementModeTagMap()
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Custom, "Movement.Mode.Custom", "This is invalid and should be replaced with custom tags. See DCGameplayTags::CustomMovementModeTagMap.");

	// Unreal Movement Modes
	const TMap<uint8, FGameplayTag> MovementModeTagMap =
	{
		{ MOVE_Walking, Movement_Mode_Walking },
		{ MOVE_NavWalking, Movement_Mode_NavWalking },
		{ MOVE_Falling, Movement_Mode_Falling },
		{ MOVE_Swimming, Movement_Mode_Swimming },
		{ MOVE_Flying, Movement_Mode_Flying },
		{ MOVE_Custom, Movement_Mode_Custom }
	};

	// Custom Movement Modes
	const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
	{
		// Fill these in with your custom modes.
	};

	FGameplayTag FindTagByString(
		const FString& TagString,
		bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(
						LogDC,
						Display,
						TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."),
						*TagString,
						*TestTag.ToString());

					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}

	// 현재 DreamCatcher 구현을 위한 임시 호환성 태그.

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look, "InputTag.Look", "Camera look input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Aim, "InputTag.Aim", "Aim input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "Dodge input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ultimate, "InputTag.Ultimate", "Ultimate input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Fire, "InputTag.Weapon.Fire", "Primary weapon fire input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Reload, "InputTag.Weapon.Reload", "Weapon reload input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Aim, "Ability.Action.Aim", "Aim ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Dodge, "Ability.Action.Dodge", "Dodge ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Ultimate, "Ability.Action.Ultimate", "Ultimate ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_WeaponFire, "Ability.Action.WeaponFire", "Weapon fire ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Reload, "Ability.Action.Reload", "Reload ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Death, "Ability.Action.Death", "Death ability.");

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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_DamageImmunity, "Gameplay.DamageImmunity", "Actor ignores normal incoming damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked", "Ability input processing is blocked.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Healing, "SetByCaller.Healing", "Healing magnitude supplied when creating a GameplayEffect spec.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Dodge_Success, "GameplayEvent.Dodge.Success", "Damage was successfully avoided during dodge.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dodge, "Cooldown.Dodge", "Dodge cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ultimate, "Cooldown.Ultimate", "Ultimate cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Hip, "Camera.Type.Hip", "Default third-person hip camera.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Shoulder, "Camera.Type.Shoulder", "Third-person shoulder aiming camera.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Camera_Type_Scope, "Camera.Type.Scope", "Scoped aiming camera.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Rifle_Fire, "GameplayCue.Weapon.Rifle.Fire", "Cosmetic feedback for a committed rifle shot.");
}