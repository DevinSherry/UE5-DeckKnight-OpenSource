// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/Character/Projectile/Components/GASCourseProjectileMovementComp.h"

void UGASC_ProjectilePoolingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UGASC_ProjectilePoolingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGASC_ProjectilePoolingSubsystem::Deinitialize()
{
	for (AGASCourseProjectile* Projectile : ProjectilePool)
	{
		if (IsValid(Projectile))
		{
			Projectile->Destroy();
		}
	}

	ProjectilePool.Empty();
	AvailableProjectilePool.Empty();
	ActiveProjectilePool.Empty();

	Super::Deinitialize();
}

bool UGASC_ProjectilePoolingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* OuterWorld = Cast<UWorld>(Outer);
	return (OuterWorld && Super::ShouldCreateSubsystem(Outer));
}

void UGASC_ProjectilePoolingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::GetAvailableProjectile()
{
	while (AvailableProjectilePool.Num() > 0)
	{
		AGASCourseProjectile* AvailableProjectile = AvailableProjectilePool.Pop(EAllowShrinking::No);
		if (IsValid(AvailableProjectile))
		{
			ActiveProjectilePool.Push(AvailableProjectile);
			return AvailableProjectile;
		}
	}
			
	if (!IsValid(ProjectileClass))
		return nullptr;
		
	return AddProjectileToPool_Internal();
}

void UGASC_ProjectilePoolingSubsystem::ReturnProjectileToPool(AGASCourseProjectile* Projectile)
{
	if (!IsValid(Projectile))
		return;
	
	Projectile->SetActorHiddenInGame(true);
	Projectile->SetActorTickEnabled(false);
	Projectile->SetActorEnableCollision(false);
	
	//Any custom function from projectile to push here.
	AvailableProjectilePool.Push(Projectile);
	ActiveProjectilePool.Remove(Projectile);
}

void UGASC_ProjectilePoolingSubsystem::AddProjectilesToPool(int32 Count)
{
	if (!IsValid(ProjectileClass))
		return;
	
	for (int32 i = 0; i < Count; ++i)
	{
		AddProjectileToPool_Internal();
	}
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::AddProjectileToPool_Internal()
{
	SCOPED_NAMED_EVENT(AddProjectileToPool_Internal, FColor::Green)
	AGASCourseProjectile* NewProjectile = GetWorld()->SpawnActorDeferred<AGASCourseProjectile>(
	ProjectileClass, FTransform::Identity, nullptr, nullptr,
	ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(NewProjectile))
		return nullptr;
		
	NewProjectile->SetActorHiddenInGame(true);
	NewProjectile->SetActorTickEnabled(false);
	NewProjectile->SetActorEnableCollision(false);
	
	if (UGASCourseProjectileMovementComp* MovementComp = NewProjectile->FindComponentByClass<UGASCourseProjectileMovementComp>())
	{
		MovementComp->StopMovementImmediately();
		MovementComp->Velocity = FVector::ZeroVector;
		MovementComp->bIsHomingProjectile = false;
		MovementComp->HomingTargetComponent = nullptr;
		MovementComp->Deactivate();
	}
	
	ProjectilePool.Add(NewProjectile);
	NewProjectile->FinishSpawning(FTransform::Identity);
	
	return NewProjectile;
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::SpawnAndClaimProjectile(AActor* Owner, APawn* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, const FProjectileDamagePipelineData DamagePipelineData)
{
	AGASCourseProjectile* NewProjectile = GetAvailableProjectile();
	if (!IsValid(NewProjectile))
		return nullptr;
		
	NewProjectile->SetOwner(Owner);
	NewProjectile->SetInstigator(Instigator);
	NewProjectile->SetActorTransform(SpawnTransform);
	NewProjectile->ProjectileDataAsset = InProjectileData;
	NewProjectile->ProjectileDamagePipelineData = DamagePipelineData;
	NewProjectile->SetActorHiddenInGame(false);
	NewProjectile->InstantiateProjectileFromData();
	
	
	ActiveProjectilePool.Push(NewProjectile);
	AvailableProjectilePool.Remove(NewProjectile);
	
	return NewProjectile;
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::SpawnAndClaimProjectile_Homing(AActor* Owner, APawn* Instigator,
	const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, const FProjectileDamagePipelineData DamagePipelineData, const FProjectileHomingMovementData HomingMovementData)
{
	AGASCourseProjectile* NewProjectile = GetAvailableProjectile();

	if (!IsValid(NewProjectile))
		return nullptr;
	
	/** 
	 * Instantiate important spawn data first, and make projectile hidden by default.
	 */
	NewProjectile->SetActorHiddenInGame(false);
	NewProjectile->SetOwner(Owner);
	NewProjectile->SetInstigator(Instigator);
	NewProjectile->SetActorTransform(SpawnTransform);
	
	/**
	 * Update data to be used when constructing the projectile
	 */
	NewProjectile->ProjectileDataAsset = InProjectileData;
	NewProjectile->ProjectileHomingMovementData = HomingMovementData;
	NewProjectile->ProjectileDamagePipelineData = DamagePipelineData;
	NewProjectile->InstantiateProjectileFromData();
	
	/** *
	 * Add projectile to relevant pool
	 */
	ActiveProjectilePool.Push(NewProjectile);
	AvailableProjectilePool.Remove(NewProjectile);
	
	return NewProjectile;
}
