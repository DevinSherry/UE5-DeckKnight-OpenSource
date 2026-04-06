// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(InputTag_Move, "Input.NativeAction.Move")
UE_DEFINE_GAMEPLAY_TAG(InputTag_PointClickMovement, "Input.NativeAction.PointClickMovement")
UE_DEFINE_GAMEPLAY_TAG(InputTag_Look_Stick, "Input.NativeAction.GamepadLook")
UE_DEFINE_GAMEPLAY_TAG(InputTag_Jump, "Input.NativeAction.Jump")
UE_DEFINE_GAMEPLAY_TAG(InputTag_Evade, "Input.NativeAction.Evade")
UE_DEFINE_GAMEPLAY_TAG(InputTag_WeaponPrimaryFire, "Input.NativeAction.PrimaryWeaponFire")
UE_DEFINE_GAMEPLAY_TAG(InputTag_Crouch, "Input.NativeAction.Crouch")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityBase, "Input.NativeAction.Ability")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityOne, "Input.NativeAction.Ability.1")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityTwo, "Input.NativeAction.Ability.2")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityThree, "Input.NativeAction.Ability.3")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityFour, "Input.NativeAction.Ability.4")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AbilityFive, "Input.NativeAction.Ability.5")
UE_DEFINE_GAMEPLAY_TAG(InputTag_AssignActiveSlot, "Input.NativeAction.Ability.AssignSlot")
UE_DEFINE_GAMEPLAY_TAG(InputTag_EquipmentAbilityOne, "Input.NativeAction.Ability.Equipment.One")
UE_DEFINE_GAMEPLAY_TAG(InputTag_EquipmentAbilityTwo, "Input.NativeAction.Ability.Equipment.Two")
UE_DEFINE_GAMEPLAY_TAG(InputTag_ConfirmTargetData, "Input.NativeAction.ConfirmTargeting")
UE_DEFINE_GAMEPLAY_TAG(InputTag_CancelTargetData, "Input.NativeAction.CancelTargeting")
UE_DEFINE_GAMEPLAY_TAG(InputTag_BlockStance, "Input.NativeAction.Stance.Block")

UE_DEFINE_GAMEPLAY_TAG(InputTag_MoveCamera, "Input.NativeAction.Camera.Movement")
UE_DEFINE_GAMEPLAY_TAG(InputTag_RecenterCamera, "Input.NativeAction.Camera.Recenter")
UE_DEFINE_GAMEPLAY_TAG(InputTag_RotateCamera, "Input.NativeAction.Camera.Rotate")
UE_DEFINE_GAMEPLAY_TAG(InputTag_RotateCameraAxis, "Input.NativeAction.Camera.Rotate.Axis")
UE_DEFINE_GAMEPLAY_TAG(InputTag_CameraZoom, "Input.NativeAction.Camera.Zoom")
UE_DEFINE_GAMEPLAY_TAG(InputTag_CameraMovementChordedAction, "Input.NativeAction.Camera.Enable.Movement.Chorded")
UE_DEFINE_GAMEPLAY_TAG(InputTag_CameraRotationChordedAction, "Input.NativeAction.Camera.Enable.Rotation.Chorded")

UE_DEFINE_GAMEPLAY_TAG(Status_Crouching, "Status.Crouching")
UE_DEFINE_GAMEPLAY_TAG(Status_Falling, "Status.Falling")
UE_DEFINE_GAMEPLAY_TAG(Status_IsMoving, "Status.IsMoving")
UE_DEFINE_GAMEPLAY_TAG(Status_Block_PointClickMovementInput, "Status.Block.Input.PointClickMovement")
UE_DEFINE_GAMEPLAY_TAG(Status_Gameplay_Targeting, "Status.Gameplay.Targeting")
UE_DEFINE_GAMEPLAY_TAG(Status_Block_MovementInput, "Status.Block.Input.Movement")
UE_DEFINE_GAMEPLAY_TAG(Status_Block_RotationInput, "Status.Block.Input.Rotation")
UE_DEFINE_GAMEPLAY_TAG(Status_Block_AbilityInput, "Status.Block.Input.AbilityActivation")
UE_DEFINE_GAMEPLAY_TAG(Status_Block_PassiveCardEnergyXPGain, "Status.Block.Cards.EnergyXP.PassiveGain")
UE_DEFINE_GAMEPLAY_TAG(Status_Death, "Status.Death")
UE_DEFINE_GAMEPLAY_TAG(Status_CanMoveInterrupt, "Status.Movement.CanInterrupt")
UE_DEFINE_GAMEPLAY_TAG(Status_Damage_Immunity_All, "Status.Damage.Immunity.All")
UE_DEFINE_GAMEPLAY_TAG(Status_Damage_Immunity_Elemental_Fire, "Status.Damage.Immunity.Elemental.Fire")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Immortal, "Status.Immortal",
	"When applied, character health never reaches below 1.0f.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Invulnerable_Damage, "Status.Invulnerable.Damage",
	"When applied, a character does not receive damage or reactions at all.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Invulnerable_Reactions, "Status.Invulnerable.Reactions",
	"When applied, a character does not play any animation reactions, except for death.")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_CardEnergyCostOverride, "Status.Card.CostOverride",
	"When applied, card cost will equal value set to GetCardEnergyCostOverrideAttribute.")

UE_DEFINE_GAMEPLAY_TAG(Data_IncomingDamage, "Data.IncomingDamage")
UE_DEFINE_GAMEPLAY_TAG(Data_IncomingHealing, "Data.IncomingHealing")
UE_DEFINE_GAMEPLAY_TAG(Data_IncomingCardEnergyXP, "Data.IncomingCardEnergyXP")
UE_DEFINE_GAMEPLAY_TAG(Data_CachedDamage, "Data.Damage.Cached")
UE_DEFINE_GAMEPLAY_TAG(Data_DamageOverTime, "Data.Damage.OverTime")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_ActiveCardEnergyXP, "Data.CardEnergyXP.Active",
	"Used to tag incoming Card Energy XP as Active, meaning the XP was gained from an in-game action/event.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_DebugSimulated, "Data.Damage.Simulated",
	"Used to inform debug systems that the damage applied was simulated.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_DamageCritical, "Data.Damage.Critical",
	"Used to inform damage systems that the damage applied was a critical hit.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_DamageResisted, "Data.Damage.Resisted",
	"Used to inform damage systems that the damage applied was a critical hit.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_HealingLifeSteal, "Data.Healing.LifeSteal",
	"Used to inform damage systems that the healing applied was from life steal mechanics.")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_CardCost, "Data.Card.Cost",
	"Used to inform card cost execution class of the base cost of the activated card.")

UE_DEFINE_GAMEPLAY_TAG(DamageType_Root, "Damage.Type")
UE_DEFINE_GAMEPLAY_TAG(DamageType_Physical, "Damage.Type.Physical")
UE_DEFINE_GAMEPLAY_TAG(DamageType_Elemental, "Damage.Type.Elemental")
UE_DEFINE_GAMEPLAY_TAG(DamageType_Elemental_Fire, "Damage.Type.Elemental.Fire")
UE_DEFINE_GAMEPLAY_TAG(DamageType_Healing, "Damage.Type.Healing")

UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay, "Event.Gameplay")
UE_DEFINE_GAMEPLAY_TAG(Event_OnDeath, "Event.Gameplay.OnDeath")
UE_DEFINE_GAMEPLAY_TAG(Event_OnDeathDealt, "Event.Gameplay.OnDeathDealt")
UE_DEFINE_GAMEPLAY_TAG(Event_OnStatusDeath, "Event.Gameplay.Status.OnDeath")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnDamageDealt, "Event.Gameplay.OnDamageDealt")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnDamageReceived, "Event.Gameplay.OnDamageReceived")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnHit, "Event.Gameplay.OnHit")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnHealing, "Event.Gameplay.OnHealing")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnTargetHealed, "Event.Gameplay.OnTargetHealed")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_OnAbilityGranted, "Event.Gameplay.OnAbilityGranted")
UE_DEFINE_GAMEPLAY_TAG(Event_Gameplay_Reaction_OnHit, "Event.Gameplay.Reaction.OnHit")

UE_DEFINE_GAMEPLAY_TAG(Reaction_OnHit, "Reaction.Hit")

UE_DEFINE_GAMEPLAY_TAG(Collision_IgnorePawn, "Collision.Ignore.Pawn")


UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_IsDead, "Ability.Activation.Failure.IsDead")
UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_OnCooldown, "Ability.Activation.Failure.OnCooldown")
UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_CantAffordCost, "Ability.Activation.Failure.CantAffordCost")
UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_BlockedByTags, "Ability.Activation.Failure.BlockedByTags")
UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_MissingTags, "Ability.Activation.Failure.MissingTags")
UE_DEFINE_GAMEPLAY_TAG(AbilityActivationFail_Networking, "Ability.Activation.Failure.Networking")

UE_DEFINE_GAMEPLAY_TAG(Event_AbilityActivation_Fail, "Event.Ability.Activation.Failure");
UE_DEFINE_GAMEPLAY_TAG(Event_AbilityActivation_CantAffordCost, "Event.Ability.Activation.CantAffordCost");


FGASCourseNativeGameplayTags FGASCourseNativeGameplayTags::GameplayTags;
