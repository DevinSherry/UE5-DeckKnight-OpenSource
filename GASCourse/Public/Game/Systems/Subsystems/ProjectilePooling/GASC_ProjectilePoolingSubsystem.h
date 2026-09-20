// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Projectile/GASC_ProjectileData.h"
#include "Game/Projectile/GASC_ProjectileEventListener.h"
#include "Subsystems/WorldSubsystem.h"
#include "GASC_ProjectilePoolingSubsystem.generated.h"

struct FProjectileSpawnShapeBaseFragment;
struct FInstancedStruct;
class AGASCourseProjectile;
class UTargetingPreset;

/** Whether a pool slot's projectile is sitting in the pool or has been handed out. */
UENUM()
enum class EProjectilePoolSlotState : uint8
{
	Available,
	Claimed
};

/** One entry in the pool. The projectile's PoolSlotIndex is this entry's index. */
USTRUCT()
struct FPooledProjectileSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AGASCourseProjectile> Projectile = nullptr;

	UPROPERTY()
	EProjectilePoolSlotState State = EProjectilePoolSlotState::Available;
};

/**
 * @class UGASC_ProjectilePoolingSubsystem
 * @brief A subsystem responsible for pooling and managing reusable projectile instances.
 *
 * This class implements a pooling mechanism to optimize the creation and reuse of
 * projectile objects in a game. By maintaining a pool of pre-instantiated, inactive
 * projectiles, it helps improve performance and reduce runtime memory allocation overhead
 * during gameplay.
 *
 * The subsystem is typically used in games with frequent projectile usage, such as
 * shooting or combat systems, where new projectiles are spawned constantly. Instead of
 * creating and destroying projectiles dynamically, this subsystem retrieves projectiles
 * from the pool when needed and recycles them after they are no longer in use.
 *
 * Features include:
 * - Initial pre-allocation of a configurable number of projectile instances.
 * - Dynamic growth of the pool if no projectiles are available.
 * - Efficient tracking of active and inactive projectiles.
 * - Mechanisms to reset, initialize, and return projectiles to the pool.
 * - Integration with Unreal Engine's subsystem to ensure efficient lifecycle management.
 */
UCLASS()
class GASCOURSE_API UGASC_ProjectilePoolingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
	UGASC_ProjectilePoolingSubsystem();
	
public:

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION(BlueprintCallable)
	AGASCourseProjectile* GetAvailableProjectile();
	
	UFUNCTION(BlueprintCallable)
	void ReturnProjectileToPool(AGASCourseProjectile* Projectile);
	
	UFUNCTION(BlueprintCallable)
	void AddProjectilesToPool(int32 Count = 1);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "AdditionalProjectileFragments"))
	AGASCourseProjectile* SpawnAndClaimProjectile(AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments);
	
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "AdditionalProjectileFragments"))
	TArray<AGASCourseProjectile*> SpawnAndClaimProjectilesInShape(AActor* Instigator, const FInstancedStruct Shape,
		int32 SpawnCount, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener,
		const TArray<FInstancedStruct>& AdditionalProjectileFragments);
	
	UFUNCTION()
	TArray<AGASCourseProjectile*> ClaimProjectilesFromPool(int32 Count);

	UFUNCTION()
	void ActivateProjectileFromPool_Internal(AGASCourseProjectile* ProjectileToActivate, AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments);

	UFUNCTION()
	void ConstructShapeSpawnTransforms(const FInstancedStruct& ShapeFragment, const FVector& SpawnOrigin, const TWeakObjectPtr<AActor> Instigator, TArray<FTransform>& OutSpawnTransforms, const int32 Count = 1);

	/**
	 * Builds and applies one projectile's snake formation fragment from the full batch. Shared by
	 * the subsystem's shape spawn and the spawn ability tasks, which previously each carried their
	 * own drifted copy of this logic. Index is the projectile's position in SnakeProjectiles;
	 * element 0 is the leader. Not a UFUNCTION - C++ callers only.
	 */
	void AssignSnakeFragment(const TArray<AGASCourseProjectile*>& SnakeProjectiles, int32 Index,
		const FTransform& SpawnTransform, bool bStopTrailAfterLeaderEnd, float SpawnDelayBetween);

	UFUNCTION(BlueprintCallable)
	int32 GetNumberOfAvailableProjectilesInPool() const
	{
		return AvailableSlots.Num();
	}

	/** The ricochet preset from project settings, resolved once at OnWorldBeginPlay. */
	UFUNCTION()
	FORCEINLINE UTargetingPreset* GetRicochetTargetingPreset() const
	{
		return RicochetTargetingPreset;
	}
	
	/**
	 * Returns null when the group does not exist. Projectiles spawned via SpawnAndClaimProjectile
	 * never get a group, and a group is erased once its last projectile returns, so a miss is
	 * routine rather than exceptional - the previous version dereferenced Find() directly and
	 * crashed on both cases. Not a UFUNCTION: struct pointers are not a valid reflected return.
	 */
	FProjectileGroup* GetProjectileGroup(const FGuid GroupId)
	{
		return ProjectileGroupMap.Find(GroupId);
	}
	
protected:

	/** Spawns one projectile into a fresh slot and returns it. Available slot, not yet claimed. */
	AGASCourseProjectile* AddProjectileToPool_Internal();

	/**
	 * One slot per pooled projectile, indexed by the projectile's own PoolSlotIndex. Replaces the
	 * previous trio of a TSet of everything plus separate active and available arrays, which were
	 * kept in step by hand via Contains checks at every call site - the shape that let claimed
	 * projectiles go missing when an ability was cancelled. State here is the single source of
	 * truth, so a double return or a double claim cannot be expressed.
	 */
	UPROPERTY()
	TArray<FPooledProjectileSlot> PooledProjectiles;

	/** Free list of indices into PooledProjectiles, used as a stack. */
	UPROPERTY()
	TArray<int32> AvailableSlots;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Projectile|Pooling")
	TSubclassOf<AGASCourseProjectile> ProjectileClass;
	
	UPROPERTY()
	TMap<FGuid, FProjectileGroup> ProjectileGroupMap;

	UPROPERTY(Transient)
	TObjectPtr<UTargetingPreset> RicochetTargetingPreset;
};
