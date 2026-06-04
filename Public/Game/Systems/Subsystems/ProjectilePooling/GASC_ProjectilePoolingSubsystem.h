// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Character/Projectile/GASCourseProjectile.h"
#include "Subsystems/WorldSubsystem.h"
#include "GASC_ProjectilePoolingSubsystem.generated.h"

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
	
public:

	virtual void Tick(float DeltaTime) override;

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	virtual TStatId GetStatId() const override
	{
		return GetStatID();
	}

public:
	
	UFUNCTION(BlueprintCallable)
	AGASCourseProjectile* GetAvailableProjectile();
	
	UFUNCTION(BlueprintCallable)
	void ReturnProjectileToPool(AGASCourseProjectile* Projectile);
	
	UFUNCTION(BlueprintCallable)
	void AddProjectilesToPool(int32 Count = 1);
	
	UFUNCTION()
	AGASCourseProjectile* AddProjectileToPool_Internal();
	
	UFUNCTION(BlueprintCallable)
	AGASCourseProjectile* SpawnAndClaimProjectile(AActor* Owner, APawn* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, const FProjectileDamagePipelineData DamagePipelineData);
	
	UFUNCTION(BlueprintCallable)
	AGASCourseProjectile* SpawnAndClaimProjectile_Homing(AActor* Owner, APawn* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, const FProjectileDamagePipelineData DamagePipelineData, const FProjectileHomingMovementData HomingMovementData);
	
protected:
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> ProjectilePool;
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> ActiveProjectilePool;
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> AvailableProjectilePool;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Projectile|Pooling")
	TSubclassOf<AGASCourseProjectile> ProjectileClass = AGASCourseProjectile::StaticClass();;
};
