// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Projectile/GASCourseProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameplayCueManager.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Game/Projectile/Components/GASCourseProjectileMovementComp.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "TargetingSystem/TargetingPreset.h"
#include "Types/TargetingSystemTypes.h"
#include "Game/Projectile/GASC_ProjectileData.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolSettings.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_AreaOfEffectData.h"

DEFINE_LOG_CATEGORY(LOG_GASC_Projectile);

// Sets default values
AGASCourseProjectile::AGASCourseProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetTickGroup(TG_PrePhysics);
	PrimaryActorTick.TickInterval = 0.0167;
	
	ProjectileMovementComp = CreateDefaultSubobject<UGASCourseProjectileMovementComp>(TEXT("ProjectileMovementComp"));
	ProjectileCollisionComp = CreateDefaultSubobject<UCapsuleComponent>("ProjectileCollisionComp");
	ProjectileVisualMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileVisualMeshComponent");
	ProjectileAudioComponent = CreateDefaultSubobject<UAudioComponent>("ProjectileAudioComponent");
	ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("ProjectileNiagaraComponent");
	ProjectileRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileRootComponent"));

	SetRootComponent(ProjectileCollisionComp);

	ProjectileCollisionComp->SetCollisionProfileName("Projectile");
	ProjectileCollisionComp->SetEnableGravity(false);
	ProjectileCollisionComp->SetAutoActivate(true);
	ProjectileCollisionComp->SetComponentTickEnabled(false);
	
	/**
	 * 
	 * Common properties of the projectile visual mesh component.
	 */
	ProjectileVisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileVisualMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileVisualMeshComponent->SetGenerateOverlapEvents(false);
	ProjectileVisualMeshComponent->SetAutoActivate(false);
	ProjectileVisualMeshComponent->SetComponentTickEnabled(false);
	ProjectileVisualMeshComponent->SetupAttachment(ProjectileCollisionComp);
	
	//** 
	// Common properties of the projectile visual mesh component.
	//*/
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->SetComponentTickEnabled(false);
	ProjectileMovementComp->Deactivate();
	ProjectileMovementComp->StopMovementImmediately();
	ProjectileMovementComp->Velocity = FVector::ZeroVector;
	ProjectileMovementComp->bIsHomingProjectile = false;
	ProjectileMovementComp->HomingTargetComponent = nullptr;
	
	/** *
	 * Common properties of the projectile Niagara component.
	 */
	ProjectileNiagaraComponent->SetAutoActivate(false);
	ProjectileNiagaraComponent->SetComponentTickEnabled(false);
	ProjectileNiagaraComponent->SetupAttachment(ProjectileCollisionComp);
	
	//TODO: Add common properties for the audio component
	ProjectileAudioComponent->SetAutoActivate(false);
	ProjectileAudioComponent->SetComponentTickEnabled(false);
	ProjectileAudioComponent->SetupAttachment(ProjectileCollisionComp);
}

bool AGASCourseProjectile::ApplyDamagetoTargetOnHit_Implementation(AActor* InHitActor, const FHitResult& InHitResult)
{
	return true;
}

bool AGASCourseProjectile::IsActorAnAlly_Implementation(AActor* InHitActor) const
{
	if (InHitActor)
	{
		if (AGASCourseCharacter* InstigatorCharacter = Cast<AGASCourseCharacter>(GetInstigator()))
		{
			uint8 InstigatorTeamID = InstigatorCharacter->GetGenericTeamId();
			if (AGASCourseCharacter* HitTargetActorAsCharacter = Cast<AGASCourseCharacter>(InHitActor))
			{
				uint8 TargetTeamID = HitTargetActorAsCharacter->GetGenericTeamId();
				return InstigatorTeamID == TargetTeamID;
			}
		}
	}

	return false;
}

void AGASCourseProjectile::OnProjectileRicochet_Implementation()
{
}

void AGASCourseProjectile::ApplyDamagePipelineToHitTarget(AActor* OtherActor, const FHitResult& InHitResult)
{
	AActor* HitActor = OtherActor;
	if (!HitActor)
	{
		return;
	}
	
	if (GetWorld() == nullptr)
	{
		return;
	}
	
	const bool bCanDamageTarget = IsActorAnAlly(HitActor) ? ProjectileDamageFragment.bCanDamageAllies : true;
	const bool bCanHealTarget = IsActorAnAlly(HitActor) ? true : ProjectileHealingFragment.bCanHealEnemies;
	
	UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = GetWorld()->GetSubsystem<UGASC_ResourcePipelineSubsystem>();
	if (!DamagePipelineSubsystem)
	{
		return;
	}
	
	FGameplayEventData HitEventData;
	HitEventData.Target = HitActor;
	HitEventData.Instigator = GetInstigator();
	HitEventData.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(InHitResult);
	UGASCourseAbilitySystemComponent* TargetASC = Cast<UGASCourseAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor));
	if (TargetASC)
	{
		FGameplayTagContainer TargetTags;
		TargetASC->GetOwnedGameplayTags(TargetTags);
		HitEventData.TargetTags = TargetTags;
	}
	
	if (bCanDamageTarget)
	{
		if (ProjectileDamageFragment.Damage > 0.0f && bCanDamageTarget)
		{
			ConstructDamagePipelineHitEvent(ProjectileDamageFragment.OnHitEvent, HitEventData);
			
			const float InDamage = ProjectileDamageFragment.Damage;
			const FGameplayTag InDamageTypeTag = ProjectileDamageFragment.DamageType;
			const FGameplayTagContainer InDamageGrantedTags = ProjectileDamageFragment.DamageGrantedTags;
			FDamagePipelineContext DamagePipelineContext;
			DamagePipelineContext.DamageType = InDamageTypeTag;
			DamagePipelineContext.GrantedTags = InDamageGrantedTags;
			DamagePipelineContext.HitResult = InHitResult;
			HitEventData.EventMagnitude = InDamage;
		
			if (ProjectileDamageFragment.bDamageOverTime)
			{
				FDamagePipelineEffectOverTimeContext OverTimeContext = ProjectileDamageFragment.EffectOverTimeContext;
				DamagePipelineSubsystem->ApplyDamageOverTimeToTarget(HitActor, GetInstigator(), InDamage, DamagePipelineContext, OverTimeContext);
			}
			else
			{
				DamagePipelineSubsystem->ApplyDamageToTarget(HitActor, GetInstigator(), InDamage, DamagePipelineContext);
			}
			
			if (TargetASC)
			{
				TargetASC->SendGameplayEventAsync(HitEventData.EventTag, HitEventData);
			}
		}
	}
	
	if (ProjectileHealingFragment.Healing > 0.0f && bCanHealTarget)
	{
		ConstructDamagePipelineHitEvent(ProjectileHealingFragment.OnHitEvent, HitEventData);
		
		float InHealing = ProjectileHealingFragment.Healing;
		FGameplayTag InHealingTypeTag = ProjectileHealingFragment.HealingType;
		FGameplayTagContainer InHealingGrantedTags = ProjectileHealingFragment.HealingGrantedTags;
		FDamagePipelineContext HealingPipelineContext;
		HealingPipelineContext.DamageType = InHealingTypeTag;
		HealingPipelineContext.GrantedTags = InHealingGrantedTags;
		HealingPipelineContext.HitResult = InHitResult;
		HitEventData.EventMagnitude = InHealing;
		
		if (ProjectileHealingFragment.bHealOverTime)
		{
			FDamagePipelineEffectOverTimeContext OverTimeContext = ProjectileHealingFragment.EffectOverTimeContext;
			DamagePipelineSubsystem->ApplyHealOverTimeToTarget(HitActor, GetInstigator(), InHealing, HealingPipelineContext, OverTimeContext);
		}
		else
		{
			DamagePipelineSubsystem->ApplyHealToTarget(HitActor, GetInstigator(), InHealing, HealingPipelineContext);
		}
		
		if (TargetASC)
		{
			TargetASC->SendGameplayEventAsync(HitEventData.EventTag, HitEventData);
		}
	}
}

void AGASCourseProjectile::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ProjectileCollisionComp->GetMoveIgnoreActors().Contains(OtherActor) || OtherActor == GetInstigator() || OtherActor == this || !bFromSweep)
	{
		return;
	}
	
	const bool bCanDamageTarget = IsActorAnAlly(OtherActor) ? ProjectileDamageFragment.bCanDamageAllies : ProjectileDamageFragment.Damage > 0.0f;
	const bool bCanHealTarget = IsActorAnAlly(OtherActor) ? ProjectileHealingFragment.Healing > 0.0f : ProjectileHealingFragment.bCanHealEnemies;
	if (!bCanDamageTarget && !bCanHealTarget && OtherActor->IsA(AGASCourseCharacter::StaticClass()))
	{
		return;
	}
	
	if (!HitTargets.Contains(OtherActor))
	{
		if (OtherActor->IsA(AGASCourseCharacter::StaticClass()) && !HitTargets.Contains(OtherActor))
		{
			HitTargets.Add(OtherActor);
			
			if (ProjectileRicochetFragment.bCanRicochet && ProjectileRicochetCount < ProjectileRicochetFragment.NumberOfRicochet && RicochetTargetingPreset)
			{
				//TODO this can be where we do the percentage roll for ricochet
				if(UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld()))
				{
					FTargetingSourceContext TargetingSourceContext;
					TargetingSourceContext.InstigatorActor = GetInstigator();
					TargetingSourceContext.SourceActor = this;
					TargetingSourceContext.SourceLocation = GetActorLocation();
					TargetingSourceContext.SourceObject = this;
					FTargetingRequestDelegate OnCompletedDelegate;
					OnCompletedDelegate.BindUFunction(this, FName("OnTargetRequestCompleted"));
					CurrentTargetHandle = UTargetingSubsystem::MakeTargetRequestHandle(RicochetTargetingPreset, TargetingSourceContext);
					FTargetingAsyncTaskData& AsyncTaskData = FTargetingAsyncTaskData::FindOrAdd(CurrentTargetHandle);
					AsyncTaskData.bReleaseOnCompletion = true;
					TargetingSubsystem->StartAsyncTargetingRequestWithHandle(CurrentTargetHandle, OnCompletedDelegate);
				}
			}
		}
	}
	
	ProjectileCollisionComp->IgnoreActorWhenMoving(OtherActor, true);
	OnProjectileHitDelegate.Broadcast(OtherActor, SweepResult);
	OnProjectileHit(OtherActor, SweepResult);
}

void AGASCourseProjectile::OnTargetRequestCompleted(FTargetingRequestHandle TargetingRequestHandle)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if(UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(World))
	{
		TargetingSubsystem->GetTargetingResultsActors(TargetingRequestHandle, FoundTargets);
		if(FoundTargets.IsEmpty())
		{
			ReturnProjectileToPool();
			return;
		}

		TargetingSubsystem->RemoveAsyncTargetingRequestWithHandle(TargetingRequestHandle);

		for(int32 i = FoundTargets.Num() -1; i >= 0;  --i)
		{
			if(HitTargets.Contains(FoundTargets[i]))
			{
				FoundTargets.RemoveAt(i);
			}
		}

		if(FoundTargets.IsEmpty())
		{
			ReturnProjectileToPool();
			return;
		}

		ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndProbe);
		if(AActor* NewTarget = FoundTargets[0])
		{
			TargetActor = NewTarget;
			FRotator RotationDirection = (UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), NewTarget->GetActorLocation()));
			FVector DirectionVector = RotationDirection.Vector();
			DirectionVector.Normalize();
			ProjectileMovementComp->Velocity = DirectionVector * ProjectileMovementComp->InitialSpeed;

			// Redirecting the velocity is not sufficient for a homing projectile - the homing state
			// still refers to the target that was just hit. Must follow the TargetActor assignment
			// above, which the death-callback re-registration inside this reads.
			RetargetHomingToActor(NewTarget);

			OnProjectileRicochet();
		}
		else
		{
			ReturnProjectileToPool();
		}
	}
}

void AGASCourseProjectile::OnTargetDeathCallback(FGameplayTag MatchingTag, int32 NewCount)
{
	if (MatchingTag == Status_Death)
	{
		UnregisterTargetDeathCallback();
		ProjectileMovementComp->bIsHomingProjectile = false;
		ProjectileMovementComp->ResetBezierHoming();
		ProjectileMovementComp->bUseBezierHoming = false;
		ProjectileMovementComp->HomingTargetComponent = nullptr;
	}
}

void AGASCourseProjectile::OnProjectileLifetimeExpired()
{
	OnProjectileLifetimeExpiredDelegate.Broadcast();
	ProjectileLifetimeTimer.Invalidate();
	
	if (ProjectileDataAsset)
	{
		if (ProjectileVisualsFragment.ExpireGameplayCueTag.IsValid())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = GetActorLocation();
			Parameters.Instigator = GetOwner();
			Parameters.EffectCauser = GetOwner();
			
			UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), ProjectileVisualsFragment.ExpireGameplayCueTag, Parameters);
		}
		else
		{
			// Preloaded at activation; the sync load is only a not-yet-resident fallback.
			UNiagaraSystem* ProjectileExpireVFX = ProjectileVisualsFragment.ProjectileExpireVFX.Get();
			if (!ProjectileExpireVFX)
			{
				ProjectileExpireVFX = ProjectileVisualsFragment.ProjectileExpireVFX.LoadSynchronous();
			}

			if (ProjectileExpireVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ProjectileExpireVFX, GetActorLocation());
			}
		}
	}
	
	ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectileExpire);
	ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectileExpire, this);
	
	//Handle object pooling return logic in separate function?
	ReturnProjectileToPool();
}

void AGASCourseProjectile::OnProjectileHit(AActor* OtherActor, const FHitResult& InHitResult)
{
	FHitResult ProjectileHitResult = InHitResult;
	if (!ProjectileHitResult.bBlockingHit)
	{
		//ProjectileHitResult.ImpactPoint = GetActorLocation();
	}
	if (ProjectileDataAsset)
	{
		if (ProjectileVisualsFragment.ImpactGameplayCueTag.IsValid())
		{
			FGameplayCueParameters Parameters = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(ProjectileHitResult);
#if ENABLE_DRAW_DEBUG
			DrawDebugSphere(GetWorld(), ProjectileHitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
#endif
			UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), ProjectileVisualsFragment.ImpactGameplayCueTag, Parameters);
		}
		else
		{
			// Preloaded at activation, so Get() normally succeeds. The sync load stays only as a
			// fallback for the case where the load has not finished yet.
			UNiagaraSystem* ProjectileImpactVFX = ProjectileVisualsFragment.ProjectileImpactVFX.Get();
			if (!ProjectileImpactVFX)
			{
				ProjectileImpactVFX = ProjectileVisualsFragment.ProjectileImpactVFX.LoadSynchronous();
			}

			if (ProjectileImpactVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ProjectileImpactVFX, ProjectileHitResult.ImpactPoint);
			}
		}
	}
	
	ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectileHit, OtherActor);
	ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectileHit, OtherActor);
	ApplyDamagePipelineToHitTarget(OtherActor, ProjectileHitResult);
	
	if (ProjectileRicochetFragment.bCanRicochet && ProjectileRicochetCount < ProjectileRicochetFragment.NumberOfRicochet && OtherActor->IsA(AGASCourseCharacter::StaticClass()))
	{
		ProjectileRicochetCount++;
		OnProjectileRicochetDelegate.Broadcast(OtherActor);
		ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectileRicochet, OtherActor);
		ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectileRicochet, OtherActor);
	}
	else if (ProjectilePiercingFragment.bCanPierce)
	{
		// The pierce payload has to run while the projectile is still live. ReturnProjectileToPool
		// clears the fragments and nulls the instigator, after which
		// ApplyGameplayEffectOnProjectileEvent early-outs on its instigator check and the pierce
		// effects silently never apply - which is what happened when this ran after the return.
		OnProjectilePierceDelegate.Broadcast(OtherActor);
		ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectilePierce, OtherActor);
		ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectilePierce, OtherActor);
	}
	else
	{
		// Neither ricocheting nor piercing: this hit consumes the projectile. The lifetime timer
		// is cleared inside the teardown, so it no longer needs clearing here.
		ReturnProjectileToPool();
	}
}

void AGASCourseProjectile::InstantiateProjectileFromData()
{
	if (!ProjectileMovementComp)
	{
		return;
	}
	if (!ProjectileDataAsset)
	{
		return;
	}

	// Must precede every Instantiate*Fragment call below - they all resolve through it.
	RebuildFragmentLookup();

	// Each of these returns false when its fragment is absent, which is the normal case for most
	// of them. Nothing acts on the result, so they are called as a plain sequence. Order is
	// load-bearing in two places: collision and movement must precede the homing passes (which
	// read InitialSpeed and the spawn transform), and orbiting must come after them because it
	// overrides the movement they set up.
	InstantiateProjectileCollisionFragment();
	InstantiateProjectileMovementFragment();
	InstantiateProjectileParabolicMovementFragment();
	InstantiateProjectileHomingMovementFragment();
	InstantiateProjectileBezierHomingMovementFragment();
	InstantiateProjectileVisualFragment();
	InstantiateProjectileRicochetFragment();
	InstantiateProjectileDamageFragment();
	InstantiateProjectileHealingFragment();
	InstantiateProjectileGameplayEffectsFragment();
	InstantiateProjectileAreaOfEffectFragment();
	InstantiateProjectilePiercingFragment();
	InstantiateProjectileOrbitRotationFragment();

	SetActorHiddenInGame(false);
	OnProjectileCreatedDelegate.Broadcast(GetOwner());
	ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectileSpawn);
	ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectileSpawn);
	GetWorld()->GetTimerManager().SetTimer(ProjectileLifetimeTimer, this, &AGASCourseProjectile::OnProjectileLifetimeExpired, ProjectileDataAsset->ProjectilePoolingData.LifeTime, false);
}

void AGASCourseProjectile::ReturnProjectileToPool()
{
	if (bIsReturnedToPool)
	{
		return;
	}
	bIsReturnedToPool = true;

	// ClearTimer is a no-op on an invalid handle and invalidates the handle it is given, so no
	// guards are needed. The lifetime timer matters most: only one of the five return paths used
	// to clear it, so an early return left it pending and it fired against the recycled actor.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProjectileHomingDOTCheckTimer);
		World->GetTimerManager().ClearTimer(ProjectileHomingTimeoutTimer);
		World->GetTimerManager().ClearTimer(ProjectileLifetimeTimer);
	}

	// An in-flight ricochet search would otherwise complete against this actor after it has been
	// recycled and redirect whichever shot is using it next. Remove cancels the executing task,
	// releases the handle's data stores and resets the handle.
	if (CurrentTargetHandle.IsValid())
	{
		if (UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld()))
		{
			TargetingSubsystem->RemoveAsyncTargetingRequestWithHandle(CurrentTargetHandle);
		}
	}
	CurrentTargetHandle.Reset();

	UnregisterTargetDeathCallback();

	PromoteNewSnakeLeader();
	
	if (ProjectileNiagaraComponent)
	{
		// The asset is deliberately left assigned. Clearing it to null forced a full
		// ReinitializeSystem on the next activation - the most expensive per-spawn operation in
		// the pipeline - whereas keeping it lets the next shot reuse the asset when it matches.
		ProjectileNiagaraComponent->Deactivate();
		ProjectileNiagaraComponent->ResetSystem();
		ProjectileNiagaraComponent->SetHiddenInGame(true);
	}

	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->bIsHomingProjectile = false;
		ProjectileMovementComp->HomingAccelerationMagnitude = 0.0f;
		ProjectileMovementComp->InitialSpeed = 0.0f;
		ProjectileMovementComp->MaxSpeed = 0.0f;
		ProjectileMovementComp->StopMovementImmediately();
		ProjectileMovementComp->Velocity = FVector::ZeroVector;
		ProjectileMovementComp->HomingTargetComponent = nullptr;
		ProjectileMovementComp->Deactivate();
		ProjectileMovementComp->SetComponentTickEnabled(false);
		ProjectileMovementComp->bShouldBounce = false;
		ProjectileMovementComp->ResetBezierHoming();
		ProjectileMovementComp->bUseBezierHoming = false;
		ProjectileMovementComp->bDisableHomingBasedOnDotProduct = false;
		ProjectileMovementComp->DisableHomingDotProductMin = 0.0f;
		// Restored to the default rather than false, so a pooled projectile without a homing
		// fragment does not inherit an unconstrained flag from a previous shot.
		ProjectileMovementComp->bConstrainHomingToHorizontalPlane = true;
	}
	
	if (ProjectileVisualMeshComponent)
	{
		ProjectileVisualMeshComponent->Deactivate();
	}
	
	if (ProjectileCollisionComp)
	{
		ProjectileCollisionComp->SetCapsuleHalfHeight(0.0f);
		ProjectileCollisionComp->SetCapsuleRadius(0.0f);
		ProjectileCollisionComp->ClearMoveIgnoreActors();
		ProjectileCollisionComp->SetComponentTickEnabled(false);
		ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	ProjectileHomingMovementFragment = FProjectileHomingMovementFragment();
	ProjectileHomingBezierMovementFragment = FProjectileHomingBezierMovementFragment();
	ProjectileCollisionFragment = FProjectileCollisionFragment();
	ProjectileMovementFragment = FProjectileMovementFragment();
	ProjectileVisualsFragment = FProjectileVisualsFragment();
	ProjectileDamageFragment = FProjectileDamageFragment();
	ProjectileHealingFragment = FProjectileHealingFragment();
	ProjectileGameplayEffectsFragment = FProjectileGameplayEffectsFragment();
	ProjectileSpawnShapeSnakeFragment = FProjectileSpawnShapeSnakeFragment();
	ProjectileOrbitingFragment = FProjectileOrbitingFragment();

	// Both of these are read without a FindProjectileFragment guard - OnProjectileHit tests
	// bCanPierce and ApplyAreaOfEffectOnProjectileEvent walks the AOE data directly - so leaving
	// them populated let a recycled projectile inherit the previous shot's piercing and AOE
	// behaviour. A stale bCanPierce in particular meant the projectile never returned on hit.
	ProjectilePiercingFragment = FProjectilePiercingFragment();
	ProjectileAreaOfEffectFragment = FProjectileAOESpawnFragment();
	
	ProjectileRicochetFragment = FProjectileRicochetFragment();
	ProjectileRicochetCount = 0;
	
	bOrbitRotationEnabled = false;
	
	ProjectileIndex = -1;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	
	FVector DisabledProjectileLocation = FVector(0.0f, 0.0f, -10000.0f);
	FRotator DisabledProjectileRotation = FRotator::ZeroRotator;
	FTransform DisabledProjectileTransform(
	DisabledProjectileRotation,
	DisabledProjectileLocation);
	SetActorTransform(DisabledProjectileTransform);

	SetOwner(nullptr);
	SetInstigator(nullptr);
	HitTargets.Empty();

	// UPROPERTY array of raw AActor* - leaving it populated keeps the last ricochet search's
	// actors referenced for as long as this pooled instance lives, and the stale entries feed
	// the next shot's filter in OnTargetRequestCompleted.
	FoundTargets.Empty();

	ProjectileFragments.Empty();
	FragmentLookup.Reset();

	// Safe here: the expire VFX is spawned before this runs, and SpawnSystemAtLocation's component
	// holds its own reference to the asset.
	ProjectileImpactAssetHandle.Reset();
	
	OnProjectileReturnedToPoolDelegate.Broadcast(this);

	// After the broadcast above, so listeners still hear about this return, but before the actor
	// goes back into the pool - nothing may stay bound across a reuse.
	ClearProjectileEventDelegates();

	if (UGASC_ProjectilePoolingSubsystem* PoolingSubsystem = GetWorld()->GetSubsystem<UGASC_ProjectilePoolingSubsystem>())
	{
		PoolingSubsystem->ReturnProjectileToPool(this);
	}
	else
	{
		//If the pooling system does not exist, destroy instead.
		Destroy();
		UE_LOGFMT(LOG_GASC_Projectile, Warning, "Object Pooling subsystem not found! Destroying {0} - ", *GetNameSafe(this));
	}
}

void AGASCourseProjectile::ClearProjectileEventDelegates()
{
	OnProjectileCreatedDelegate.Clear();
	OnProjectileHitDelegate.Clear();
	OnProjectileRicochetDelegate.Clear();
	OnProjectilePierceDelegate.Clear();
	OnProjectileReturnedToPoolDelegate.Clear();
	OnProjectileLifetimeExpiredDelegate.Clear();
}

void AGASCourseProjectile::PromoteNewSnakeLeader()
{
	if (!ProjectileSpawnShapeSnakeFragment.bIsLeader)
	{
		return;
	}
	
	if (ProjectileSpawnShapeSnakeFragment.bStopTrailAfterLeaderEnd)
	{
		TArray<TWeakObjectPtr<AGASCourseProjectile>> OldSnakeList = ProjectileSpawnShapeSnakeFragment.SnakeProjectiles;
		for (auto Projectile : OldSnakeList)
		{
			if (Projectile.IsValid())
			{
				Projectile->ProjectileSpawnShapeSnakeFragment.LeaderProjectile = nullptr;
				Projectile->ProjectileSpawnShapeSnakeFragment.bIsLeader = false;
				Projectile->ProjectileSpawnShapeSnakeFragment.SnakeTrailLocations.Reset();
				Projectile->ProjectileSpawnShapeSnakeFragment.SnakeTrailRotations.Reset();
				Projectile->ProjectileSpawnShapeSnakeFragment.SnakeTrailIndex = INDEX_NONE;
				Projectile->ProjectileSpawnShapeSnakeFragment.SnakeProjectiles.Empty();
				
				Projectile->InstantiateProjectileMovementFragment();
				if (Projectile->ProjectileMovementComp->bIsHomingProjectile)
				{
					Projectile->ProjectileMovementComp->HomingTargetComponent = nullptr;
					Projectile->ProjectileMovementComp->bIsHomingProjectile = false;
				}
			}
		}
		
		return;
	}

	TArray<TWeakObjectPtr<AGASCourseProjectile>> OldSnakeList =
		ProjectileSpawnShapeSnakeFragment.SnakeProjectiles;

	OldSnakeList.RemoveAll([](const TWeakObjectPtr<AGASCourseProjectile>& Projectile)
	{
		return !Projectile.IsValid();
	});

	const int32 CurrentLeaderIndex = OldSnakeList.IndexOfByPredicate(
		[this](const TWeakObjectPtr<AGASCourseProjectile>& Projectile)
		{
			return Projectile.Get() == this;
		});

	if (CurrentLeaderIndex == INDEX_NONE)
	{
		return;
	}

	const int32 NewLeaderIndex = CurrentLeaderIndex + 1;

	if (!OldSnakeList.IsValidIndex(NewLeaderIndex))
	{
		return;
	}

	AGASCourseProjectile* NewLeader = OldSnakeList[NewLeaderIndex].Get();

	if (!IsValid(NewLeader))
	{
		return;
	}

	TArray<TWeakObjectPtr<AGASCourseProjectile>> NewSnakeList;

	for (int32 i = NewLeaderIndex; i < OldSnakeList.Num(); ++i)
	{
		AGASCourseProjectile* Snake = OldSnakeList[i].Get();

		if (!IsValid(Snake))
		{
			continue;
		}

		NewSnakeList.Add(Snake);
	}

	if (NewSnakeList.Num() == 0)
	{
		return;
	}

	FProjectileSpawnShapeSnakeFragment& NewLeaderFragment =
		NewLeader->ProjectileSpawnShapeSnakeFragment;

	NewLeaderFragment.bIsLeader = true;
	NewLeaderFragment.LeaderProjectile = NewLeader;
	NewLeaderFragment.SnakeProjectiles = NewSnakeList;
	NewLeaderFragment.SnakeTrailIndex = 0;

	NewLeaderFragment.SnakeTrailLocations.Reset();
	NewLeaderFragment.SnakeTrailRotations.Reset();

	for (int32 i = NewSnakeList.Num() - 1; i >= 0; --i)
	{
		AGASCourseProjectile* Snake = NewSnakeList[i].Get();

		if (!IsValid(Snake))
		{
			continue;
		}

		NewLeaderFragment.SnakeTrailLocations.Add(Snake->GetActorLocation());
		NewLeaderFragment.SnakeTrailRotations.Add(Snake->GetActorRotation());
	}

	NewLeader->InstantiateProjectileMovementFragment();

	for (int32 i = 0; i < NewSnakeList.Num(); ++i)
	{
		AGASCourseProjectile* Snake = NewSnakeList[i].Get();

		if (!IsValid(Snake))
		{
			continue;
		}

		FProjectileSpawnShapeSnakeFragment& SnakeFragment =
			Snake->ProjectileSpawnShapeSnakeFragment;

		SnakeFragment.SnakeProjectiles = NewSnakeList;
		SnakeFragment.LeaderProjectile = NewLeader;
		SnakeFragment.SnakeTrailIndex = i;
		SnakeFragment.bIsLeader = Snake == NewLeader;
	}

	ProjectileSpawnShapeSnakeFragment.bIsLeader = false;
	ProjectileSpawnShapeSnakeFragment.LeaderProjectile = nullptr;
	ProjectileSpawnShapeSnakeFragment.SnakeTrailIndex = INDEX_NONE;
	ProjectileSpawnShapeSnakeFragment.SnakeProjectiles.Empty();
	ProjectileSpawnShapeSnakeFragment.SnakeTrailLocations.Reset();
	ProjectileSpawnShapeSnakeFragment.SnakeTrailRotations.Reset();
}

void AGASCourseProjectile::ApplyProjectileMesh(UStaticMesh* Mesh)
{
	if (!IsValid(ProjectileVisualMeshComponent))
	{
		return;
	}

	// Only touch the mesh when it actually changes - SetStaticMesh dirties render state even when
	// assigning the identical asset, which a pooled projectile usually is.
	if (ProjectileVisualMeshComponent->GetStaticMesh() != Mesh)
	{
		ProjectileVisualMeshComponent->SetStaticMesh(Mesh);
	}

	if (!Mesh)
	{
		return;
	}

	ProjectileVisualMeshComponent->Activate();
	ProjectileVisualMeshComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileMeshTransformOverride);
}

void AGASCourseProjectile::ApplyProjectileTrailVFX(UNiagaraSystem* VFX)
{
	if (!IsValid(ProjectileNiagaraComponent))
	{
		return;
	}

	// ReinitializeSystem rebuilds the system and its data interface bindings, so it is only worth
	// paying when the asset genuinely changes. The teardown leaves the asset assigned and calls
	// ResetSystem, so a projectile reused for the same trail just re-activates.
	if (ProjectileNiagaraComponent->GetAsset() != VFX)
	{
		ProjectileNiagaraComponent->SetAsset(VFX);
		ProjectileNiagaraComponent->ReinitializeSystem();
	}

	if (!VFX)
	{
		return;
	}

	ProjectileNiagaraComponent->Activate();
	ProjectileNiagaraComponent->SetHiddenInGame(false);
	ProjectileNiagaraComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileTrailTransformOverride);
}

void AGASCourseProjectile::PreloadProjectileImpactAssets()
{
	TArray<FSoftObjectPath> PathsToLoad;

	if (!ProjectileVisualsFragment.ProjectileImpactVFX.IsNull())
	{
		PathsToLoad.Add(ProjectileVisualsFragment.ProjectileImpactVFX.ToSoftObjectPath());
	}

	if (!ProjectileVisualsFragment.ProjectileExpireVFX.IsNull())
	{
		PathsToLoad.Add(ProjectileVisualsFragment.ProjectileExpireVFX.ToSoftObjectPath());
	}

	if (PathsToLoad.IsEmpty())
	{
		ProjectileImpactAssetHandle.Reset();
		return;
	}

	ProjectileImpactAssetHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(PathsToLoad);
}

bool AGASCourseProjectile::InstantiateProjectileVisualFragment()
{
	if (!FindProjectileFragment(ProjectileVisualsFragment))
	{
		return false;
	}

	// Warm the hit/expire systems now so those paths can resolve them without blocking.
	PreloadProjectileImpactAssets();
	TSoftObjectPtr<UStaticMesh> ProjectileMesh = ProjectileVisualsFragment.ProjectileMesh;
	if (!ProjectileMesh.IsNull())
	{
		UStaticMesh* LoadedMesh = ProjectileMesh.Get();
		if (!LoadedMesh)
		{
			FSoftObjectPath Path = ProjectileMesh.ToSoftObjectPath();
			const TWeakObjectPtr<AGASCourseProjectile> WeakThis(this);
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([WeakThis, Path]()
				{
					AGASCourseProjectile* Projectile = WeakThis.Get();
					if (!Projectile || !IsValid(Projectile->ProjectileVisualMeshComponent))
					{
						return;
					}

					// The projectile may have been recycled while the load was in flight. Only
					// apply if this is still the asset the current activation asked for.
					if (Projectile->ProjectileVisualsFragment.ProjectileMesh.ToSoftObjectPath() != Path)
					{
						return;
					}

					if (UStaticMesh* Mesh = Cast<UStaticMesh>(Path.ResolveObject()))
					{
						Projectile->ApplyProjectileMesh(Mesh);
					}
				})
			);
		}

		else
		{
			// Asset already loaded — use immediately
			ApplyProjectileMesh(LoadedMesh);
		}
	}
	else
	{
		ApplyProjectileMesh(nullptr);
	}
	
	TSoftObjectPtr<UNiagaraSystem> TrailVFX = ProjectileVisualsFragment.ProjectileTrailVFX;
	if (!TrailVFX.IsNull())
	{
		UNiagaraSystem* LoadedVFX = TrailVFX.Get();
		if (!LoadedVFX)
		{
			// Asset not loaded yet — request async load, THEN initialize
			FSoftObjectPath Path = TrailVFX.ToSoftObjectPath();
			const TWeakObjectPtr<AGASCourseProjectile> WeakThis(this);
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([WeakThis, Path]()
				{
					AGASCourseProjectile* Projectile = WeakThis.Get();
					if (!Projectile || !IsValid(Projectile->ProjectileNiagaraComponent))
					{
						return;
					}

					if (Projectile->ProjectileVisualsFragment.ProjectileTrailVFX.ToSoftObjectPath() != Path)
					{
						return;
					}

					if (UNiagaraSystem* VFX = Cast<UNiagaraSystem>(Path.ResolveObject()))
					{
						Projectile->ApplyProjectileTrailVFX(VFX);
					}
				})
			);
		}
		else
		{
			// Asset already loaded — use immediately
			ApplyProjectileTrailVFX(LoadedVFX);
		}
	}
	else
	{
		ApplyProjectileTrailVFX(nullptr);
	}

	TSoftObjectPtr<USoundBase> ProjectileTravelSFX = ProjectileVisualsFragment.ProjectileTravelSFX;
	if (!ProjectileTravelSFX.IsNull())
	{
		USoundBase* LoadedTravelSFX = ProjectileTravelSFX.Get();
		if (!LoadedTravelSFX)
		{
			FSoftObjectPath Path = ProjectileTravelSFX.ToSoftObjectPath();
			const TWeakObjectPtr<AGASCourseProjectile> WeakThis(this);
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([WeakThis, Path]()
				{
					AGASCourseProjectile* Projectile = WeakThis.Get();
					if (!Projectile || !IsValid(Projectile->ProjectileAudioComponent))
					{
						return;
					}

					if (Projectile->ProjectileVisualsFragment.ProjectileTravelSFX.ToSoftObjectPath() != Path)
					{
						return;
					}

					if (USoundBase* SFX = Cast<USoundBase>(Path.ResolveObject()))
					{
						Projectile->ProjectileAudioComponent->SetSound(SFX);
						Projectile->ProjectileAudioComponent->Activate();
						Projectile->ProjectileAudioComponent->Play();
					}
				})
			);
		}
		else
		{
			ProjectileAudioComponent->SetSound(LoadedTravelSFX);
			ProjectileAudioComponent->Activate();
			ProjectileAudioComponent->Play();
		}
	}
	else
	{
		ProjectileAudioComponent->SetSound(nullptr);
	}
	
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileCollisionFragment()
{
	if (!FindProjectileFragment(ProjectileCollisionFragment))
	{
		return false;
	}
	
	if (!ProjectileCollisionComp)
	{
		return false;
	}
	if (ProjectileCollisionFragment.bCollisionProfileOverride && ProjectileCollisionComp->GetCollisionProfileName() != ProjectileCollisionFragment.CollisionProfileName)
	{
		ProjectileCollisionComp->SetCollisionProfileName(ProjectileCollisionFragment.CollisionProfileName);
	}
	
	if (ProjectileCollisionComp->GetUnscaledCapsuleRadius() != ProjectileCollisionFragment.ProjectileCollisionRadius
		|| ProjectileCollisionComp->GetUnscaledCapsuleHalfHeight() != ProjectileCollisionFragment.ProjectileCollisionHalfHeight)
	{
		ProjectileCollisionComp->SetCapsuleRadius(ProjectileCollisionFragment.ProjectileCollisionRadius, false);
		ProjectileCollisionComp->SetCapsuleHalfHeight(ProjectileCollisionFragment.ProjectileCollisionHalfHeight, false);
	}
	ProjectileCollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	ProjectileCollisionComp->IgnoreActorWhenMoving(this, true);
	ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndProbe);
	
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileMovementFragment()
{
	if (!FindProjectileFragment(ProjectileMovementFragment))
	{
		return false;
	}
	if (!ProjectileMovementComp)
	{
		return false;
	}
	
	ProjectileMovementComp->InitialSpeed = ProjectileMovementFragment.ProjectileInitialSpeed;
	ProjectileMovementComp->MaxSpeed = ProjectileMovementFragment.ProjectileMaxSpeed;
	
	ProjectileMovementComp->ProjectileGravityScale = ProjectileMovementFragment.bUseGravity ? ProjectileMovementFragment.GravityScale : 0.0f;
		
	const FVector Direction = GetActorTransform().GetRotation().GetForwardVector();
	FVector NewVelocity = Direction * ProjectileMovementComp->InitialSpeed;
	ProjectileMovementComp->Velocity = NewVelocity;
	ProjectileMovementComp->Activate();
	
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileParabolicMovementFragment()
{
	if (!FindProjectileFragment(ProjectileParabolicMovementFragment))
	{
		return false;
	}
	if (!ProjectileMovementComp)
	{
		return false;
	}
	
	ProjectileMovementComp->InitialSpeed = ProjectileParabolicMovementFragment.VelocityOverride.Length();
	ProjectileMovementComp->MaxSpeed = ProjectileParabolicMovementFragment.VelocityOverride.Length();
	
	ProjectileMovementComp->ProjectileGravityScale = 1.0f;
	ProjectileMovementComp->Velocity = ProjectileParabolicMovementFragment.VelocityOverride;
	ProjectileMovementComp->Activate();
	
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileRicochetFragment() 
{
	if (!FindProjectileFragment(ProjectileRicochetFragment))
	{
		return false;
	}
	if (!ProjectileRicochetFragment.bCanRicochet || !ProjectileMovementComp)
	{
		return false;
	}
	
	ProjectileMovementComp->bShouldBounce = true;
	return true;
}

void AGASCourseProjectile::TickProjectile(float DeltaTime)
{
	
	if (bOrbitRotationEnabled)
	{
		UpdateProjectileOrbitRotation(DeltaTime);
		return;
	}
	
	const bool bIsSnake = ProjectileSpawnShapeSnakeFragment.LeaderProjectile != nullptr;

	const bool bIsSnakeFollower =
		bIsSnake && !ProjectileSpawnShapeSnakeFragment.bIsLeader;
	
	if (bIsSnake && ProjectileSpawnShapeSnakeFragment.bIsLeader)
	{
		ProjectileSpawnShapeSnakeFragment.SnakeTrailLocations.Add(GetActorLocation());
		ProjectileSpawnShapeSnakeFragment.SnakeTrailRotations.Add(GetActorRotation());
		PruneSnakeTrail();
	}

	if (!bIsSnakeFollower && ProjectileMovementComp->IsActive())
	{
		ProjectileMovementComp->TickComponent(DeltaTime, ELevelTick::LEVELTICK_TimeOnly, nullptr);
		return;
	}
	TickTrailFollower(DeltaTime);
}

void AGASCourseProjectile::PruneSnakeTrail()
{
	// Keep a margin over the distance the furthest follower actually needs, so speed changes or a
	// lagging follower cannot walk off the end of the trail.
	constexpr float RetentionScale = 1.5f;
	// Only pay the front-removal memmove once this many samples have become reclaimable.
	constexpr int32 PruneSlack = 64;

	FProjectileSpawnShapeSnakeFragment& Snake = ProjectileSpawnShapeSnakeFragment;
	TArray<FVector>& Locations = Snake.SnakeTrailLocations;
	TArray<FRotator>& Rotations = Snake.SnakeTrailRotations;

	if (Locations.Num() != Rotations.Num() || Locations.Num() <= PruneSlack)
	{
		return;
	}

	// Followers sample by arc length walking back from the newest entry (see TickTrailFollower),
	// and index by ordinal rather than into these arrays, so trimming the front is safe.
	const int32 FollowerCount = FMath::Max(Snake.SnakeProjectiles.Num() - 1, 0);
	const float RequiredDistance =
		FMath::Abs(ProjectileMovementComp ? ProjectileMovementComp->InitialSpeed : 0.0f)
		* FMath::Abs(Snake.SpawnDelayBetween)
		* FollowerCount
		* RetentionScale;

	float AccumulatedDistance = 0.0f;
	int32 OldestNeededIndex = 0;

	for (int32 i = Locations.Num() - 1; i > 0; --i)
	{
		AccumulatedDistance += FVector::Distance(Locations[i], Locations[i - 1]);

		if (AccumulatedDistance >= RequiredDistance)
		{
			OldestNeededIndex = i - 1;
			break;
		}
	}

	// The loop can only break at i >= 1, so at least two samples always survive - the minimum
	// TickTrailFollower requires.
	if (OldestNeededIndex < PruneSlack)
	{
		return;
	}

	Locations.RemoveAt(0, OldestNeededIndex, EAllowShrinking::No);
	Rotations.RemoveAt(0, OldestNeededIndex, EAllowShrinking::No);
}

void AGASCourseProjectile::TickTrailFollower(float DeltaTime)
{
	const int32 ProjectileSnakeIndex = ProjectileSpawnShapeSnakeFragment.SnakeTrailIndex;

	if (ProjectileSnakeIndex < 0 || ProjectileSpawnShapeSnakeFragment.bIsLeader)
	{
		return;
	}

	AGASCourseProjectile* Leader = ProjectileSpawnShapeSnakeFragment.LeaderProjectile;
	if (!IsValid(Leader) || !IsValid(Leader->ProjectileMovementComp))
	{
		return;
	}

	const TArray<FVector>& TrailLocations =
		Leader->ProjectileSpawnShapeSnakeFragment.SnakeTrailLocations;

	const TArray<FRotator>& TrailRotations =
		Leader->ProjectileSpawnShapeSnakeFragment.SnakeTrailRotations;

	if (TrailLocations.Num() < 2 || TrailRotations.Num() != TrailLocations.Num())
	{
		return;
	}

	const float DesiredDistance =
		FMath::Abs(
			Leader->ProjectileMovementComp->InitialSpeed *
			Leader->ProjectileSpawnShapeSnakeFragment.SpawnDelayBetween *
			ProjectileSnakeIndex
		);

	float DistanceTravelled = 0.0f;

	for (int32 i = TrailLocations.Num() - 1; i > 0; --i)
	{
		const FVector& A = TrailLocations[i];
		const FVector& B = TrailLocations[i - 1];

		const float SegmentLength = FVector::Distance(A, B);

		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (DistanceTravelled + SegmentLength >= DesiredDistance)
		{
			const float Alpha =
				(DesiredDistance - DistanceTravelled) / SegmentLength;

			const FVector TargetLocation =
				FMath::Lerp(A, B, Alpha);

			const FRotator TargetRotation =
				FMath::Lerp(TrailRotations[i], TrailRotations[i - 1], Alpha);

			FHitResult SweepHit;

			SetActorLocationAndRotation(
				TargetLocation,
				TargetRotation,
				true,                  // bSweep
				&SweepHit,
				ETeleportType::None
			);

			if (SweepHit.GetActor())
			{
				OnBeginOverlap(
					ProjectileCollisionComp,
					SweepHit.GetActor(),
					SweepHit.GetComponent(),
					INDEX_NONE,
					true,
					SweepHit
				);
			}

			return;
		}

		DistanceTravelled += SegmentLength;
	}
}

void AGASCourseProjectile::UpdateProjectileOrbitRotation(float DeltaTime)
{
	AActor* OrbitTarget = GetInstigator();

	if (!IsValid(OrbitTarget))
	{
		ReturnProjectileToPool();
		return;
	}

	const FVector PreviousLocation = GetActorLocation();

	ProjectileOrbitingFragment.OrbitAngleDegrees +=
		ProjectileOrbitingFragment.OrbitSpeedDegreesPerSecond * DeltaTime;

	ProjectileOrbitingFragment.OrbitAngleDegrees =
		FMath::Fmod(ProjectileOrbitingFragment.OrbitAngleDegrees, 360.f);

	const FVector CenterLocation = OrbitTarget->GetActorLocation();

	const FVector OrbitOffset =
		FRotator(
			0.f,
			ProjectileOrbitingFragment.OrbitAngleDegrees,
			0.f)
		.RotateVector(
			FVector(
				ProjectileOrbitingFragment.OrbitRadius,
				0.f,
				ProjectileOrbitingFragment.OrbitHeightOffset));

	const FVector NewLocation = CenterLocation + OrbitOffset;

	const FVector MoveDirection = (NewLocation - PreviousLocation).GetSafeNormal();

	if (!MoveDirection.IsNearlyZero())
	{
		SetActorLocationAndRotation(
			NewLocation,
			MoveDirection.ToOrientationRotator(),
			true);
	}
	else
	{
		SetActorLocation(NewLocation, true);
	}
}

void AGASCourseProjectile::ConstructDamagePipelineHitEvent(const FProjectileHitEvent& HitEvent, FGameplayEventData& OutEventData)
{
	FGameplayTag EventTag = HitEvent.OnHitEventTag;
	OutEventData.EventTag = EventTag;
	OutEventData.OptionalObject = HitEvent.OptionalObject.IsValid() ? HitEvent.OptionalObject.Get() : this;
	
	FGameplayTagContainer InstigatorTags;
	if (UGASCourseAbilitySystemComponent* InstigatorASC = Cast<UGASCourseAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator())))
	{
		InstigatorASC->GetOwnedGameplayTags(InstigatorTags);
	}
	InstigatorTags.AppendTags(HitEvent.AdditionalInstigatorTags);
	OutEventData.InstigatorTags = InstigatorTags;
}

void AGASCourseProjectile::ApplyGameplayEffectOnProjectileEvent(EProjectileEventType EventType, const TWeakObjectPtr<AActor>& InEventTarget)
{
	if (ProjectileGameplayEffectsFragment.GameplayEffects.IsEmpty())
	{
		return;
	}
	AActor* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator)
	{
		return;
	}
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InEventTarget.Get());
	
	if (UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ProjectileInstigator))
	{
		FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
		ContextHandle.AddInstigator(ProjectileInstigator,ProjectileInstigator);
		ContextHandle.AddOrigin(ProjectileInstigator->GetActorLocation());
		ContextHandle.AddSourceObject(ProjectileInstigator);
		
		for (const FProjectileGameplayEffectData& CurrentGameplayEffect : ProjectileGameplayEffectsFragment.GameplayEffects)
		{
			if (CurrentGameplayEffect.EventType == EventType)
			{
				if (!CurrentGameplayEffect.GameplayEffect)
				{
					continue;
				}
				
				FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(CurrentGameplayEffect.GameplayEffect, 1.0f, ContextHandle);
				if (SpecHandle.IsValid())
				{
					switch (CurrentGameplayEffect.EventType)
					{
					case EProjectileEventType::OnProjectileSpawn:
					
						if (CurrentGameplayEffect.bApplyOnInstigator)
						{
							InstigatorASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else
						{
							UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Cannot apply On Spawn Gameplay Effect to target actor {0}", *GetNameSafe(InEventTarget.Get()));
						}
						break;
						
					case EProjectileEventType::OnProjectileHit:
					
						if (CurrentGameplayEffect.bApplyOnInstigator)
						{
							InstigatorASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else
						{
							InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
						}
						break;
						
					case EProjectileEventType::OnProjectileExpire:
						if (CurrentGameplayEffect.bApplyOnInstigator)
						{
							InstigatorASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else
						{
							UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Cannot apply On Spawn Gameplay Effect to target actor {0}", *GetNameSafe(InEventTarget.Get()));
						}
						break;
					case EProjectileEventType::OnProjectileRicochet:
						
						if (CurrentGameplayEffect.bApplyOnInstigator)
						{
							InstigatorASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else
						{
							InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
						}
					default:
						break;
					}
				}
			}
		}
	}
}

void AGASCourseProjectile::ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType EventType, const TWeakObjectPtr<AActor>& InEventTarget)
{
	if (ProjectileAreaOfEffectFragment.AreaOfEffectFragmentData.IsEmpty())
	{
		return;
	}
	AActor* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator)
	{
		UE_LOGFMT(LOG_GASC_Projectile, Warning, "No instigator assigned to projectile {0}", *GetNameSafe(this));
		return;
	}
	for (const FProjectileAOESpawnData& CurrentAOESpawnData : ProjectileAreaOfEffectFragment.AreaOfEffectFragmentData)
	{
		if (CurrentAOESpawnData.EventType == EventType)
		{
			if (!IsValid(CurrentAOESpawnData.AOEClass))
			{
				UE_LOGFMT(LOG_GASC_Projectile, Verbose, "No area of effect assigned to projectile event type: {0}", static_cast<int32>(EventType));
				continue;
			}
			
			if (!InEventTarget.Get())
			{
				UE_LOGFMT(LOG_GASC_Projectile, Verbose, "No target actor assigned to projectile event type: {0}", static_cast<int32>(EventType));
			}
			
			FVector CurrentLocation = GetActorLocation();
			if (AGASCourseCharacter* HitCharacter = Cast<AGASCourseCharacter>(InEventTarget.Get()))
			{
				CurrentLocation = HitCharacter->GetActorLocation();
			}
				
			FVector SpawnLocation = CurrentAOESpawnData.bSpawnAtInstigatorLocation ? ProjectileInstigator->GetActorLocation() : CurrentLocation;
			CurrentAOESpawnData.AOEClass->ProcessAreaOfEffect(ProjectileInstigator, SpawnLocation);
			break;
		}
	}
}

bool AGASCourseProjectile::InstantiateProjectileDamageFragment()
{
	if (!FindProjectileFragment(ProjectileDamageFragment))
	{
		return false;
	}

	return true;
}

bool AGASCourseProjectile::InstantiateProjectileHealingFragment()
{
	if (!FindProjectileFragment(ProjectileHealingFragment))
	{
		return false;
	}

	return true;
}

bool AGASCourseProjectile::InstantiateProjectileHomingMovementFragment()
{
	if (!FindProjectileFragment(ProjectileHomingMovementFragment))
	{
		return false;
	}
	if (!ProjectileMovementComp)
	{
		return false;
	}

	// Bezier homing takes precedence. Both fragments drive the same component state and share the
	// same two timer handles, so letting them both run leaves the bezier pass silently overwriting
	// this one and registers the target death callback twice.
	FProjectileHomingBezierMovementFragment BezierHomingFragmentProbe;
	if (FindProjectileFragment(BezierHomingFragmentProbe))
	{
		return false;
	}

	if (!ProjectileMovementComp->IsActive())
	{
		ProjectileMovementComp->Activate();
	}

	if (!ProjectileHomingMovementFragment.bUseHoming)
	{
		return false;
	}
	
	if (bOrbitRotationEnabled)
	{
		return false;
	}
	
	if (ProjectileHomingMovementFragment.HomingTarget.IsValid() || ProjectileHomingMovementFragment.HomingTargetComponent.IsValid())
	{
		TargetActor = ProjectileHomingMovementFragment.HomingTarget.Get();
		ProjectileMovementComp->bIsHomingProjectile = true;
		ProjectileMovementComp->HomingAccelerationMagnitude = ProjectileHomingMovementFragment.HomingAcceleration;
		ProjectileMovementComp->bConstrainHomingToHorizontalPlane = ProjectileHomingMovementFragment.bConstrainHomingToHorizontalPlane;
		ProjectileMovementComp->HomingTargetComponent = ProjectileHomingMovementFragment.HomingTargetComponent.IsValid() ?
			ProjectileHomingMovementFragment.HomingTargetComponent : ProjectileHomingMovementFragment.HomingTarget->GetRootComponent();
		
		if (ProjectileHomingMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::DoTThreshold)
		{
			ProjectileMovementComp->bDisableHomingBasedOnDotProduct = true;
			ProjectileMovementComp->DisableHomingDotProductMin = ProjectileHomingMovementFragment.DoTThreshold;
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingDOTCheckTimer, this, &AGASCourseProjectile::CheckDOTToTargetHoming, 0.1f, true);
		}
		if (ProjectileHomingMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::Timeout)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingTimeoutTimer, this, &AGASCourseProjectile::DisableProjectileHoming, ProjectileHomingMovementFragment.HomingTimeout, false);
		}
	}

	return RegisterTargetDeathCallback();
}

bool AGASCourseProjectile::InstantiateProjectileBezierHomingMovementFragment()
{
	if (!FindProjectileFragment(ProjectileHomingBezierMovementFragment))
	{
		return false;
	}
	if (!ProjectileMovementComp)
	{
		return false;
	}
	
	if (!ProjectileMovementComp->IsActive())
	{
		ProjectileMovementComp->Activate();
	}
	
	if (bOrbitRotationEnabled)
	{
		return false;
	}
	
	if (ProjectileHomingBezierMovementFragment.HomingTarget.IsValid() || ProjectileHomingBezierMovementFragment.HomingTargetComponent.IsValid())
	{
		TargetActor = ProjectileHomingBezierMovementFragment.HomingTarget.Get();
		ProjectileMovementComp->bUseBezierHoming = true;
		ProjectileMovementComp->bIsHomingProjectile = true;
		ProjectileMovementComp->HomingAccelerationMagnitude = ProjectileHomingBezierMovementFragment.HomingAcceleration;
		ProjectileMovementComp->HomingTargetComponent = ProjectileHomingBezierMovementFragment.HomingTargetComponent.IsValid() ?
			ProjectileHomingBezierMovementFragment.HomingTargetComponent : ProjectileHomingBezierMovementFragment.HomingTarget->GetRootComponent();
		
		ProjectileMovementComp->BezierLateralRatio = ProjectileHomingBezierMovementFragment.BezierLateralRatio;
		ProjectileMovementComp->BezierVerticalRatio = ProjectileHomingBezierMovementFragment.BezierVerticalRatio;
		ProjectileMovementComp->BezierLookAhead = ProjectileHomingBezierMovementFragment.BezierLookAhead;
		ProjectileMovementComp->BezierApproachStraightenRatio = ProjectileHomingBezierMovementFragment.BezierApproachStraightenRatio;
		ProjectileMovementComp->BezierMinCurveDistance = ProjectileHomingBezierMovementFragment.BezierMinCurveDistance;
		ProjectileMovementComp->bClampCurveToFlyableArc = ProjectileHomingBezierMovementFragment.bClampCurveToFlyableArc;
		ProjectileMovementComp->BezierCurveTolerance = ProjectileHomingBezierMovementFragment.BezierCurveTolerance;
		ProjectileMovementComp->bConstrainHomingToHorizontalPlane = ProjectileHomingBezierMovementFragment.bConstrainHomingToHorizontalPlane;

		// Alternate curve direction per projectile so grouped shots don't stack on one arc.
		const float LateralSign = (ProjectileIndex % 2 == 0) ? 1.0f : -1.0f;
		ProjectileMovementComp->InitializeBezierHoming(GetActorLocation(), LateralSign);

		if (ProjectileHomingBezierMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::DoTThreshold)
		{
			ProjectileMovementComp->bDisableHomingBasedOnDotProduct = true;
			ProjectileMovementComp->DisableHomingDotProductMin = ProjectileHomingBezierMovementFragment.DoTThreshold;
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingDOTCheckTimer, this, &AGASCourseProjectile::CheckDOTToTargetHoming, 0.1f, true);
		}
		if (ProjectileHomingBezierMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::Timeout)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingTimeoutTimer, this, &AGASCourseProjectile::DisableProjectileHoming, ProjectileHomingBezierMovementFragment.HomingTimeout, false);
		}
	}

	return RegisterTargetDeathCallback();
}

void AGASCourseProjectile::RetargetHomingToActor(AActor* NewTarget)
{
	if (!ProjectileMovementComp || !IsValid(NewTarget))
	{
		return;
	}

	USceneComponent* NewTargetComponent = NewTarget->GetRootComponent();
	if (!NewTargetComponent)
	{
		return;
	}

	// Which homing style applies is decided by the fragment this activation was given. Bezier wins
	// when both are present, matching the precedence guard in the plain homing instantiation.
	FProjectileHomingBezierMovementFragment BezierFragment;
	const bool bUsesBezierHoming = FindProjectileFragment(BezierFragment);

	FProjectileHomingMovementFragment HomingFragment;
	const bool bUsesPlainHoming = !bUsesBezierHoming
		&& FindProjectileFragment(HomingFragment)
		&& HomingFragment.bUseHoming;

	if (!bUsesBezierHoming && !bUsesPlainHoming)
	{
		// Non-homing projectile: the redirected velocity is the whole ricochet.
		return;
	}

	ProjectileMovementComp->HomingTargetComponent = NewTargetComponent;

	// Set rather than assume: if the previous target died on impact, OnTargetDeathCallback turned
	// homing off, cleared the bezier flag and reset the curve state.
	ProjectileMovementComp->bIsHomingProjectile = true;

	if (bUsesBezierHoming)
	{
		ProjectileMovementComp->bUseBezierHoming = true;
		ProjectileMovementComp->HomingAccelerationMagnitude = BezierFragment.HomingAcceleration;
		ProjectileMovementComp->BezierLateralRatio = BezierFragment.BezierLateralRatio;
		ProjectileMovementComp->BezierVerticalRatio = BezierFragment.BezierVerticalRatio;
		ProjectileMovementComp->BezierLookAhead = BezierFragment.BezierLookAhead;
		ProjectileMovementComp->BezierApproachStraightenRatio = BezierFragment.BezierApproachStraightenRatio;
		ProjectileMovementComp->BezierMinCurveDistance = BezierFragment.BezierMinCurveDistance;
		ProjectileMovementComp->bClampCurveToFlyableArc = BezierFragment.bClampCurveToFlyableArc;
		ProjectileMovementComp->BezierCurveTolerance = BezierFragment.BezierCurveTolerance;
		ProjectileMovementComp->bConstrainHomingToHorizontalPlane = BezierFragment.bConstrainHomingToHorizontalPlane;

		// The curve must be rebuilt from where the projectile is now. Keeping the original launch
		// point as P0 leaves DistanceTravelled far larger than the new chord, which pins Alpha at 1
		// and collapses the aim point onto P2 - a straight line, aimed at the old target.
		// Alternating the sign per bounce gives a zig-zag rather than repeating the same sweep.
		const float LateralSign = (ProjectileRicochetCount % 2 == 0) ? 1.0f : -1.0f;
		ProjectileMovementComp->InitializeBezierHoming(GetActorLocation(), LateralSign);
	}
	else
	{
		ProjectileMovementComp->HomingAccelerationMagnitude = HomingFragment.HomingAcceleration;
		ProjectileMovementComp->bConstrainHomingToHorizontalPlane = HomingFragment.bConstrainHomingToHorizontalPlane;
	}

	// Move the death subscription to the new target. This unregisters from the previous target's
	// ability system component first, so the projectile does not stay bound to the one it hit.
	RegisterTargetDeathCallback();
}

bool AGASCourseProjectile::RegisterTargetDeathCallback()
{
	AGASCourseCharacter* TargetCharacter = Cast<AGASCourseCharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return true;
	}

	UAbilitySystemComponent* InASC = TargetCharacter->GetAbilitySystemComponent();
	if (!InASC)
	{
		return false;
	}

	if (InASC->HasMatchingGameplayTag(Status_Death))
	{
		return true;
	}

	// Drop any previous subscription against the component it was actually made on. A pooled
	// projectile can be re-targeted, and unregistering a handle from the wrong component is a
	// silent no-op that would leave the old target permanently bound to this projectile.
	UnregisterTargetDeathCallback();

	OnTargetDeathDelegateHandle = InASC->RegisterGameplayTagEvent(FGameplayTag(Status_Death),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AGASCourseProjectile::OnTargetDeathCallback);
	OnTargetDeathRegisteredASC = InASC;

	return true;
}

void AGASCourseProjectile::UnregisterTargetDeathCallback()
{
	if (OnTargetDeathDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* RegisteredASC = OnTargetDeathRegisteredASC.Get())
		{
			RegisteredASC->UnregisterGameplayTagEvent(OnTargetDeathDelegateHandle, FGameplayTag(Status_Death), EGameplayTagEventType::NewOrRemoved);
		}
	}

	OnTargetDeathDelegateHandle.Reset();
	OnTargetDeathRegisteredASC.Reset();
}

bool AGASCourseProjectile::InstantiateProjectileGameplayEffectsFragment()
{
	if (!FindProjectileFragment(ProjectileGameplayEffectsFragment))
	{
		return false;
	}

	return true;
}

bool AGASCourseProjectile::InstantiateProjectileAreaOfEffectFragment()
{
	if (!FindProjectileFragment(ProjectileAreaOfEffectFragment))
	{
		return false;
	}

	return true;
}

bool AGASCourseProjectile::InstantiateProjectilePiercingFragment()
{
	if (!FindProjectileFragment(ProjectilePiercingFragment))
	{
		return false;
	}

	return true;
}

bool AGASCourseProjectile::InstantiateProjectileShapeSnakeFragment(const FProjectileSpawnShapeSnakeFragment& InSnakeShapeFragmentData)
{
	
	//Instantiated before the rest of the data inside of the subsystem
	ProjectileSpawnShapeSnakeFragment = InSnakeShapeFragmentData;
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileOrbitRotationFragment()
{
	if (!FindProjectileFragment(ProjectileOrbitingFragment))
	{
		return false;
	}
	UGASC_ProjectilePoolingSubsystem* PoolingSubsystem = GetWorld()->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();
	if (!PoolingSubsystem)
	{
		return false;
	}

	// Orbiting spaces projectiles evenly around their group, so it needs a group to belong to.
	// Single spawns have no group (and ProjectileIndex stays -1), which is not an error - they
	// just cannot orbit, so leave bOrbitRotationEnabled false and let normal movement apply.
	const FProjectileGroup* Group = PoolingSubsystem->GetProjectileGroup(ProjectileGroupId);
	if (!Group || Group->Projectiles.IsEmpty() || ProjectileIndex < 0)
	{
		return false;
	}

	bOrbitRotationEnabled = true;
	ProjectileOrbitingFragment.OrbitAngleDegrees = ProjectileIndex * (360.f / Group->Projectiles.Num());

	return true;
}

void AGASCourseProjectile::CheckDOTToTargetHoming()
{
	if (!ProjectileMovementComp || !ProjectileMovementComp->bDisableHomingBasedOnDotProduct)
	{
		return;
	}

	if (TargetActor)
	{
		float DotToTarget = GetHorizontalDotProductTo(TargetActor);
		if(DotToTarget <= ProjectileMovementComp->DisableHomingDotProductMin)
		{
			ProjectileMovementComp->bIsHomingProjectile = false;

			// ClearTimer, not Invalidate: this timer loops, so dropping the handle alone would
			// leave it firing for the rest of the projectile's life and defeat the ClearTimer
			// guard in ReturnProjectileToPool. ClearTimer invalidates the handle for us.
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ProjectileHomingDOTCheckTimer);
			}
		}
	}
}

void AGASCourseProjectile::DisableProjectileHoming()
{
	if (ProjectileHomingTimeoutTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ProjectileHomingTimeoutTimer);
		}
	}
	ProjectileMovementComp->bIsHomingProjectile = false;
}

// Called when the game starts or when spawned
void AGASCourseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	SetActorEnableCollision(true);
	
	// Read the subsystem's already-resolved copy rather than sync-loading the same asset once per
	// pooled projectile. The subsystem loads it before it spawns the pool, so it is ready here.
	if (const UGASC_ProjectilePoolingSubsystem* PoolingSubsystem = GetWorld()->GetSubsystem<UGASC_ProjectilePoolingSubsystem>())
	{
		RicochetTargetingPreset = PoolingSubsystem->GetRicochetTargetingPreset();
	}
}

void AGASCourseProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (ProjectileCollisionComp)
	{
		ProjectileCollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::AGASCourseProjectile::OnBeginOverlap);
	}
}

void AGASCourseProjectile::RebuildFragmentLookup()
{
	FragmentLookup.Reset();
	FragmentLookup.Reserve(ProjectileFragments.Num());

	for (int32 i = 0; i < ProjectileFragments.Num(); ++i)
	{
		if (!ProjectileFragments[i].IsValid())
		{
			continue;
		}

		// Walk the ancestor chain so a query for a base type still resolves in one lookup.
		// Chains are two or three deep, so this stays cheap.
		for (const UStruct* Current = ProjectileFragments[i].GetScriptStruct(); Current; Current = Current->GetSuperStruct())
		{
			// First fragment of a type wins, matching the old front-to-back linear scan.
			if (!FragmentLookup.Contains(Current))
			{
				FragmentLookup.Add(Current, i);
			}
		}
	}
}

template <typename T>
bool AGASCourseProjectile::FindProjectileFragment(T& OutProjectileFragment) const
{
	const int32* FragmentIndex = FragmentLookup.Find(T::StaticStruct());
	if (!FragmentIndex || !ProjectileFragments.IsValidIndex(*FragmentIndex))
	{
		return false;
	}

	if (const T* Fragment = ProjectileFragments[*FragmentIndex].GetPtr<T>())
	{
		OutProjectileFragment = *Fragment;
		return true;
	}

	return false;
}