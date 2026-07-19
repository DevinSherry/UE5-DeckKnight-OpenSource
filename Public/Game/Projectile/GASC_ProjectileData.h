// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "GASC_ProjectileData.generated.h"

class AGASCourseProjectile;
class UGASC_AreaOfEffectData;
class UGameplayEffect;
class UNiagaraSystem;

UENUM(Blueprintable)
enum class EProjectileHomingDisableRules : uint8
{
	DoTThreshold UMETA(DisplayName="Dot Threshold"),
	Timeout UMETA(DisplayName="Timeout")
};

UENUM(Blueprintable)
enum class EProjectileEventType : uint8
{
	OnProjectileSpawn UMETA(DisplayName="On Projectile Spawn"),
	OnProjectileHit UMETA(DisplayName="On Projectile Hit"),
	OnProjectileExpire UMETA(DisplayName="On Projectile Expired"),
	OnProjectileRicochet UMETA(DisplayName="On Projectile Ricochet"),
	OnProjectilePierce UMETA(DisplayName="On Projectile Pierce")
};

USTRUCT()
struct FProjectileGroup
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid GroupId;

	UPROPERTY()
	TArray<TWeakObjectPtr<AGASCourseProjectile>> Projectiles;

	UPROPERTY()
	TWeakObjectPtr<AGASCourseProjectile> Leader;
};

USTRUCT(meta=(Hidden))
struct FProjectileFragmentBase: public FTableRowBase
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FProjectileVisualsFragment : public  FProjectileFragmentBase
{
	GENERATED_BODY()
	
	/**
	 * @brief Specifies the static mesh to be used for the visual representation of the projectile.
	 *
	 * This variable holds a reference to a `UStaticMesh` asset that defines the appearance
	 * of the projectile in the game. It can be set to any valid static mesh and allows customization
	 * of the projectile's visual design directly through the editor or via blueprints.
	 *
	 * The selected mesh will serve as the primary visual component for the projectile, enabling
	 * developers to represent various projectile types with unique appearances. This is crucial
	 * for distinguishing projectiles in gameplay and aligning their visuals with specific game mechanics.
	 *
	 * Modifiable through blueprints and editor properties, this variable ensures flexibility
	 * in defining projectile visuals for diverse gameplay requirements.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	/**
	 * @brief Defines a transformation override for the projectile's mesh.
	 *
	 * This variable provides the ability to set a custom transformation matrix
	 * that overrides the default mesh transform for a projectile. It can be used
	 * to apply specific position, rotation, or scaling adjustments to the mesh of
	 * the projectile independently of its parent object or system-wide transforms.
	 *
	 * The use case for this variable includes scenarios where precise customization
	 * of mesh behavior is required, such as ensuring visual accuracy or alignment
	 * in gameplay mechanics.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual", meta=(EditCondition="!ProjectileMesh.IsNull()", EditConditionHides))
	FTransform ProjectileMeshTransformOverride;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual")
	TSoftObjectPtr<UNiagaraSystem> ProjectileTrailVFX;

	/**
	 * @brief Specifies a transformation override for the projectile's trail effect.
	 *
	 * This variable allows defining a custom transformation matrix to override
	 * the default trail transform associated with a projectile. It enables
	 * precise control over the position, rotation, or scaling of the trail effect,
	 * independent of other transformations applied to the projectile itself.
	 *
	 * This is particularly useful in cases where the trail visual appearance needs
	 * customization to achieve specific effects or align properly with unique
	 * gameplay requirements or visual designs.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual", meta=(EditCondition="!ProjectileMesh.IsNull()", EditConditionHides))
	FTransform ProjectileTrailTransformOverride;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual")
	TSoftObjectPtr<UNiagaraSystem> ProjectileImpactVFX;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual")
	TSoftObjectPtr<UNiagaraSystem> ProjectileExpireVFX;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Visual")
	TSoftObjectPtr<USoundBase> ProjectileTravelSFX;

	/**
	 * @brief Defines the gameplay cue tag triggered upon impact.
	 *
	 * This variable holds an `FGameplayTag` that specifies the gameplay cue
	 * to be executed when the associated impact event occurs. Gameplay cues
	 * are used to play visual or audio effects, spawn particles, or trigger any
	 * behavior defined in the cue system.
	 *
	 * The tag can be set to any valid gameplay cue category, enabling seamless
	 * integration of custom effects or responses during gameplay. By using this
	 * tag, developers can dynamically associate context-sensitive impact cues
	 * tailored to specific game mechanics or entities.
	 *
	 * Configurable only through the editor, this variable provides a consistent
	 * approach to managing impact-related cues across various game objects or
	 * behaviors.
	 */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue, meta=(Categories="GameplayCue"))
	FGameplayTag ImpactGameplayCueTag;

	/**
	 * @brief Represents the gameplay cue triggered upon the expiration of an effect or state.
	 *
	 * This variable holds a `FGameplayTag` that corresponds to the specific gameplay cue
	 * to be invoked when an associated gameplay effect or state expires. Gameplay cues
	 * are visual or audio indicators used to communicate gameplay events to players.
	 *
	 * By assigning a valid gameplay tag, developers can define custom behaviors
	 * or effects that are activated when the expiration event occurs. This can include
	 * animations, particle effects, sounds, or any other type of feedback intended
	 * to enhance the player's understanding of in-game events.
	 *
	 * Configurable as an editable property within the editor, it allows for easy modification
	 * and alignment of expiration cues with various gameplay needs. The `ExpireGameplayCueTag`
	 * plays a critical role in maintaining clear feedback loops during gameplay.
	 */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue, meta=(Categories="GameplayCue"))
	FGameplayTag ExpireGameplayCueTag;
};

USTRUCT(BlueprintType)
struct FProjectileCollisionFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Collision")
	float ProjectileCollisionRadius = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Collision")
	bool bCollisionProfileOverride = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision", meta=(EditCondition = "bCollisionProfileOverride", EditConditionHides, GetOptions="GetCollisionProfileOptions"))
	FName CollisionProfileName;
	
	FProjectileCollisionFragment()
	{
		CollisionProfileName = "Projectile";
	}

};

USTRUCT(BlueprintType)
struct FProjectileMovementFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	/**
	 * @brief Defines the initial speed of the projectile upon being launched.
	 *
	 * This variable determines how fast the projectile travels at the moment it is spawned
	 * or fired into the game world. The speed is measured in unreal units per second and
	 * can be adjusted to influence the velocity at the start of the projectile's movement.
	 *
	 * By modifying this property through blueprints or the editor, developers can customize
	 * the behavior of projectile-based mechanics, such as increasing or decreasing the
	 * launch speed to suit various gameplay scenarios or weapon types.
	 *
	 * This parameter plays an essential role in controlling the trajectory and range of the
	 * projectile, with higher values resulting in faster travel and potentially covering
	 * greater distances within a shorter period.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement")
	float ProjectileInitialSpeed = 1000.0f;

	/**
	 * @brief Sets the maximum speed limit for the projectile's movement.
	 *
	 * This variable defines the highest velocity the projectile can achieve during its trajectory.
	 * Measured in unreal units per second, it serves as a cap on the projectile's speed, ensuring
	 * that its motion does not exceed the value specified. This is particularly relevant in scenarios
	 * where movement mechanics or gameplay mechanics require controlled speed boundaries.
	 *
	 * The `ProjectileMaxSpeed` property can be customized through blueprints or the editor, allowing
	 * developers to fine-tune projectile dynamics to match the desired gameplay experience. By doing so,
	 * this property enables precise control over balance, realism, and intended functionality of
	 * the projectile's behavior within the game environment.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement")
	float ProjectileMaxSpeed = 1000.0f;

	/**
	 * @brief Determines whether gravity affects the projectile's movement.
	 *
	 * This variable controls whether the projectile is influenced by gravity during its trajectory.
	 * When set to `true`, the projectile will be subject to gravitational acceleration, causing its
	 * path to curve downward over time. Conversely, when set to `false`, the projectile will not
	 * be affected by gravity, allowing it to travel along a straight path unless influenced by
	 * other forces or behaviors.
	 *
	 * This property can be modified in the editor or through blueprints, providing flexibility
	 * for designing projectiles with or without realistic gravitational effects. It is particularly
	 * useful for creating diverse gameplay mechanics, such as simulating ballistic motion or designing
	 * projectiles with custom flight characteristics.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement")
	bool bUseGravity = false;

	/**
	 * @brief Adjusts the influence of gravity on the projectile's movement.
	 *
	 * This variable serves as a multiplier for the gravity force affecting the projectile. A value
	 * of `1.0` applies the default gravity specified by the game engine, while values greater or
	 * less than `1.0` respectively increase or reduce the gravitational effect. Setting the value
	 * to `0.0` effectively disables gravity for the projectile, allowing it to move without any
	 * downward force unless other forces are introduced.
	 *
	 * By customizing this property in the editor or through blueprints, developers can control
	 * the behavior of the projectile's trajectory, such as creating arcs influenced by heavier
	 * gravitational forces or simulating near-zero gravity environments. This provides flexibility
	 * for designing varied gameplay mechanics and realistic or stylized projectiles.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement", meta=(EditCondition="bUseGravity", EditConditionHides))
	float GravityScale = 1.0f;
};

USTRUCT(BlueprintType)
struct FProjectileParabolicMovementFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement")
	FVector VelocityOverride = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FProjectileGameplayEffectData
{
	GENERATED_BODY()

	/**
	 * @brief Represents the type or category of an event within the system.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|GameplayEffect")
	EProjectileEventType EventType;

	/**
	 * @brief Represents the gameplay effect applied to a target entity within the game.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|GameplayEffect")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	/**
	 * @brief Indicates whether the effect or operation should be applied to the instigator of an action or the target.
	 * If false, the effect or operation will be applied to the target of the action. Target is not valid for spawn and expired events.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|GameplayEffect")
	bool bApplyOnInstigator = true;
	
};

USTRUCT(BlueprintType)
struct FProjectileGameplayEffectsFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|GameplayEffect")
	TArray<FProjectileGameplayEffectData> GameplayEffects;
};

USTRUCT(BlueprintType)
struct FProjectilePiercingFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Piercing")
	bool bCanPierce = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Piercing")
	float PiercingChance = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Piercing")
	int32 PierceCount = 1;
};

USTRUCT(BlueprintType)
struct FProjectileHomingMovementFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement|Homing")
	bool bUseHoming = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing")
	float HomingAcceleration = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing")
	TWeakObjectPtr<USceneComponent> HomingTargetComponent = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing")
	TWeakObjectPtr<AActor> HomingTarget = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing")
	EProjectileHomingDisableRules HomingDisableRule = EProjectileHomingDisableRules::DoTThreshold;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing", meta=(EditCondition="HomingDisableRule==EProjectileHomingDisableRules::Timeout", EditConditionHides))
	float HomingTimeout = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Movement|Homing", meta=(EditCondition="HomingDisableRule==EProjectileHomingDisableRules::Timeout", EditConditionHides))
	float DoTThreshold = 0.0f;
};

USTRUCT(BlueprintType)
struct FProjectilePoolingData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Lifetime")
	float LifeTime = -1.0f;
	
};

USTRUCT(BlueprintType)
struct FProjectileHitEvent
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|HitEvent")
	FGameplayTag OnHitEventTag;
	
	//Primarily used for weapons.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|HitEvent")
	TWeakObjectPtr<UObject> OptionalObject = nullptr;
	
	// Appends the instigator actors' gameplay tags to pass along in the payload
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|HitEvent")
	FGameplayTagContainer AdditionalInstigatorTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|HitEvent")
	bool bSendEventtoStateTree = true;
};

USTRUCT(BlueprintType)
struct FProjectileDamageFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	float Damage = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage", meta=(Categories = "Damage.Type"))
	FGameplayTag DamageType;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	FGameplayTagContainer DamageGrantedTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	bool bCanDamageAllies = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	bool bDamageOverTime = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage", meta=(EditCondition="bDamageOverTime", EditConditionHides))
	FDamagePipelineEffectOverTimeContext EffectOverTimeContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	FProjectileHitEvent OnHitEvent;
};

USTRUCT(BlueprintType)
struct FProjectileHealingFragment: public FProjectileFragmentBase
{	
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing")
	float Healing = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing", meta=(Categories = "Damage.Type"))
	FGameplayTag HealingType = DamageType_Healing;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing")
	FGameplayTagContainer HealingGrantedTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing")
	bool bCanHealEnemies = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing")
	bool bHealOverTime = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing", meta=(EditCondition="bHealOverTime", EditConditionHides))
	FDamagePipelineEffectOverTimeContext EffectOverTimeContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Healing")
	FProjectileHitEvent OnHitEvent;
	
};

USTRUCT(BlueprintType)
struct FProjectileRicochetFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Ricochet")
	bool bCanRicochet = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Ricochet")
	float RicochetChance = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Ricochet")
	int32 NumberOfRicochet = 0;
	
	//TODO Add target preset
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|SpawnShape")
	float SpawnDelayBetween = -1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	bool bCustomSpawnTransform = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape", meta=(EditConditionHides="bCustomSpawnTransform"))
	FTransform SpawnTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeCircleFragment : public FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float SpawnRadius = 100.0f;
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeConeFragment : public FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float ConeAngle = 60.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float SpawnRadius = 100.0f;
	
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeSpiralFragment : public FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	int32 NumRotations = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float SpawnRadius = 100.0f;
	
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeLineFragment : public FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	bool bUseInstigatorActorForward = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	FVector SpawnDirection = FVector::ForwardVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float SpawnRadius = 100.0f;
	
};

USTRUCT(BlueprintType)
struct FProjectileSpawnShapeSnakeFragment : public FProjectileSpawnShapeBaseFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	float SpawnRadius = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpawnShape")
	bool bStopTrailAfterLeaderEnd = false;
	
	UPROPERTY()
	TObjectPtr<AGASCourseProjectile> LeaderProjectile;
	
	UPROPERTY()
	TArray<TWeakObjectPtr<AGASCourseProjectile>> SnakeProjectiles;
	
	UPROPERTY()
	TArray<FVector> SnakeTrailLocations;
	
	UPROPERTY()
	TArray<FRotator> SnakeTrailRotations;
	
	UPROPERTY()
	bool bIsLeader;
	
	UPROPERTY()
	int32 SnakeTrailIndex = -1;
	
	FProjectileSpawnShapeSnakeFragment()
	{
		bIsLeader = false;
		LeaderProjectile = nullptr;
		SnakeTrailLocations = TArray<FVector>();
		SnakeTrailRotations = TArray<FRotator>();
		SnakeTrailIndex = -1;
	}
	
};

USTRUCT(BlueprintType)
struct FProjectileOrbitingFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Orbit")
	float OrbitAngleDegrees = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Orbit")
	float OrbitRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Orbit")
	float OrbitSpeedDegreesPerSecond = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Orbit")
	float OrbitHeightOffset = 50.f;
};


USTRUCT(BlueprintType)
struct FProjectileAOESpawnData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|AOESpawn")
	TObjectPtr<UGASC_AreaOfEffectData> AOEClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|AOESpawn")
	EProjectileEventType EventType;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|AOESpawn")
	bool bSpawnAtInstigatorLocation = true;
};

USTRUCT(BlueprintType)
struct FProjectileAOESpawnFragment : public FProjectileFragmentBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|AOESpawn")
	TArray<FProjectileAOESpawnData> AreaOfEffectFragmentData;
	
};

/**
 * @brief A data container for encapsulating configuration details of a projectile.
 *
 * This class serves as a centralized structure to store and manage attributes related
 * to projectiles within the game. It allows developers to define and organize properties
 * such as damage, speed, lifetime, and any other gameplay-specific parameters associated
 * with projectiles.
 *
 * By utilizing this class, projectiles can be configured in a modular and reusable manner,
 * streamlining the process of adjusting projectile behavior and attributes across different
 * scenarios. This enhances maintainability and enables consistent behavior for similar
 * projectile types.
 *
 * Designed for flexibility, the class can be extended or modified to accommodate new
 * projectile attributes, ensuring scalability for diverse gameplay mechanics and future
 * feature implementations.
 */
UCLASS()
class GASCOURSE_API UGASC_ProjectileData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Data|Pooling")
	FProjectilePoolingData ProjectilePoolingData;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Fragments", meta = (BaseStruct = "/Script/GASCourse.ProjectileFragmentBase"))
	TArray<FInstancedStruct> ProjectileDataFragments;
	
#if WITH_EDITOR
	UFUNCTION()
	TArray<FName> GetCollisionProfileOptions() const;
#endif
	
};
