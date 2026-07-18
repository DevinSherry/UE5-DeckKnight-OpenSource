// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/Projectile/GASCourseProjectile.h"
#include "Game/Projectile/Components/GASCourseProjectileMovementComp.h"
#include "Kismet/KismetMathLibrary.h"

UGASC_ProjectilePoolingSubsystem::UGASC_ProjectilePoolingSubsystem()
{
	ProjectileClass = AGASCourseProjectile::StaticClass();
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

void UGASC_ProjectilePoolingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int32 i = ActiveProjectilePool.Num() - 1; i >= 0; --i)
	{
		if (AGASCourseProjectile* Projectile = ActiveProjectilePool[i])
		{
			Projectile->TickProjectile(DeltaTime);
		}
		else
		{
			ActiveProjectilePool.RemoveAtSwap(i);
		}
	}
}

TStatId UGASC_ProjectilePoolingSubsystem::GetStatId() const
{
	return TStatId();
}

bool UGASC_ProjectilePoolingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* OuterWorld = Cast<UWorld>(Outer);
	return (OuterWorld && Super::ShouldCreateSubsystem(Outer));
}

void UGASC_ProjectilePoolingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	AddProjectilesToPool(100);
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::GetAvailableProjectile()
{
	while (AvailableProjectilePool.Num() > 0)
	{
		AGASCourseProjectile* AvailableProjectile =
			AvailableProjectilePool.Pop(EAllowShrinking::No);

		if (IsValid(AvailableProjectile))
		{
			ActiveProjectilePool.Add(AvailableProjectile);
			return AvailableProjectile;
		}
	}
			
	if (!IsValid(ProjectileClass))
		return nullptr;
		
	AGASCourseProjectile* NewProjectile = AddProjectileToPool_Internal();
	if (!IsValid(NewProjectile))
	{
		return nullptr;
	}

	// AddProjectileToPool_Internal creates an available projectile.
	// Claim it immediately.
	AvailableProjectilePool.Remove(NewProjectile);
	ActiveProjectilePool.Add(NewProjectile);

	return NewProjectile;
}

void UGASC_ProjectilePoolingSubsystem::ReturnProjectileToPool(AGASCourseProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	if (!ProjectilePool.Contains(Projectile))
	{
		return;
	}

	if (!ActiveProjectilePool.Contains(Projectile))
	{
		return;
	}

	uint32 Index = ActiveProjectilePool.Find(Projectile);
	ActiveProjectilePool.RemoveAtSwap(Index);

	if (!AvailableProjectilePool.Contains(Projectile))
	{
		AvailableProjectilePool.Push(Projectile);
	}
	
	FProjectileGroup* Group =
		ProjectileGroupMap.Find(Projectile->GetProjectileGroupId());

	if (!Group)
	{
		return;
	}

	Group->Projectiles.RemoveAll(
		[Projectile](const TWeakObjectPtr<AGASCourseProjectile>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Projectile;
		});

	if (Group->Projectiles.IsEmpty())
	{
		ProjectileGroupMap.Remove(Projectile->GetProjectileGroupId());
	}

	Projectile->GetProjectileGroupId().Invalidate();
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
	SCOPED_NAMED_EVENT(AddProjectileToPool_Internal, FColor::Green);

	if (!GetWorld() || !IsValid(ProjectileClass))
	{
		return nullptr;
	}

	AGASCourseProjectile* NewProjectile =
		GetWorld()->SpawnActorDeferred<AGASCourseProjectile>(
			ProjectileClass,
			FTransform::Identity,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

	if (!IsValid(NewProjectile))
	{
		return nullptr;
	}
	NewProjectile->ProjectileMovementComp->SetComponentTickEnabled(false);

	NewProjectile->FinishSpawning(FTransform::Identity);

	ProjectilePool.Add(NewProjectile);
	AvailableProjectilePool.Push(NewProjectile);

	return NewProjectile;
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::SpawnAndClaimProjectile(AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
	SCOPED_NAMED_EVENT(SpawnProjectile, FColor::Red);
	AGASCourseProjectile* NewProjectile = ClaimProjectileFromPool_Internal();
	if (!IsValid(NewProjectile))
		return nullptr;
	
	TArray<AGASCourseProjectile*> Projectiles;
	Projectiles.Reserve(1);
	Projectiles.Add(NewProjectile);
	ActivateProjectileFromPool_Internal(NewProjectile, Instigator, SpawnTransform, InProjectileData, EventListener, AdditionalProjectileFragments);
	
	if (EventListener)
	{
		UGASC_ProjectileEventListener* NewEventListener = NewObject<UGASC_ProjectileEventListener>(NewProjectile, EventListener);
		NewEventListener->Projectiles = Projectiles;
		NewEventListener->OnListenerConstructed(Instigator);
	}
	
	return NewProjectile;
}

TArray<AGASCourseProjectile*> UGASC_ProjectilePoolingSubsystem::SpawnAndClaimProjectilesInShape(AActor* Instigator, const FInstancedStruct Shape,
	int32 SpawnCount, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener,
	const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
	TArray<AGASCourseProjectile*> Projectiles;
	Projectiles.Reserve(SpawnCount);
	TArray<FTransform> SpawnTransforms;
	SpawnTransforms.Reserve(SpawnCount);
	ConstructShapeSpawnTransforms(Shape, Instigator->GetActorLocation(), Instigator, SpawnTransforms, SpawnCount);
	
	bool bIsSnakeShape = false;
	if (const FProjectileSpawnShapeSnakeFragment* CurrentFragment = Shape.GetPtr<FProjectileSpawnShapeSnakeFragment>())
	{
		bIsSnakeShape = true;
	}
	
	for (int32 i = 0; i < SpawnTransforms.Num(); ++i)
	{
		AGASCourseProjectile* NewProjectile = ClaimProjectileFromPool_Internal();
		if (!IsValid(NewProjectile))
			continue;
		Projectiles.Add(NewProjectile);
		
		if (bIsSnakeShape)
		{
			FProjectileSpawnShapeSnakeFragment AssignedSnakeFragment;

			AssignedSnakeFragment.bIsLeader = (i == 0);

			AGASCourseProjectile* LeaderProjectile =
				Projectiles.IsValidIndex(0) ? Projectiles[0] : NewProjectile;

			AssignedSnakeFragment.LeaderProjectile = LeaderProjectile;
			AssignedSnakeFragment.SnakeTrailIndex = i;

			AssignedSnakeFragment.SnakeTrailLocations.Add(
				SpawnTransforms[i].GetLocation());

			AssignedSnakeFragment.SnakeTrailRotations.Add(
				SpawnTransforms[i].GetRotation().Rotator());

			TArray<TWeakObjectPtr<AGASCourseProjectile>> UniqueSnakeProjectiles;

			for (AGASCourseProjectile* ExistingProjectile : Projectiles)
			{
				if (IsValid(ExistingProjectile))
				{
					UniqueSnakeProjectiles.AddUnique(ExistingProjectile);
				}
			}

			if (IsValid(NewProjectile))
			{
				UniqueSnakeProjectiles.AddUnique(NewProjectile);
			}

			AssignedSnakeFragment.SnakeProjectiles = UniqueSnakeProjectiles;

			NewProjectile->InstantiateProjectileShapeSnakeFragment(AssignedSnakeFragment);

			for (const TWeakObjectPtr<AGASCourseProjectile>& SnakeProjectileWeak : UniqueSnakeProjectiles)
			{
				AGASCourseProjectile* SnakeProjectile = SnakeProjectileWeak.Get();

				if (!IsValid(SnakeProjectile))
				{
					continue;
				}

				SnakeProjectile->ProjectileSpawnShapeSnakeFragment.SnakeProjectiles =
					UniqueSnakeProjectiles;
			}
		}

		ActivateProjectileFromPool_Internal(NewProjectile, Instigator, SpawnTransforms[i], InProjectileData, EventListener, AdditionalProjectileFragments);
	}
	
	if (EventListener)
	{
		UGASC_ProjectileEventListener* NewEventListener = NewObject<UGASC_ProjectileEventListener>(Instigator, EventListener);
		NewEventListener->Projectiles = Projectiles;
		NewEventListener->OnListenerConstructed(Instigator);
	}
	
	return Projectiles;
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::ClaimProjectileFromPool_Internal()
{
	AGASCourseProjectile* NewProjectile = GetAvailableProjectile();
	if (!IsValid(NewProjectile))
		return nullptr;
	
	return NewProjectile;
}

TArray<AGASCourseProjectile*> UGASC_ProjectilePoolingSubsystem::ClaimProjectilesFromPool(int32 Count)
{
	TArray<AGASCourseProjectile*> Projectiles;
	Projectiles.Reserve(Count);
	
	const FGuid GroupId = FGuid::NewGuid();
	FProjectileGroup& Group = ProjectileGroupMap.Add(GroupId);
	Group.GroupId = GroupId;
	
	for (int32 i = 0; i < Count; ++i)
	{
		AGASCourseProjectile* NewProjectile = ClaimProjectileFromPool_Internal();
		if (!IsValid(NewProjectile))
			continue;
		Projectiles.Add(NewProjectile);
		NewProjectile->SetProjectileIndex(i);
		NewProjectile->SetProjectileGroupId(GroupId);
		
		Group.Projectiles.Add(NewProjectile);
	}
	
	return Projectiles;
}

void UGASC_ProjectilePoolingSubsystem::ActivateProjectileFromPool_Internal(AGASCourseProjectile* ProjectileToActivate,
                                                                           AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData,
                                                                           TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
	if (APawn* Pawn = Cast<APawn>(Instigator))
	{
		ProjectileToActivate->SetInstigator(Pawn);
	}
	
	ProjectileToActivate->SetOwner(Instigator);
	ProjectileToActivate->SetActorTransform(SpawnTransform);
	ProjectileToActivate->ProjectileDataAsset = InProjectileData;
	ProjectileToActivate->ProjectileFragments.Append(InProjectileData->ProjectileDataFragments);
	ProjectileToActivate->ProjectileFragments.Append(AdditionalProjectileFragments);
	ProjectileToActivate->InstantiateProjectileFromData();
}

void UGASC_ProjectilePoolingSubsystem::ConstructShapeSpawnTransforms(const FInstancedStruct& ShapeFragment, const FVector& SpawnOrigin, const TWeakObjectPtr<AActor> Instigator, TArray<FTransform>& OutSpawnTransforms, const int32 Count)
{
	if (!Instigator.IsValid())
	{
		return;
	}
	
	
	const FProjectileSpawnShapeBaseFragment* BaseFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeBaseFragment>();
	FTransform SpawnTransform;
	if (!BaseFragment)
	{
		OutSpawnTransforms.Init(Instigator->GetActorTransform(), Count);
		return;
	}
	
	SpawnTransform = BaseFragment->bCustomSpawnTransform ? BaseFragment->SpawnTransform : Instigator->GetActorTransform();

	if (const FProjectileSpawnShapeCircleFragment* CurrentFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeCircleFragment>())
	{
		for (int i = 0; i < Count; ++i)
		{
			float AngleDegrees = UKismetMathLibrary::SafeDivide(360.0f, Count) * i;
			float AngleRad = FMath::DegreesToRadians(AngleDegrees);
			
			float SpawnDirectionX = FMath::Cos(AngleRad);
			float SpawnDirectionY = FMath::Sin(AngleRad);
			
			FVector SpawnDirection = FVector(SpawnDirectionX, SpawnDirectionY, SpawnTransform.GetRotation().Vector().Z).GetSafeNormal();
			FVector SpawnPosition = SpawnTransform.GetLocation() + (SpawnDirection * CurrentFragment->SpawnRadius);
			
			OutSpawnTransforms.Add(FTransform(SpawnDirection.Rotation(), SpawnPosition));
		}
		return;
	}
	
	if (const FProjectileSpawnShapeConeFragment* CurrentFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeConeFragment>())
	{
		float ArcDegrees = CurrentFragment->ConeAngle;
		FVector OwnerForward = Instigator.Get() ? Instigator->GetActorForwardVector() : FVector::ZeroVector;
		float BaseAngleRad = FMath::Atan2(OwnerForward.Y, OwnerForward.X);

		for (int32 i = 0; i < Count; ++i)
		{
			float Alpha = (Count == 1) ? 0.5f : static_cast<float>(i) / (Count - 1);
			float OffsetAngleRad = FMath::DegreesToRadians(-ArcDegrees * 0.5f + ArcDegrees * Alpha);
			float AngleRad = BaseAngleRad + OffsetAngleRad;
			
			float SpawnDirectionX = FMath::Cos(AngleRad);
			float SpawnDirectionY = FMath::Sin(AngleRad);
			
			FVector SpawnDirection = FVector(SpawnDirectionX, SpawnDirectionY, SpawnTransform.GetRotation().Vector().Z);
			FVector SpawnPosition = SpawnTransform.GetLocation() + SpawnDirection * CurrentFragment->SpawnRadius;
			
			OutSpawnTransforms.Add(FTransform(SpawnDirection.Rotation(), SpawnPosition));
		}
		return;
	}
	
	if (const FProjectileSpawnShapeSpiralFragment* CurrentFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeSpiralFragment>())
	{
		int32 SpiralTurns = CurrentFragment->NumRotations;
		float Radius = CurrentFragment->SpawnRadius;
		
		for (int32 i = 0; i < Count; ++i)
		{
			float t = static_cast<float>(i) / Count; // 0..1
			float AngleRad = SpiralTurns * 2.0f * PI * t;
			float CurrentRadius = Radius * t;
			FVector SpawnPosition = SpawnOrigin + FVector(
				FMath::Cos(AngleRad) * CurrentRadius,
				FMath::Sin(AngleRad) * CurrentRadius,
				0.0f
			);
			FVector SpawnDirection = (SpawnPosition - SpawnOrigin).GetSafeNormal();

			OutSpawnTransforms.Add(FTransform(SpawnDirection.Rotation(), SpawnPosition));
		}
		return;
	}
	
	if (const FProjectileSpawnShapeLineFragment* CurrentFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeLineFragment>())
	{
		FVector SpawnDirection = CurrentFragment->bUseInstigatorActorForward ? Instigator->GetActorForwardVector() : CurrentFragment->SpawnDirection;
		FVector SpawnPosition = SpawnOrigin + SpawnDirection * CurrentFragment->SpawnRadius;
		
		OutSpawnTransforms.Init(FTransform(SpawnDirection.Rotation(), SpawnPosition), Count);
		return;
	}
	
	if (const FProjectileSpawnShapeSnakeFragment* CurrentFragment = ShapeFragment.GetPtr<FProjectileSpawnShapeSnakeFragment>())
	{
		FVector SpawnDirection = Instigator->GetActorForwardVector();
		FVector SpawnPosition = SpawnOrigin + SpawnDirection * CurrentFragment->SpawnRadius;
		OutSpawnTransforms.Init(FTransform(SpawnDirection.Rotation(), SpawnPosition), Count);
		return;
	}
}
