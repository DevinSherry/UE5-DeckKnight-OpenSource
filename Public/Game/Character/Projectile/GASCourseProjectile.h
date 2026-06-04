// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "GASC_ProjectileData.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"
#include "GASCourseProjectile.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProjectileLifetimeExpired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileHit, FHitResult, HitResult);

class TargetingSystemTypes;

UCLASS()
class GASCOURSE_API AGASCourseProjectile : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	AGASCourseProjectile();

	/**
	 * Manages the movement of the projectile.
	 * Provides functionality for controlling the trajectory, speed, and behavior of the projectile as it moves through the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	class UGASCourseProjectileMovementComp* ProjectileMovementComp;

	/**
	 * Defines the collision behavior for the projectile.
	 * Serves as the collision representation, determining how the projectile interacts with objects in the game world upon contact.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* ProjectileCollisionComp;

	/**
	 * Represents the visual appearance of the projectile.
	 * Handles the static mesh component used to render the projectile's visuals within the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ProjectileVisualMeshComponent;

	/**
	 * Handles visual effects for the projectile using the Niagara particle system.
	 * Manages rendering and simulation of particle-based effects associated with the projectile.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* ProjectileNiagaraComponent;

	/**
	 * Handles the audio associated with the projectile.
	 * Facilitates playing, controlling, and managing sound effects specific to the projectile's behavior and interactions in the game world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* ProjectileAudioComponent;
	/**
	 * Represents the actor targeted by the projectile.
	 * Used to designate or track the intended target during the projectile's behavior or trajectory.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite,  Category = Projectile, meta = (ExposeOnSpawn=true))
	AActor* TargetActor = nullptr;

	//-----------------------ON HIT---------------------//
	
	/**
	 * Handles the event triggered when the projectile collides with another object.
	 * Provides details about the collision, including the components involved, the point of impact, and resulting physical reactions.
	 *
	 * @param HitComponent The component of the projectile that initiated the hit.
	 * @param OtherActor The actor that was hit by the projectile.
	 * @param OtherComp The specific component of the other actor that was hit.
	 * @param NormalImpulse The impulse applied at the point of impact.
	 * @param Hit Detailed information about the hit, including location, normal, and hit surface attributes.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|OnHit")
	void OnProjectileHitEvent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/**
	 * Processes a damage event caused by the projectile.
	 * This function is called to handle and apply damage when the projectile impacts a target.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|OnHit")
	void EventProcessDamageEvent(AActor* InHitActor, const FHitResult& InHitResult);

	/**
	 * Determines whether the target actor is an ally.
	 * This function is designed to ascertain if the actor hit by the projectile aligns with the same team or affiliation.
	 * @return True if the target actor is considered an ally; false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|OnHit")
	bool IsActorAnAlly(AActor* InHitActor) const;

	//-----------------------RICOCHET---------------------//

	/**
	 * Determines whether the projectile has the ability to ricochet upon impact.
	 * Enables behavior where the projectile can bounce off surfaces instead of stopping or being destroyed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Ricochet")
	bool bCanRicochet;

	/**
	 * Specifies the gameplay tag requirements for enabling ricochet behavior.
	 * Determines the conditions under which the projectile can bounce or deflect based on specified tags.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Ricochet")
	FGameplayTagQuery RicochetTagRequirements;

	/**
	 * Defines the targeting parameters for ricochet functionality.
	 * Configures how projectiles determine their ricochet targets and manage interactions after a ricochet.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Ricochet")
	UTargetingPreset* RicochetTargetingPreset;

	/**
	 * Triggered when the projectile ricochets off a surface.
	 * Allows custom handling or behavior to be implemented upon detecting a ricochet event.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Projectile|Ricochet")
	void OnProjectileRicochet();

	/**
	 * Determines whether the projectile can ricochet off surfaces upon impact.
	 * This method governs the behavior related to the projectile's ricochet capability.
	 *
	 * @return True if the projectile is allowed to ricochet, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Projectile|Ricochet")
	bool CanProjectileRicochet();

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
	 * Retrieves the damage value for the projectile from a specified curve table row.
	 * This function is used to dynamically calculate the projectile's damage based on predefined values in a curve table.
	 *
	 * @param CurveRow The handle to a row in the curve table specifying the damage values.
	 * @return The damage value retrieved from the specified curve table row.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Projectile|Damage")
	float GetProjectileDamage(FCurveTableRowHandle CurveRow) const;

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

	/**
	 * Specifies the row handle for retrieving damage values from a curve table.
	 * This handle is used to dynamically adjust projectile damage based on a predefined curve.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage", meta=(ExposeOnSpawn=true))
	FCurveTableRowHandle DamageCurveRow;
	
	/**
	 * Specifies the weapon level that affects the projectile's damage.
	 * This property allows the projectile to scale its damage based on the weapon's level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage", meta=(ExposeOnSpawn=true))
	int32 WeaponLevel = 1;

	/**
	 * Indicates whether the projectile can inflict damage on allied actors.
	 * This property determines if allied entities hit by the projectile should receive damage.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	bool bCanDamageAllies = false;

	//-----------------------HEALING---------------------//
	/**
	 * Applies healing to the target actor upon projectile impact.
	 * This function is invoked to provide a healing effect to the hit target
	 * based on the projectile's behavior and settings.
	 *
	 * @return True if healing was successfully applied to the target; false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Healing")
	bool ApplyHealingtoTargetOnHit(AActor* InHitActor, const FHitResult& InHitResult);

	/**
	 * Retrieves the healing value for the projectile from a specified curve table row.
	 * This function is used to dynamically adjust the projectile's healing effect based on predefined values in a curve table.
	 *
	 * @param CurveRow The handle to a row in the curve table specifying the healing values.
	 * @return The healing value retrieved from the specified curve table row.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Projectile|Healing")
	float GetProjectileHealing(FCurveTableRowHandle CurveRow) const;

	/**
	 * Specifies a handle to retrieve healing values from a curve table.
	 * This handle is used to dynamically adjust healing effects based on a predefined curve.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Healing")
	FCurveTableRowHandle HealingCurveRow;

	/**
	 * Determines whether the projectile has the ability to heal allied actors on impact.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Healing")
	bool bHealsAllies = false;

	/**
	 * Indicates whether the projectile is capable of healing enemy actors upon impact.
	 * When set to true, the projectile will apply healing effects to enemy targets it hits.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Healing")
	bool bCanHealEnemies = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Data")
	TObjectPtr<UGASC_ProjectileData> ProjectileDataAsset;
	
	UFUNCTION()
	void InstantiateProjectileFromData();
	
	UFUNCTION()
	void ReturnProjectileToPool();
	
	UFUNCTION()
	bool InstantiateProjectileVisualData(const FProjectileVisualData& InProjectileVisualData);
	
	UFUNCTION()
	bool InstantiateProjectileCollisionData(const FProjectileCollisionData& InProjectileCollisionData);
	
	UFUNCTION()
	bool InstantiateProjectileMovementData(const FProjectileMovementData& InProjectileMovementData);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Movement|Homing")
	FProjectileHomingMovementData ProjectileHomingMovementData;
	
	UFUNCTION()
	bool InstantiateProjectileHomingMovementData(const FProjectileHomingMovementData& InProjectileHomingMovementData);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|DamagePipeline")
	FProjectileDamagePipelineData ProjectileDamagePipelineData;
	
	UFUNCTION()
	void ApplyDamagePipelineToHitTarget(const FHitResult& InHitResult) const;

protected:
	/**
	 * Handles the event triggered when the projectile collides with another object.
	 * Processes the collision, potentially disables further collisions, manages hit events,
	 * and initiates ricochet logic if enabled.
	 *
	 * @param HitComponent The component on the projectile that triggered the hit event.
	 * @param OtherActor The actor that was hit by the projectile.
	 * @param OtherComp The specific component of the hit actor that was collided with.
	 * @param NormalImpulse The impulse generated as a result of the collision.
	 * @param Hit Detailed information about the hit, including location and surface properties.
	 */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

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
	void OnProjectileHit(const FHitResult& InHitResult);

private:
	
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
	FProjectileLifetimeExpired OnProjectileLifetimeExpiredDelegate;

	UPROPERTY()
	FOnProjectileHit OnProjectileHitDelegate;
	
	UPROPERTY()
	FTimerHandle ProjectileHomingTimeoutTimer;
	
	UPROPERTY()
	FTimerHandle ProjectileHomingDOTCheckTimer;
	
	UFUNCTION()
	void CheckDOTToTargetHoming();
	
	UFUNCTION()
	void DisableProjectileHoming();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
};
