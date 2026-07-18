// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "StructUtils/InstancedStruct.h"
#include "GASC_AT_SpawnProjectilesAsync.generated.h"

class AGASCourseProjectile;
class UGASC_ProjectileData;
class UGASC_ProjectileEventListener;

USTRUCT(BlueprintType)
struct FGASCSpawnedProjectiles
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AGASCourseProjectile>> Projectiles;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGASCourseProjectile> LastSpawnedProjectile;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileSpawned, FGASCSpawnedProjectiles, Projectile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllProjectilesSpawned, FGASCSpawnedProjectiles, AllProjectiles);

/**
 * @class UGASC_AT_SpawnProjectilesInShape
 *
 * @brief A class responsible for spawning projectiles in a specified geometric shape.
 *
 * This class provides functionality to spawn a number of projectiles at runtime, distributed
 * based on a defined shape or pattern. It is typically used in gameplay abilities to create
 * area-of-effect attacks or patterns of projectiles.
 *
 * The shape and parameters of the projectile distribution can be customized to meet specific
 * gameplay requirements. The generated projectiles can have configurable attributes such as
 * speed, damage, and lifespan.
 *
 * Features:
 * - Supports various geometric shapes for projectile spawning.
 * - Configurable number and spacing of projectiles.
 * - Integration with gameplay abilities and effects.
 *
 * Use cases include creating radial projectile patterns, grid-based attack spreads, or custom
 * projectile layouts.
 */
UCLASS()
class GASCOURSE_API UGASC_AT_SpawnProjectilesInShape : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnAllProjectilesSpawned OnAllProjectilesSpawned;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectileSpawned OnSingleProjectileSpawned;
	
	UFUNCTION(BlueprintCallable, meta = (
	DisplayName = "Spawn Projectiles In Shape With Delay",
	HidePin = "OwningAbility",
	DefaultToSelf = "OwningAbility",
	BlueprintInternalUseOnly = "true",
	AutoCreateRefTerm = "AdditionalProjectileFragments"
))
	static UGASC_AT_SpawnProjectilesInShape* SpawnProjectilesInShapeWithDelay(
		UGameplayAbility* OwningAbility,
		AActor* Instigator,
		FInstancedStruct Shape,
		int32 SpawnCount,
		float SpawnDelayBetween,
		UGASC_ProjectileData* ProjectileData,
		TSubclassOf<UGASC_ProjectileEventListener> EventListener,
		const TArray<FInstancedStruct>& AdditionalProjectileFragments
	);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UPROPERTY()
	AActor* Instigator;

	UPROPERTY()
	UGASC_ProjectileData* ProjectileData;

	UPROPERTY()
	TSubclassOf<UGASC_ProjectileEventListener> EventListener;
	
	UPROPERTY()
	TArray<AGASCourseProjectile*> SpawnedProjectiles;

	FInstancedStruct Shape;
	TArray<FInstancedStruct> AdditionalProjectileFragments;
	TArray<FTransform> SpawnTransforms;
	
	UPROPERTY()
	FGASCSpawnedProjectiles SpawnedProjectilesStruct;

	int32 SpawnCount = 0;
	int32 CurrentIndex = 0;
	float SpawnDelayBetween = 0.f;

	FTimerHandle SpawnTimerHandle;

	void SpawnNextProjectileInShape();
	void FinishTask();
};

UCLASS()
class GASCOURSE_API UGASC_AT_SpawnProjectilesFromTransform : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnAllProjectilesSpawned OnAllProjectilesSpawned;
	
	UPROPERTY(BlueprintAssignable)
	FOnProjectileSpawned OnSingleProjectileSpawned;
	
	UFUNCTION(BlueprintCallable, meta = (
	DisplayName = "Spawn Projectiles From Transform With Delay",
	HidePin = "OwningAbility",
	DefaultToSelf = "OwningAbility",
	BlueprintInternalUseOnly = "true",
	AutoCreateRefTerm = "AdditionalProjectileFragments"
))
	static UGASC_AT_SpawnProjectilesFromTransform* SpawnProjectilesFromTransformWithDelay(
		UGameplayAbility* OwningAbility,
		AActor* Instigator,
		int32 SpawnCount,
		bool bUseInstigatorTransform,
		FTransform InCustomSpawnTransform,
		float SpawnDelayBetween,
		UGASC_ProjectileData* ProjectileData,
		TSubclassOf<UGASC_ProjectileEventListener> EventListener,
		const TArray<FInstancedStruct>& AdditionalProjectileFragments
	);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UPROPERTY()
	AActor* Instigator;

	UPROPERTY()
	UGASC_ProjectileData* ProjectileData;

	UPROPERTY()
	TSubclassOf<UGASC_ProjectileEventListener> EventListener;
	
	UPROPERTY()
	TArray<AGASCourseProjectile*> SpawnedProjectiles;
	
	TArray<FInstancedStruct> AdditionalProjectileFragments;
	TArray<FTransform> SpawnTransforms;
	
	UPROPERTY()
	FGASCSpawnedProjectiles SpawnedProjectilesStruct;

	int32 SpawnCount = 0;
	int32 CurrentIndex = 0;
	float SpawnDelayBetween = 0.f;
	FTransform CustomSpawnTransform;
	bool bInstigatorTransform = false;

	FTimerHandle SpawnTimerHandle;

	void SpawnNextProjectileFromTransform();
	void FinishTask();
};
