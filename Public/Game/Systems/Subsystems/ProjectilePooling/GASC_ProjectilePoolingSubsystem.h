// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Projectile/GASC_ProjectileData.h"
#include "Game/Projectile/GASC_ProjectileEventListener.h"
#include "Subsystems/WorldSubsystem.h"
#include "GASC_ProjectilePoolingSubsystem.generated.h"

struct FProjectileSpawnShapeBaseFragment;
struct FInstancedStruct;
class AGASCourseProjectile;

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
	
	UFUNCTION()
	AGASCourseProjectile* AddProjectileToPool_Internal();
	
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "AdditionalProjectileFragments"))
	AGASCourseProjectile* SpawnAndClaimProjectile(AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments);
	
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "AdditionalProjectileFragments"))
	TArray<AGASCourseProjectile*> SpawnAndClaimProjectilesInShape(AActor* Instigator, const FInstancedStruct Shape,
		int32 SpawnCount, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener,
		const TArray<FInstancedStruct>& AdditionalProjectileFragments);
	
	UFUNCTION()
	AGASCourseProjectile* ClaimProjectileFromPool_Internal();
	
	UFUNCTION()
	TArray<AGASCourseProjectile*> ClaimProjectilesFromPool(int32 Count);
	
	UFUNCTION()
	void ActivateProjectileFromPool_Internal(AGASCourseProjectile* ProjectileToActivate, AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments);
	
	UFUNCTION()
	void ConstructShapeSpawnTransforms(const FInstancedStruct& ShapeFragment, const FVector& SpawnOrigin, const TWeakObjectPtr<AActor> Instigator, TArray<FTransform>& OutSpawnTransforms, const int32 Count = 1);
	
	UFUNCTION(BlueprintCallable)
	int32 GetNumberOfAvailableProjectilesInPool() const
	{
		return AvailableProjectilePool.Num();
	}
	
	UFUNCTION()
	FProjectileGroup& GetProjectileGroup(const FGuid GroupId)
	{
		return *ProjectileGroupMap.Find(GroupId);
	}
	
protected:
	
	UPROPERTY()
	TSet<TObjectPtr<AGASCourseProjectile>> ProjectilePool;
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> ActiveProjectilePool;
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> AvailableProjectilePool;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Projectile|Pooling")
	TSubclassOf<AGASCourseProjectile> ProjectileClass;
	
	UPROPERTY()
	TMap<FGuid, FProjectileGroup> ProjectileGroupMap;
};
