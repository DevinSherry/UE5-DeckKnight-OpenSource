// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "GASC_ProjectileData.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "Types/TargetingSystemTypes.h"
#include "GASCourseProjectile.generated.h"

class URotatingMovementComponent;
struct FGameplayEventData;
class UNiagaraComponent;
class UCapsuleComponent;
class UGASCourseProjectileMovementComp;
class UTargetingPreset;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProjectileLifetimeExpired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHit, AActor*, OtherActor, FHitResult, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProjectileCreated, const AActor*, InstigatorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileRicochet, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectilePierce, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileReturnedToPool, AActor*, Projectile);

class TargetingSystemTypes;

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_Projectile, Log, All);

UCLASS()
class GASCOURSE_API AGASCourseProjectile : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	AGASCourseProjectile();
	
	UPROPERTY(BlueprintAssignable)
	FProjectileLifetimeExpired OnProjectileLifetimeExpiredDelegate;
	
	UPROPERTY()
	FProjectileCreated OnProjectileCreatedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectileHit OnProjectileHitDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectileRicochet OnProjectileRicochetDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectileReturnedToPool OnProjectileReturnedToPoolDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectilePierce OnProjectilePierceDelegate;
	
	UPROPERTY(meta = (BaseStruct = "/Script/GASCourse.ProjectileFragmentBase"))
	TArray<FInstancedStruct> ProjectileFragments;
	
	/**
	 * Manages the movement of the projectile.
	 * Provides functionality for controlling the trajectory, speed, and behavior of the projectile as it moves through the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASCourseProjectileMovementComp> ProjectileMovementComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> ProjectileRootComponent;

	/**
	 * Defines the collision behavior for the projectile.
	 * Serves as the collision representation, determining how the projectile interacts with objects in the game world upon contact.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ProjectileCollisionComp;

	/**
	 * Represents the visual appearance of the projectile.
	 * Handles the static mesh component used to render the projectile's visuals within the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ProjectileVisualMeshComponent;

	/**
	 * Handles visual effects for the projectile using the Niagara particle system.
	 * Manages rendering and simulation of particle-based effects associated with the projectile.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ProjectileNiagaraComponent;

	/**
	 * Handles the audio associated with the projectile.
	 * Facilitates playing, controlling, and managing sound effects specific to the projectile's behavior and interactions in the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAudioComponent> ProjectileAudioComponent;
	/**
	 * Represents the actor targeted by the projectile.
	 * Used to designate or track the intended target during the projectile's behavior or trajectory.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite,  Category = Projectile, meta = (ExposeOnSpawn=true))
	AActor* TargetActor = nullptr;

	/**
	 * Determines whether the target actor is an ally.
	 * This function is designed to ascertain if the actor hit by the projectile aligns with the same team or affiliation.
	 * @return True if the target actor is considered an ally; false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|OnHit")
	bool IsActorAnAlly(AActor* InHitActor) const;

	//-----------------------RICOCHET---------------------//

	/**
	 * Defines the targeting parameters for ricochet functionality.
	 * Configures how projectiles determine their ricochet targets and manage interactions after a ricochet.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Ricochet")
	TObjectPtr<UTargetingPreset> RicochetTargetingPreset;

	/**
	 * Triggered when the projectile ricochets off a surface.
	 * Allows custom handling or behavior to be implemented upon detecting a ricochet event.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Projectile|Ricochet")
	void OnProjectileRicochet();

	/**
	 * Stores a list of actors that have been hit by the projectile.
	 * Used to track targets impacted during ricochet or other interactions.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Ricochet")
	TArray<AActor*> HitTargets;

	//-----------------------DAMAGE---------------------//

	/**
	 * Applies damage to the target upon projectile impact.
	 * This function is called to assess and inflict damage to a target hit by the projectile.
	 *
	 * @return True if damage was successfully applied to the target; false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Damage")
	bool ApplyDamagetoTargetOnHit(AActor* InHitActor, const FHitResult& InHitResult);

	/**
	 * Represents the actor that was hit by the projectile.
	 * This is assigned when the projectile registers a hit event.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	AActor* HitTargetActor = nullptr;

	/**
	 * Stores information about the hit event for the projectile.
	 * This includes details such as the impact location, normal, and the actor that was hit.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	FHitResult HitResult;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Data")
	TObjectPtr<UGASC_ProjectileData> ProjectileDataAsset;
	
	UFUNCTION()
	FORCEINLINE void SetProjectileIndex(int32 InProjectileIndex)
	{
		ProjectileIndex = InProjectileIndex;
	}
	
	UFUNCTION()
	FORCEINLINE void SetProjectileGroupId(const FGuid& InProjectileGroupId)
	{
		ProjectileGroupId = InProjectileGroupId;
	}
	
	UFUNCTION()
	FORCEINLINE FGuid GetProjectileGroupId() const
	{
		return ProjectileGroupId;
	}

	/**
	 * Called by the pooling subsystem the moment this projectile is handed out, which is before
	 * activation - a projectile can be claimed and then discarded without ever being activated.
	 */
	UFUNCTION()
	FORCEINLINE void MarkClaimedFromPool()
	{
		bIsReturnedToPool = false;
	}

	/** Index of this projectile's slot in the pool, assigned once when it is created. */
	UFUNCTION()
	FORCEINLINE void SetPoolSlotIndex(int32 InPoolSlotIndex)
	{
		PoolSlotIndex = InPoolSlotIndex;
	}

	UFUNCTION()
	FORCEINLINE int32 GetPoolSlotIndex() const
	{
		return PoolSlotIndex;
	}

	UFUNCTION()
	FORCEINLINE bool IsReturnedToPool() const
	{
		return bIsReturnedToPool;
	}
	
	/**
	 * Hands this projectile back to the pool. Public because the spawn ability tasks return any
	 * projectile they claimed but never activated.
	 */
	UFUNCTION()
	void ReturnProjectileToPool();

protected:

	/**
	 * The pooling subsystem owns this actor's lifecycle - it creates it, assigns its slot, fills in
	 * its data and fragments, activates it and ticks it. That is the only code outside this class
	 * which needs the internals below, so it is granted access explicitly instead of the whole
	 * projectile being public. External callers previously reached straight into the fragments.
	 */
	friend class UGASC_ProjectilePoolingSubsystem;

	UFUNCTION()
	void InstantiateProjectileFromData();

	/**
	 * Empties every projectile event delegate. Bindings made by a UGASC_ProjectileEventListener
	 * (or Blueprint) are per-activation: because the actor is pooled and never destroyed, anything
	 * left bound would accumulate across every reuse and deliver this shot's events to listeners
	 * from previous ones. Called at the end of ReturnProjectileToPool, after the return broadcast.
	 */
	UFUNCTION()
	void ClearProjectileEventDelegates();

	UFUNCTION()
	void PromoteNewSnakeLeader();

	UFUNCTION()
	bool InstantiateProjectileVisualFragment();

	/** Assigns the mesh only when it differs, then activates. Shared by the sync and async paths. */
	void ApplyProjectileMesh(UStaticMesh* Mesh);

	/** Assigns the trail system only when it differs, then activates. Reinit is the expensive part. */
	void ApplyProjectileTrailVFX(UNiagaraSystem* VFX);

	/**
	 * Starts an async load of the impact and expire systems and holds the handle for this
	 * activation. Those two were the only assets still resolved with LoadSynchronous during
	 * gameplay - at the moment of impact - which meant a blocking disk read mid-frame.
	 */
	void PreloadProjectileImpactAssets();
	
	UFUNCTION()
	bool InstantiateProjectileCollisionFragment();
	
	UFUNCTION()
	bool InstantiateProjectileMovementFragment();
	
	UFUNCTION()
	bool InstantiateProjectileParabolicMovementFragment();
	
	UFUNCTION()
	bool InstantiateProjectileRicochetFragment();
	
	UFUNCTION()
	bool InstantiateProjectileDamageFragment();
	
	UFUNCTION()
	bool InstantiateProjectileHealingFragment();
	
	UFUNCTION()
	bool InstantiateProjectileHomingMovementFragment();
	
	UFUNCTION()
	bool InstantiateProjectileBezierHomingMovementFragment();

	/**
	 * Re-points homing at a new target mid-flight, used when a ricochet picks one. Activation is
	 * not enough on its own: the homing target component, the homing enable flags and - for bezier
	 * - the whole curve have to be rebuilt around the projectile's current position, otherwise the
	 * projectile keeps steering toward the target it just hit. No-op when this projectile was not
	 * activated with a homing fragment.
	 */
	UFUNCTION()
	void RetargetHomingToActor(AActor* NewTarget);

	/**
	 * Subscribes to the current TargetActor's death tag so homing can be cancelled when it dies.
	 * Shared by both homing fragment paths. Returns false only when the target is a character
	 * whose ability system component is missing, matching the callers' failure contract.
	 */
	UFUNCTION()
	bool RegisterTargetDeathCallback();

	/**
	 * Unregisters the death callback from the ability system component it was actually registered
	 * against, then clears the handle. Safe to call when nothing is registered.
	 */
	UFUNCTION()
	void UnregisterTargetDeathCallback();

	UFUNCTION()
	bool InstantiateProjectileGameplayEffectsFragment();
	
	UFUNCTION()
	bool InstantiateProjectileAreaOfEffectFragment();
	
	UFUNCTION()
	bool InstantiateProjectilePiercingFragment();
	
	UFUNCTION()
	bool InstantiateProjectileShapeSnakeFragment(const FProjectileSpawnShapeSnakeFragment& InSnakeShapeFragmentData);
	
	UFUNCTION()
	bool InstantiateProjectileOrbitRotationFragment();
	
	UFUNCTION()
	void TickProjectile(float DeltaTime);
	
	UFUNCTION()
	void TickTrailFollower(float DeltaTime);

	/**
	 * Discards the oldest snake trail samples the followers can no longer reach. The leader appends
	 * one location and rotation per frame, so without this the arrays grow for the whole lifetime
	 * of the shot and TickTrailFollower's arc-length walk gets steadily more expensive.
	 */
	UFUNCTION()
	void PruneSnakeTrail();
	
	UFUNCTION()
	void UpdateProjectileOrbitRotation(float DeltaTime);
	
	UFUNCTION()
	void ConstructDamagePipelineHitEvent(const FProjectileHitEvent& HitEvent, FGameplayEventData& OutEventData);
	
	UFUNCTION()
	void ApplyGameplayEffectOnProjectileEvent(EProjectileEventType EventType, const TWeakObjectPtr<AActor>& InEventTarget = nullptr);
	
	UFUNCTION()
	void ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType EventType, const TWeakObjectPtr<AActor>& InEventTarget = nullptr);
	
	template<typename T>
	bool FindProjectileFragment(T& OutProjectileFragment) const;

	/**
	 * Rebuilds FragmentLookup from ProjectileFragments. Called once per activation, before the
	 * fragments are read, so each FindProjectileFragment is a single map lookup instead of a
	 * linear scan over every stored fragment.
	 */
	UFUNCTION()
	void RebuildFragmentLookup();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement|Homing")
	FProjectileHomingMovementFragment ProjectileHomingMovementFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement|Homing")
	FProjectileHomingBezierMovementFragment ProjectileHomingBezierMovementFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Collision")
	FProjectileCollisionFragment ProjectileCollisionFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement")
	FProjectileMovementFragment ProjectileMovementFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement")
	FProjectileParabolicMovementFragment ProjectileParabolicMovementFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Visuals")
	FProjectileVisualsFragment ProjectileVisualsFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Ricochet")
	FProjectileRicochetFragment ProjectileRicochetFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|DamagePipeline|Damage")
	FProjectileDamageFragment ProjectileDamageFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|DamagePipeline|Healing")
	FProjectileHealingFragment ProjectileHealingFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|GameplayEffects")
	FProjectileGameplayEffectsFragment ProjectileGameplayEffectsFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|AreaOfEffect")
	FProjectileAOESpawnFragment ProjectileAreaOfEffectFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Piercing")
	FProjectilePiercingFragment ProjectilePiercingFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Shape|Snake")
	FProjectileSpawnShapeSnakeFragment ProjectileSpawnShapeSnakeFragment;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Shape|OrbitRotation")
	FProjectileOrbitingFragment ProjectileOrbitingFragment;
	
	UFUNCTION()
	void ApplyDamagePipelineToHitTarget(AActor* OtherActor, const FHitResult& InHitResult);

protected:
	
	/**
	 * Handles the behavior triggered when an overlap event begins.
	 * This method is invoked when another actor or component starts overlapping with the associated component or actor.
	 *
	 * @param OverlappedComponent The component that triggered the overlap event.
	 * @param OtherActor The actor that is overlapping with the component.
	 * @param OtherComp The specific component of the other actor involved in the overlap.
	 * @param OtherBodyIndex The index of the body that initiated the overlap.
	 * @param bFromSweep Indicates whether the overlap was the result of a sweep movement.
	 * @param SweepResult Contains detailed information about the hit result, if the overlap occurred due to a sweep.
	 */
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	/**
	 * Handles the completion of a targeting request for the projectile.
	 * Processes the targeting results to determine the next action for the projectile, including
	 * homing behavior, collision adjustments, or destruction.
	 *
	 * @param TargetingRequestHandle The handle associated with the completed targeting request, used to
	 * retrieve and manage the targeting results.
	 */
	UFUNCTION()
	void OnTargetRequestCompleted(FTargetingRequestHandle TargetingRequestHandle);

	/**
	 * Callback triggered when the target associated with the projectile dies.
	 * Handles cleaning up homing target references and updating projectile behavior accordingly.
	 *
	 * @param MatchingTag The gameplay tag associated with the event (e.g., death status).
	 * @param NewCount The count of how many times the MatchingTag is applied or updated.
	 */
	UFUNCTION()
	void OnTargetDeathCallback(FGameplayTag MatchingTag, int32 NewCount);
	
	UFUNCTION()
	void OnProjectileLifetimeExpired();
	
	UFUNCTION()
	void OnProjectileHit(AActor* OtherActor, const FHitResult& InHitResult);

private:
	
	UPROPERTY()
	int32 ProjectileIndex = -1;

	/**
	 * Slot this projectile owns in the pooling subsystem, fixed for its lifetime. Lets a return be
	 * an O(1) index lookup instead of searching the active list. INDEX_NONE until pooled.
	 */
	UPROPERTY()
	int32 PoolSlotIndex = INDEX_NONE;
	
	UPROPERTY()
	FGuid ProjectileGroupId;

	/**
	 * Struct type -> index into ProjectileFragments, covering each stored fragment's exact type
	 * and all of its ancestors. Registering ancestors too means a lookup for a base type resolves
	 * in one hop exactly as the old IsChildOf scan did, so a miss is a definitive "not present"
	 * and needs no fallback scan. Rebuilt per activation; keys are never GC'd type objects.
	 */
	TMap<const UStruct*, int32> FragmentLookup;

	/**
	 * Keeps the impact/expire systems resident for this activation. Soft pointers do not hold an
	 * asset loaded, so without retaining the streaming handle a preloaded system could be
	 * collected before the projectile actually hits. Released on return to the pool.
	 */
	TSharedPtr<FStreamableHandle> ProjectileImpactAssetHandle;
	
	UPROPERTY()
	FTargetingRequestHandle CurrentTargetHandle;

	UPROPERTY()
	TArray<AActor*> FoundTargets;

	FDelegateHandle OnTargetDeathDelegateHandle;

	/**
	 * The ability system component OnTargetDeathDelegateHandle was registered against. Tracked
	 * separately because a pooled projectile can be re-targeted, and the handle must be removed
	 * from the same component it was added to.
	 */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OnTargetDeathRegisteredASC = nullptr;
	
	/** 
	 * Handles the lifetime timer for the projectile, ensuring it is returned back into the pool after a specified duration.
	 */
	UPROPERTY()
	FTimerHandle ProjectileLifetimeTimer;
	
	UPROPERTY()
	FTimerHandle ProjectileHomingTimeoutTimer;
	
	UPROPERTY()
	FTimerHandle ProjectileHomingDOTCheckTimer;
	
	UFUNCTION()
	void CheckDOTToTargetHoming();
	
	UFUNCTION()
	void DisableProjectileHoming();
	
	UPROPERTY()
	int32 ProjectileRicochetCount = 0;
	
	UPROPERTY()
	bool bOrbitRotationEnabled = false;

	/**
	 * True whenever this projectile is sitting in the pool. Cleared when the subsystem hands it
	 * out, set again on the first line of ReturnProjectileToPool. Several independent paths can
	 * request a return in the same frame - multiple overlaps, the lifetime timer, a targeting
	 * callback, the orbit tick - and running the teardown more than once re-broadcasts
	 * OnProjectileReturnedToPoolDelegate, which corrupts the event listener's return count.
	 * Starts true because a freshly spawned projectile goes straight into the available list.
	 */
	UPROPERTY()
	bool bIsReturnedToPool = true;

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	
	virtual void PostInitializeComponents() override;
	
};
