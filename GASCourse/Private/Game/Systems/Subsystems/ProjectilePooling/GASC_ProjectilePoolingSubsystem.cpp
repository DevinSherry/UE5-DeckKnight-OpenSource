// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/Projectile/GASCourseProjectile.h"
#include "Game/Projectile/Components/GASCourseProjectileMovementComp.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolSettings.h"
#include "TargetingSystem/TargetingPreset.h"

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
	for (FPooledProjectileSlot& Slot : PooledProjectiles)
	{
		if (IsValid(Slot.Projectile))
		{
			Slot.Projectile->Destroy();
		}
	}

	PooledProjectiles.Empty();
	AvailableSlots.Empty();

	Super::Deinitialize();
}

void UGASC_ProjectilePoolingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Iterating every slot and testing State costs a branch per pooled projectile rather than per
	// active one, which is cheaper than the bookkeeping a separate active list required.
	for (const FPooledProjectileSlot& Slot : PooledProjectiles)
	{
		if (Slot.State != EProjectilePoolSlotState::Claimed)
		{
			continue;
		}

		if (IsValid(Slot.Projectile))
		{
			Slot.Projectile->TickProjectile(DeltaTime);
		}
	}
}

TStatId UGASC_ProjectilePoolingSubsystem::GetStatId() const
{
	// A default TStatId left this tick - which drives all projectile movement - invisible to
	// stat commands and Unreal Insights.
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGASC_ProjectilePoolingSubsystem, STATGROUP_Tickables);
}

bool UGASC_ProjectilePoolingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* OuterWorld = Cast<UWorld>(Outer);
	if (!OuterWorld)
	{
		return false;
	}

	// Outer is always a UWorld for a world subsystem, so the old cast-only check was a tautology
	// and every world - editor preview, inactive, asset-editor - warmed a full pool of actors.
	return OuterWorld->WorldType == EWorldType::Game || OuterWorld->WorldType == EWorldType::PIE;
}

void UGASC_ProjectilePoolingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const UGASC_ProjectilePoolSettings* PoolSettings = GetDefault<UGASC_ProjectilePoolSettings>();

	// Resolved once here instead of once per pooled projectile: every instance used to
	// sync-load this same asset in its own BeginPlay. Must precede AddProjectilesToPool, which
	// is what triggers those BeginPlay calls.
	RicochetTargetingPreset = PoolSettings->RicochetTargetingPreset.LoadSynchronous();

	AddProjectilesToPool(PoolSettings->InitialPoolSize);
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::GetAvailableProjectile()
{
	while (AvailableSlots.Num() > 0)
	{
		const int32 SlotIndex = AvailableSlots.Pop(EAllowShrinking::No);
		if (!PooledProjectiles.IsValidIndex(SlotIndex))
		{
			continue;
		}

		FPooledProjectileSlot& Slot = PooledProjectiles[SlotIndex];
		if (!IsValid(Slot.Projectile))
		{
			continue;
		}

		Slot.State = EProjectilePoolSlotState::Claimed;
		Slot.Projectile->MarkClaimedFromPool();
		return Slot.Projectile;
	}

	if (!IsValid(ProjectileClass))
		return nullptr;

	// Pool exhausted: grow it. AddProjectileToPool_Internal pushes the new slot onto the free
	// list, so claim it straight back off the top.
	AGASCourseProjectile* NewProjectile = AddProjectileToPool_Internal();
	if (!IsValid(NewProjectile))
	{
		return nullptr;
	}

	AvailableSlots.Pop(EAllowShrinking::No);
	PooledProjectiles[NewProjectile->GetPoolSlotIndex()].State = EProjectilePoolSlotState::Claimed;
	NewProjectile->MarkClaimedFromPool();

	return NewProjectile;
}

void UGASC_ProjectilePoolingSubsystem::ReturnProjectileToPool(AGASCourseProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	// O(1) and no searching: the projectile knows its own slot, and the slot's state is what
	// makes a double return a no-op rather than something that could corrupt the free list.
	const int32 SlotIndex = Projectile->GetPoolSlotIndex();
	if (!PooledProjectiles.IsValidIndex(SlotIndex))
	{
		return;
	}

	FPooledProjectileSlot& Slot = PooledProjectiles[SlotIndex];
	if (Slot.Projectile != Projectile || Slot.State == EProjectilePoolSlotState::Available)
	{
		return;
	}

	Slot.State = EProjectilePoolSlotState::Available;
	AvailableSlots.Push(SlotIndex);

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

	// Must be last: the group lookup above reads this ID. GetProjectileGroupId returns by value,
	// so the previous GetProjectileGroupId().Invalidate() only cleared a temporary and the
	// projectile carried a stale group ID into its next activation.
	Projectile->SetProjectileGroupId(FGuid());
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

	const int32 SlotIndex = PooledProjectiles.Emplace(FPooledProjectileSlot{ NewProjectile, EProjectilePoolSlotState::Available });
	NewProjectile->SetPoolSlotIndex(SlotIndex);
	AvailableSlots.Push(SlotIndex);

	return NewProjectile;
}

AGASCourseProjectile* UGASC_ProjectilePoolingSubsystem::SpawnAndClaimProjectile(AActor* Instigator, const FTransform& SpawnTransform, UGASC_ProjectileData* InProjectileData, TSubclassOf<UGASC_ProjectileEventListener> EventListener, const TArray<FInstancedStruct>& AdditionalProjectileFragments)
{
	SCOPED_NAMED_EVENT(SpawnProjectile, FColor::Red);
	AGASCourseProjectile* NewProjectile = GetAvailableProjectile();
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
	
	const FProjectileSpawnShapeSnakeFragment* SnakeShapeFragment = Shape.GetPtr<FProjectileSpawnShapeSnakeFragment>();
	const bool bIsSnakeShape = SnakeShapeFragment != nullptr;
	
	// Claim the whole batch before assigning anything, so the snake fragments can be built against
	// the final projectile list. Building them against an incrementally grown list is what forced
	// the old code to re-broadcast the list to every member on each iteration.
	for (int32 i = 0; i < SpawnTransforms.Num(); ++i)
	{
		AGASCourseProjectile* NewProjectile = GetAvailableProjectile();
		if (!IsValid(NewProjectile))
			continue;
		Projectiles.Add(NewProjectile);
	}

	for (int32 i = 0; i < Projectiles.Num(); ++i)
	{
		if (bIsSnakeShape)
		{
			AssignSnakeFragment(Projectiles, i, SpawnTransforms[i],
				SnakeShapeFragment->bStopTrailAfterLeaderEnd, SnakeShapeFragment->SpawnDelayBetween);
		}

		ActivateProjectileFromPool_Internal(Projectiles[i], Instigator, SpawnTransforms[i], InProjectileData, EventListener, AdditionalProjectileFragments);
	}
	
	if (EventListener)
	{
		UGASC_ProjectileEventListener* NewEventListener = NewObject<UGASC_ProjectileEventListener>(Instigator, EventListener);
		NewEventListener->Projectiles = Projectiles;
		NewEventListener->OnListenerConstructed(Instigator);
	}
	
	return Projectiles;
}

void UGASC_ProjectilePoolingSubsystem::AssignSnakeFragment(const TArray<AGASCourseProjectile*>& SnakeProjectiles,
	int32 Index, const FTransform& SpawnTransform, bool bStopTrailAfterLeaderEnd, float SpawnDelayBetween)
{
	if (!SnakeProjectiles.IsValidIndex(Index))
	{
		return;
	}

	AGASCourseProjectile* Projectile = SnakeProjectiles[Index];
	if (!IsValid(Projectile))
	{
		return;
	}

	AGASCourseProjectile* LeaderProjectile = SnakeProjectiles.IsValidIndex(0) ? SnakeProjectiles[0] : nullptr;
	if (!IsValid(LeaderProjectile))
	{
		LeaderProjectile = Projectile;
	}

	FProjectileSpawnShapeSnakeFragment AssignedSnakeFragment;

	AssignedSnakeFragment.bIsLeader = (Projectile == LeaderProjectile);
	AssignedSnakeFragment.LeaderProjectile = LeaderProjectile;
	AssignedSnakeFragment.SnakeTrailIndex = Index;
	AssignedSnakeFragment.bStopTrailAfterLeaderEnd = bStopTrailAfterLeaderEnd;

	// TickTrailFollower spaces followers by speed * delay * index, so a zero delay collapses the
	// whole snake onto the leader. Each caller passes its own authoritative value.
	AssignedSnakeFragment.SpawnDelayBetween = SpawnDelayBetween;

	AssignedSnakeFragment.SnakeTrailLocations.Add(SpawnTransform.GetLocation());
	AssignedSnakeFragment.SnakeTrailRotations.Add(SpawnTransform.GetRotation().Rotator());

	for (AGASCourseProjectile* SnakeProjectile : SnakeProjectiles)
	{
		if (IsValid(SnakeProjectile))
		{
			AssignedSnakeFragment.SnakeProjectiles.AddUnique(SnakeProjectile);
		}
	}

	Projectile->InstantiateProjectileShapeSnakeFragment(AssignedSnakeFragment);
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
		AGASCourseProjectile* NewProjectile = GetAvailableProjectile();
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
	// Reset rather than append: callers pass an array they have only Reserve'd, and appending
	// would silently double up if one were ever reused.
	OutSpawnTransforms.Reset();

	if (!Instigator.IsValid() || Count <= 0)
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
			// Count > 0 is guaranteed by the guard above, so no SafeDivide needed.
			float AngleDegrees = (360.0f / Count) * i;
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
