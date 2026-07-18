// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "GASC_ProjectileData.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"
#include "GASCourseProjectile.generated.h"

class URotatingMovementComponent;
struct FGameplayEventData;
class UNiagaraComponent;
class USphereComponent;
class UGASCourseProjectileMovementComp;
class UTargetingPreset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProjectileLifetimeExpired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHit, AActor*, OtherActor, FHitResult, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProjectileCreated, const AActor*, InstigatorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileRicochet, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectilePierce, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileReturnedToPool, AActor*, Projectile);

class TargetingSystemTypes;

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
	TObjectPtr<USphereComponent> ProjectileCollisionComp;

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
	
	UFUNCTION()
	void InstantiateProjectileFromData();
	
	UFUNCTION()
	void ReturnProjectileToPool();
	
	UFUNCTION()
	void PromoteNewSnakeLeader();
	
	UFUNCTION()
	bool InstantiateProjectileVisualFragment();
	
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
	bool InstantiateProjectileGameplayEffectsFragment();
	
	UFUNCTION()
	bool InstantiateProjectileAreaOfEffectFragment();
	
	UFUNCTION()
	bool InstantiateProjectilePiercingFragment();
	
	UFUNCTION()
	bool InstantiateProjectileShapeSnakeFragment(const FProjectileSpawnShapeSnakeFragment& InSnakeShapeFragmentData);
	
	UFUNCTION()
	bool InstantiateProjectileOrbitRotationFragment();
	
	static int32 FindClosestTrailIndex(const TArray<FVector>& TrailLocations, const FVector& Location);
	
	UFUNCTION()
	void TickProjectile(float DeltaTime);
	
	UFUNCTION()
	void TickTrailFollower(float DeltaTime);
	
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement|Homing")
	FProjectileHomingMovementFragment ProjectileHomingMovementFragment;
	
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
	
	UPROPERTY()
	FProjectileGroup ProjectileGroup;
	
	UPROPERTY()
	FGuid ProjectileGroupId;
	
	UPROPERTY()
	FTargetingRequestHandle CurrentTargetHandle;

	UPROPERTY()
	TArray<AActor*> FoundTargets;

	FDelegateHandle OnTargetDeathDelegateHandle;
	
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

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	
	virtual void PostInitializeComponents() override;
	
};
