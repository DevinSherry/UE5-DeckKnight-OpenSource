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
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "Engine/AssetManager.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolSettings.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_AreaOfEffectData.h"

// Sets default values
AGASCourseProjectile::AGASCourseProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetTickGroup(TG_PrePhysics);
	PrimaryActorTick.TickInterval = 0.0167;
	
	ProjectileMovementComp = CreateDefaultSubobject<UGASCourseProjectileMovementComp>(TEXT("ProjectileMovementComp"));
	ProjectileCollisionComp = CreateDefaultSubobject<USphereComponent>("ProjectileCollisionComp");
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
		if (ProjectileMovementComp->HomingTargetComponent.IsValid())
		{
			if (AGASCourseCharacter* TargetCharacter = Cast<AGASCourseCharacter>(ProjectileMovementComp->HomingTargetComponent->GetOwner()))
			{
				UAbilitySystemComponent* InASC = TargetCharacter->GetAbilitySystemComponent();
				InASC->UnregisterGameplayTagEvent(OnTargetDeathDelegateHandle, FGameplayTag(Status_Death), EGameplayTagEventType::NewOrRemoved);
			}
		}
		ProjectileMovementComp->bIsHomingProjectile = false;
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
			if (UNiagaraSystem* ProjectileExpireVFX = ProjectileVisualsFragment.ProjectileExpireVFX.LoadSynchronous())
			{
				ProjectileNiagaraComponent->SetAsset(ProjectileExpireVFX);
				ProjectileNiagaraComponent->Activate(true);
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
			DrawDebugSphere(GetWorld(), ProjectileHitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
			UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), ProjectileVisualsFragment.ImpactGameplayCueTag, Parameters);
		}
		else
		{
			if (UNiagaraSystem* ProjectileImpactVFX = ProjectileVisualsFragment.ProjectileImpactVFX.LoadSynchronous())
			{
				ProjectileNiagaraComponent->SetAsset(ProjectileImpactVFX);
				ProjectileNiagaraComponent->Activate(true);
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
	else
	{
		if(!ProjectilePiercingFragment.bCanPierce)
		{
			//Maybe not necessary right now, especially with bouncing
			ReturnProjectileToPool();
			if (ProjectileLifetimeTimer.IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(ProjectileLifetimeTimer);
				ProjectileLifetimeTimer.Invalidate();
			}
		}
		
		//Piercing Stuff
		OnProjectilePierceDelegate.Broadcast(OtherActor);
		ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectilePierce, OtherActor);
		ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectilePierce, OtherActor);
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
	
	if (InstantiateProjectileCollisionFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileMovementFragment())
	{
		// Do something later?
	}
	
	if (InstantiateProjectileParabolicMovementFragment())
	{
		// Do something later?
	}
	
	if (InstantiateProjectileHomingMovementFragment())
	{
		// Do something later?
	}
	
	if (InstantiateProjectileVisualFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileRicochetFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileDamageFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileHealingFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileGameplayEffectsFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileAreaOfEffectFragment())
	{
		// Do something later?
	}
	
	if (InstantiateProjectilePiercingFragment())
	{
		//Do something later?
	}
	
	if (InstantiateProjectileOrbitRotationFragment())
	{
		//Do something later?
	}
	
	SetActorHiddenInGame(false);
	OnProjectileCreatedDelegate.Broadcast(GetOwner());
	ApplyGameplayEffectOnProjectileEvent(EProjectileEventType::OnProjectileSpawn);
	ApplyAreaOfEffectOnProjectileEvent(EProjectileEventType::OnProjectileSpawn);
	GetWorld()->GetTimerManager().SetTimer(ProjectileLifetimeTimer, this, &AGASCourseProjectile::OnProjectileLifetimeExpired, ProjectileDataAsset->ProjectilePoolingData.LifeTime, false);
}

void AGASCourseProjectile::ReturnProjectileToPool()
{
	if (ProjectileHomingDOTCheckTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ProjectileHomingDOTCheckTimer);
		ProjectileHomingDOTCheckTimer.Invalidate();
	}
	
	if (ProjectileHomingTimeoutTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ProjectileHomingTimeoutTimer);
		ProjectileHomingTimeoutTimer.Invalidate();
	}
	
	PromoteNewSnakeLeader();
	
	if (ProjectileNiagaraComponent)
	{
		ProjectileNiagaraComponent->Deactivate();
		ProjectileNiagaraComponent->ResetSystem();
		ProjectileNiagaraComponent->SetAsset(nullptr);
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
	}
	
	if (ProjectileVisualMeshComponent)
	{
		ProjectileVisualMeshComponent->Deactivate();
	}
	
	if (ProjectileCollisionComp)
	{
		ProjectileCollisionComp->SetSphereRadius(0.0f);
		ProjectileCollisionComp->ClearMoveIgnoreActors();
		ProjectileCollisionComp->SetComponentTickEnabled(false);
		ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	ProjectileHomingMovementFragment = FProjectileHomingMovementFragment();
	ProjectileCollisionFragment = FProjectileCollisionFragment();
	ProjectileMovementFragment = FProjectileMovementFragment();
	ProjectileVisualsFragment = FProjectileVisualsFragment();
	ProjectileDamageFragment = FProjectileDamageFragment();
	ProjectileHealingFragment = FProjectileHealingFragment();
	ProjectileGameplayEffectsFragment = FProjectileGameplayEffectsFragment();
	ProjectileSpawnShapeSnakeFragment = FProjectileSpawnShapeSnakeFragment();
	ProjectileOrbitingFragment = FProjectileOrbitingFragment();
	
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
	ProjectileFragments.Empty();
	
	OnProjectileReturnedToPoolDelegate.Broadcast(this);
	
	if (UGASC_ProjectilePoolingSubsystem* PoolingSubsystem = GetWorld()->GetSubsystem<UGASC_ProjectilePoolingSubsystem>())
	{
		PoolingSubsystem->ReturnProjectileToPool(this);
	}
	else
	{
		//If the pooling system does not exist, destroy instead.
		Destroy();
		UE_LOGFMT(LogTemp, Warning, "Object Pooling subsystem not found! Destroying {0} - ", *GetNameSafe(this));
	}
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

bool AGASCourseProjectile::InstantiateProjectileVisualFragment()
{
	if (!FindProjectileFragment(ProjectileVisualsFragment))
	{
		return false;
	}
	TSoftObjectPtr<UStaticMesh> ProjectileMesh = ProjectileVisualsFragment.ProjectileMesh;
	if (!ProjectileMesh.IsNull())
	{
		UStaticMesh* LoadedMesh = ProjectileMesh.Get();
		if (!LoadedMesh)
		{
			FSoftObjectPath Path = ProjectileMesh.ToSoftObjectPath();
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([this, Path]()
				{
					UStaticMesh* Mesh = Cast<UStaticMesh>(Path.ResolveObject());
					if (Mesh && ProjectileVisualMeshComponent)
					{
						ProjectileVisualMeshComponent->SetStaticMesh(Mesh);
						ProjectileVisualMeshComponent->Activate();
						ProjectileVisualMeshComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileMeshTransformOverride);
					}
				})
			);
		}
		
		else
		{
			// Asset already loaded — use immediately
			ProjectileVisualMeshComponent->SetStaticMesh(LoadedMesh);
			ProjectileVisualMeshComponent->Activate();
			ProjectileVisualMeshComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileMeshTransformOverride);
		}
	}
	else
	{
		ProjectileVisualMeshComponent->SetStaticMesh(nullptr);
	}
	
	TSoftObjectPtr<UNiagaraSystem> TrailVFX = ProjectileVisualsFragment.ProjectileTrailVFX;
	if (!TrailVFX.IsNull())
	{
		UNiagaraSystem* LoadedVFX = TrailVFX.Get();
		if (!LoadedVFX)
		{
			// Asset not loaded yet — request async load, THEN initialize
			FSoftObjectPath Path = TrailVFX.ToSoftObjectPath();
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([this, Path]()
				{
					UNiagaraSystem* VFX = Cast<UNiagaraSystem>(Path.ResolveObject());
					if (VFX && IsValid(ProjectileNiagaraComponent))
					{
						ProjectileNiagaraComponent->SetAsset(VFX);
						ProjectileNiagaraComponent->ReinitializeSystem();
						ProjectileNiagaraComponent->Activate();
						ProjectileNiagaraComponent->SetHiddenInGame(false);
						ProjectileNiagaraComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileTrailTransformOverride);
					}
				})
			);
		}
		else
		{
			// Asset already loaded — use immediately
			ProjectileNiagaraComponent->SetAsset(LoadedVFX);
			ProjectileNiagaraComponent->ReinitializeSystem();
			ProjectileNiagaraComponent->Activate();
			ProjectileNiagaraComponent->SetHiddenInGame(false);
			ProjectileNiagaraComponent->SetRelativeTransform(ProjectileVisualsFragment.ProjectileTrailTransformOverride);
		}
	}
	else
	{
		ProjectileNiagaraComponent->SetAsset(nullptr);
	}

	TSoftObjectPtr<USoundBase> ProjectileTravelSFX = ProjectileVisualsFragment.ProjectileTravelSFX;
	if (!ProjectileTravelSFX.IsNull())
	{
		USoundBase* LoadedTravelSFX = ProjectileTravelSFX.Get();
		if (!LoadedTravelSFX)
		{
			FSoftObjectPath Path = ProjectileTravelSFX.ToSoftObjectPath();
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
				Path,
				FStreamableDelegate::CreateLambda([this, Path]()
				{
					USoundBase* SFX = Cast<USoundBase>(Path.ResolveObject());
					if (SFX && ProjectileAudioComponent)
					{
						ProjectileAudioComponent->SetSound(SFX);
						ProjectileAudioComponent->Activate();
						ProjectileAudioComponent->Play();
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
	
	if (ProjectileCollisionComp->GetUnscaledSphereRadius() != ProjectileCollisionFragment.ProjectileCollisionRadius)
	{
		ProjectileCollisionComp->SetSphereRadius(ProjectileCollisionFragment.ProjectileCollisionRadius, false);
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
	}

	if (!bIsSnakeFollower && ProjectileMovementComp->IsActive())
	{
		ProjectileMovementComp->TickComponent(DeltaTime, ELevelTick::LEVELTICK_TimeOnly, nullptr);
		return;
	}
	TickTrailFollower(DeltaTime);
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
							UE_LOGFMT(LogTemp, Warning, "Cannot apply On Spawn Gameplay Effect to target actor {0}", *GetNameSafe(InEventTarget.Get()));
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
							UE_LOGFMT(LogTemp, Warning, "Cannot apply On Spawn Gameplay Effect to target actor {0}", *GetNameSafe(InEventTarget.Get()));
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
		UE_LOGFMT(LogTemp, Warning, "No instigator assigned to projectile {0}", *GetNameSafe(this));
		return;
	}
	for (const FProjectileAOESpawnData& CurrentAOESpawnData : ProjectileAreaOfEffectFragment.AreaOfEffectFragmentData)
	{
		if (CurrentAOESpawnData.EventType == EventType)
		{
			if (!IsValid(CurrentAOESpawnData.AOEClass))
			{
				UE_LOGFMT(LogTemp, Warning, "No area of effect assigned to projectile event type: {0}", static_cast<int32>(EventType));
				continue;
			}
			
			if (!InEventTarget.Get())
			{
				UE_LOGFMT(LogTemp, Warning, "No target actor assigned to projectile event type: {0}", static_cast<int32>(EventType));
			}
				
			FVector SpawnLocation = CurrentAOESpawnData.bSpawnAtInstigatorLocation ? ProjectileInstigator->GetActorLocation() : InEventTarget.Get() ? InEventTarget->GetActorLocation() : FVector::ZeroVector;
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
		ProjectileMovementComp->HomingTargetComponent = ProjectileHomingMovementFragment.HomingTargetComponent.IsValid() ?
			ProjectileHomingMovementFragment.HomingTargetComponent : ProjectileHomingMovementFragment.HomingTarget->GetRootComponent();
		
		if (ProjectileHomingMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::DoTThreshold)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingDOTCheckTimer, this, &AGASCourseProjectile::CheckDOTToTargetHoming, 0.1f, true);
		}
		if (ProjectileHomingMovementFragment.HomingDisableRule == EProjectileHomingDisableRules::Timeout)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingTimeoutTimer, this, &AGASCourseProjectile::DisableProjectileHoming, ProjectileHomingMovementFragment.HomingTimeout, false);
		}
	}
	
	if (AGASCourseCharacter* TargetCharacter = Cast<AGASCourseCharacter>(TargetActor))
	{
		UAbilitySystemComponent* InASC = TargetCharacter->GetAbilitySystemComponent();
		if (!InASC)
		{
			return false;
		}
		
		if (!InASC->HasMatchingGameplayTag(Status_Death))
		{
			if (OnTargetDeathDelegateHandle.IsValid())
			{
				InASC->UnregisterGameplayTagEvent(OnTargetDeathDelegateHandle, FGameplayTag(Status_Death), EGameplayTagEventType::NewOrRemoved);
			}
			InASC->RegisterGameplayTagEvent(FGameplayTag(Status_Death),
				EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AGASCourseProjectile::OnTargetDeathCallback);
		}
	}
	
	return true;
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
	bOrbitRotationEnabled = true;
	
	UGASC_ProjectilePoolingSubsystem* PoolingSubsystem = GetWorld()->GetSubsystem<UGASC_ProjectilePoolingSubsystem>();
	if (!PoolingSubsystem)
	{
		return false;
	}
	int32 ProjectileGroupTotal = PoolingSubsystem->GetProjectileGroup(ProjectileGroupId).Projectiles.Num();
	ProjectileOrbitingFragment.OrbitAngleDegrees = ProjectileIndex * (360.f / ProjectileGroupTotal);
	
	return true;
}

int32 AGASCourseProjectile::FindClosestTrailIndex(const TArray<FVector>& TrailLocations, const FVector& Location)
{
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = TNumericLimits<float>::Max();

	for (int32 i = 0; i < TrailLocations.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(Location, TrailLocations[i]);

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}

	return BestIndex;
}

void AGASCourseProjectile::CheckDOTToTargetHoming()
{
	if (TargetActor)
	{
		float DotToTarget = GetHorizontalDotProductTo(TargetActor);
		if(DotToTarget <= ProjectileMovementComp->DisableHomingDotProductMin)
		{
			ProjectileMovementComp->bIsHomingProjectile = false;
			ProjectileHomingDOTCheckTimer.Invalidate();
		}
	}
}

void AGASCourseProjectile::DisableProjectileHoming()
{
	if (ProjectileHomingTimeoutTimer.IsValid())
	{
		ProjectileHomingTimeoutTimer.Invalidate();
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
	
	RicochetTargetingPreset = GetDefault<UGASC_ProjectilePoolSettings>()->RicochetTargetingPreset.LoadSynchronous();
}

void AGASCourseProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (ProjectileCollisionComp)
	{
		ProjectileCollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::AGASCourseProjectile::OnBeginOverlap);
	}
}

template <typename T>
bool AGASCourseProjectile::FindProjectileFragment(T& OutProjectileFragment) const
{
	for (const FInstancedStruct& CurrentFragment : ProjectileFragments)
	{
		if (!CurrentFragment.IsValid())
		{
			continue;
		}

		if (const T* Fragment = CurrentFragment.GetPtr<T>())
		{
			OutProjectileFragment = *Fragment;
			return true;
		}
	}

	return false;
}