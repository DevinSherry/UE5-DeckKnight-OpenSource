// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Projectile/AbilityTask/GASC_AT_SpawnProjectilesAsync.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/Projectile/GASCourseProjectile.h"
#include "Game/Projectile/GASC_ProjectileEventListener.h"

UGASC_AT_SpawnProjectilesInShape* UGASC_AT_SpawnProjectilesInShape::SpawnProjectilesInShapeWithDelay(
    UGameplayAbility* OwningAbility,
    AActor* Instigator,
    FInstancedStruct Shape,
    int32 SpawnCount,
    float SpawnDelayBetween,
    UGASC_ProjectileData* ProjectileData,
    TSubclassOf<UGASC_ProjectileEventListener> EventListener,
    const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
    UGASC_AT_SpawnProjectilesInShape* Task =
        NewAbilityTask<UGASC_AT_SpawnProjectilesInShape>(OwningAbility);

    Task->Instigator = Instigator;
    Task->Shape = Shape;
    Task->SpawnCount = SpawnCount;
    Task->ProjectileData = ProjectileData;
	Task->SpawnDelayBetween = SpawnDelayBetween;
    Task->EventListener = EventListener;
    Task->AdditionalProjectileFragments = AdditionalProjectileFragments;

    return Task;
}

void UGASC_AT_SpawnProjectilesInShape::Activate()
{
    if (!Ability || !Instigator || !ProjectileData || SpawnCount <= 0)
    {
        FinishTask();
        return;
    }
	SpawnedProjectilesStruct = FGASCSpawnedProjectiles();

    UWorld* World = GetWorld();
    if (!World)
    {
        FinishTask();
        return;
    }

    UGASC_ProjectilePoolingSubsystem* PoolingSubsystem =
        World->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();

    if (!PoolingSubsystem)
    {
        FinishTask();
        return;
    }

    SpawnTransforms.Reserve(SpawnCount);
	SpawnedProjectiles.Reserve(SpawnCount);

    PoolingSubsystem->ConstructShapeSpawnTransforms(
        Shape,
        Instigator->GetActorLocation(),
        Instigator,
        SpawnTransforms,
        SpawnCount
    );
	
	SpawnedProjectiles = PoolingSubsystem->ClaimProjectilesFromPool(SpawnCount);
	
	if (EventListener)
	{
		UGASC_ProjectileEventListener* NewEventListener = NewObject<UGASC_ProjectileEventListener>(Instigator, EventListener);
		NewEventListener->Projectiles = SpawnedProjectiles;
		NewEventListener->OnListenerConstructed(Instigator);
	}

    SpawnNextProjectileInShape();
}

void UGASC_AT_SpawnProjectilesInShape::SpawnNextProjectileInShape()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishTask();
		return;
	}

	if (!Instigator || CurrentIndex >= SpawnTransforms.Num())
	{
		FinishTask();
		return;
	}

	UGASC_ProjectilePoolingSubsystem* PoolingSubsystem =
		World->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();

	if (!PoolingSubsystem)
	{
		FinishTask();
		return;
	}
	
	bool bIsSnakeShape = false;
	bool bStopTrailAfterLeaderEnd = false;
	if (const FProjectileSpawnShapeSnakeFragment* CurrentFragment = Shape.GetPtr<FProjectileSpawnShapeSnakeFragment>())
	{
		bIsSnakeShape = true;
		bStopTrailAfterLeaderEnd = CurrentFragment->bStopTrailAfterLeaderEnd;
	}
	
	AGASCourseProjectile* Projectile = SpawnedProjectiles[CurrentIndex];
	if (!Projectile)
		return;
	
	if (bIsSnakeShape)
	{
		PoolingSubsystem->AssignSnakeFragment(SpawnedProjectiles, CurrentIndex,
			SpawnTransforms[CurrentIndex], bStopTrailAfterLeaderEnd, SpawnDelayBetween);
	}


	PoolingSubsystem->ActivateProjectileFromPool_Internal(Projectile, Instigator,
			SpawnTransforms[CurrentIndex],
			ProjectileData,
			EventListener,
			AdditionalProjectileFragments);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		SpawnedProjectilesStruct.LastSpawnedProjectile = Projectile;
		OnSingleProjectileSpawned.Broadcast(SpawnedProjectilesStruct);
	}

    ++CurrentIndex;

    if (CurrentIndex >= SpawnTransforms.Num())
    {
        FinishTask();
        return;
    }

    if (SpawnDelayBetween <= 0.f)
    {
        SpawnNextProjectileInShape();
        return;
    }

    World->GetTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &UGASC_AT_SpawnProjectilesInShape::SpawnNextProjectileInShape,
        SpawnDelayBetween,
        false
    );
}

void UGASC_AT_SpawnProjectilesInShape::FinishTask()
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
    	SpawnedProjectilesStruct.Projectiles = SpawnedProjectiles;
		OnAllProjectilesSpawned.Broadcast(SpawnedProjectilesStruct);
    }

    EndTask();
}

void UGASC_AT_SpawnProjectilesInShape::OnDestroy(bool bInOwnerFinished)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SpawnTimerHandle);
    }

	ReturnUnactivatedProjectiles();

	SpawnedProjectilesStruct = FGASCSpawnedProjectiles();
	SpawnedProjectiles.Empty();

    Super::OnDestroy(bInOwnerFinished);
}

void UGASC_AT_SpawnProjectilesInShape::ReturnUnactivatedProjectiles()
{
	// CurrentIndex is incremented only after a projectile has been activated, so everything from
	// it onward is still holding a pool slot it never used.
	for (int32 i = CurrentIndex; i < SpawnedProjectiles.Num(); ++i)
	{
		if (AGASCourseProjectile* Projectile = SpawnedProjectiles[i])
		{
			if (IsValid(Projectile))
			{
				Projectile->ReturnProjectileToPool();
			}
		}
	}

	CurrentIndex = SpawnedProjectiles.Num();
}

//------------------------------------------------------------------------------------- 


UGASC_AT_SpawnProjectilesFromTransform* UGASC_AT_SpawnProjectilesFromTransform::SpawnProjectilesFromTransformWithDelay(UGameplayAbility* OwningAbility,
	AActor* Instigator, int32 SpawnCount, bool bUseInstigatorTransform, FTransform CustomSpawnTransform, float SpawnDelayBetween, UGASC_ProjectileData* ProjectileData,
	TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
	UGASC_AT_SpawnProjectilesFromTransform* Task =
	NewAbilityTask<UGASC_AT_SpawnProjectilesFromTransform>(OwningAbility);

	Task->Instigator = Instigator;
	Task->SpawnCount = SpawnCount;
	Task->bInstigatorTransform = bUseInstigatorTransform;
	Task->SpawnDelayBetween = SpawnDelayBetween;
	Task->CustomSpawnTransform = CustomSpawnTransform;
	Task->ProjectileData = ProjectileData;
	Task->EventListener = EventListener;
	Task->AdditionalProjectileFragments = AdditionalProjectileFragments;

	return Task;
}

void UGASC_AT_SpawnProjectilesFromTransform::Activate()
{
	if (!Ability || !Instigator || !ProjectileData || SpawnCount <= 0)
	{
		FinishTask();
		return;
	}
	SpawnedProjectilesStruct = FGASCSpawnedProjectiles();

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishTask();
		return;
	}

	UGASC_ProjectilePoolingSubsystem* PoolingSubsystem =
		World->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();

	if (!PoolingSubsystem)
	{
		FinishTask();
		return;
	}
	
	FTransform SpawnTransform = bInstigatorTransform ? Instigator->GetActorTransform() : CustomSpawnTransform;

	SpawnTransforms.Reserve(SpawnCount);
	SpawnedProjectiles.Reserve(SpawnCount);
	SpawnTransforms.Init(SpawnTransform, SpawnCount);
	
	SpawnedProjectiles = PoolingSubsystem->ClaimProjectilesFromPool(SpawnCount);
	
	if (EventListener)
	{
		UGASC_ProjectileEventListener* NewEventListener = NewObject<UGASC_ProjectileEventListener>(Instigator, EventListener);
		NewEventListener->Projectiles = SpawnedProjectiles;
		NewEventListener->OnListenerConstructed(Instigator);
	}

	SpawnNextProjectileFromTransform();
}

void UGASC_AT_SpawnProjectilesFromTransform::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	ReturnUnactivatedProjectiles();

	SpawnedProjectilesStruct = FGASCSpawnedProjectiles();
	SpawnedProjectiles.Empty();

	Super::OnDestroy(bInOwnerFinished);
}

void UGASC_AT_SpawnProjectilesFromTransform::ReturnUnactivatedProjectiles()
{
	// CurrentIndex is incremented only after a projectile has been activated, so everything from
	// it onward is still holding a pool slot it never used.
	for (int32 i = CurrentIndex; i < SpawnedProjectiles.Num(); ++i)
	{
		if (AGASCourseProjectile* Projectile = SpawnedProjectiles[i])
		{
			if (IsValid(Projectile))
			{
				Projectile->ReturnProjectileToPool();
			}
		}
	}

	CurrentIndex = SpawnedProjectiles.Num();
}

void UGASC_AT_SpawnProjectilesFromTransform::SpawnNextProjectileFromTransform()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishTask();
		return;
	}

	if (!Instigator || CurrentIndex >= SpawnTransforms.Num())
	{
		FinishTask();
		return;
	}

	UGASC_ProjectilePoolingSubsystem* PoolingSubsystem =
		World->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();

	if (!PoolingSubsystem)
	{
		FinishTask();
		return;
	}
	
	AGASCourseProjectile* Projectile = SpawnedProjectiles[CurrentIndex];
	if (!Projectile)
		return;
	
	PoolingSubsystem->ActivateProjectileFromPool_Internal(Projectile, Instigator,
			SpawnTransforms[CurrentIndex],
			ProjectileData,
			EventListener,
			AdditionalProjectileFragments);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		SpawnedProjectilesStruct.LastSpawnedProjectile = Projectile;
		OnSingleProjectileSpawned.Broadcast(SpawnedProjectilesStruct);
	}

    ++CurrentIndex;

    if (CurrentIndex >= SpawnTransforms.Num())
    {
        FinishTask();
        return;
    }

    if (SpawnDelayBetween <= 0.f)
    {
        SpawnNextProjectileFromTransform();
        return;
    }

    World->GetTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &UGASC_AT_SpawnProjectilesFromTransform::SpawnNextProjectileFromTransform,
        SpawnDelayBetween,
        false
    );
}

void UGASC_AT_SpawnProjectilesFromTransform::FinishTask()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		SpawnedProjectilesStruct.Projectiles = SpawnedProjectiles;
		OnAllProjectilesSpawned.Broadcast(SpawnedProjectilesStruct);
	}

	EndTask();
}
