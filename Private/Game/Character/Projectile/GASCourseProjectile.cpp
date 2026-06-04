// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Projectile/GASCourseProjectile.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameplayCueManager.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Game/Character/Projectile/Components/GASCourseProjectileMovementComp.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"
#include "Game/Character/Projectile/GASC_ProjectileData.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "Game/Systems/Subsystems/ProjectilePooling/GASC_ProjectilePoolingSubsystem.h"
#include "Kismet/GameplayStatics.h"

//TODO: Ricochet is broken! Fix this.

// Sets default values
AGASCourseProjectile::AGASCourseProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetTickGroup(TG_PostPhysics);
	
	ProjectileMovementComp = CreateDefaultSubobject<UGASCourseProjectileMovementComp>(TEXT("ProjectileMovementComp"));
	ProjectileCollisionComp = CreateDefaultSubobject<USphereComponent>("ProjectileCollisionComp");
	ProjectileVisualMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileVisualMeshComponent");
	ProjectileAudioComponent = CreateDefaultSubobject<UAudioComponent>("ProjectileAudioComponent");
	ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("ProjectileNiagaraComponent");

	SetRootComponent(ProjectileCollisionComp);
	ProjectileVisualMeshComponent->SetupAttachment(ProjectileCollisionComp);
	ProjectileAudioComponent->SetupAttachment(ProjectileCollisionComp);
	ProjectileNiagaraComponent->SetupAttachment(ProjectileCollisionComp);

	ProjectileCollisionComp->SetCollisionProfileName("Projectile");
	ProjectileCollisionComp->SetEnableGravity(false);
	ProjectileVisualMeshComponent->SetAutoActivate(false);
	
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	
	/**
	 * 
	 * Common properties of the projectile visual mesh component.
	 */
	ProjectileVisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileVisualMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileVisualMeshComponent->SetGenerateOverlapEvents(false);
	ProjectileVisualMeshComponent->SetAutoActivate(false);
	
	/** *
	 * Common properties of the projectile Niagara component.
	 */
	ProjectileNiagaraComponent->SetAutoActivate(false);
	
	//TODO: Add common properties for the audio component
	ProjectileAudioComponent->SetAutoActivate(false);
}

bool AGASCourseProjectile::CanProjectileRicochet_Implementation()
{
	return false;
	//return CanProjectileRicochet();
}

bool AGASCourseProjectile::ApplyDamagetoTargetOnHit_Implementation(AActor* InHitActor, const FHitResult& InHitResult)
{
	return true;
}

bool AGASCourseProjectile::ApplyHealingtoTargetOnHit_Implementation(AActor* InHitActor, const FHitResult& InHitResult)
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

void AGASCourseProjectile::EventProcessDamageEvent_Implementation(AActor* InHitActor, const FHitResult& InHitResult)
{
	if (IsActorAnAlly(InHitActor))
	{
		if (bHealsAllies)
		{
			ApplyHealingtoTargetOnHit(InHitActor, InHitResult);
		}
		if (bCanDamageAllies)
		{
			ApplyDamagetoTargetOnHit(InHitActor, InHitResult);
		}
	}
	else
	{
		if (bCanHealEnemies)
		{
			ApplyHealingtoTargetOnHit(InHitActor, InHitResult);
		}
		else
		{
			ApplyDamagetoTargetOnHit(InHitActor, InHitResult);
		}
	}
}

void AGASCourseProjectile::OnProjectileHitEvent_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                                               UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	HitTargetActor = OtherActor;
	HitResult = Hit;
	EventProcessDamageEvent_Implementation(OtherActor, Hit);
}

void AGASCourseProjectile::OnProjectileRicochet_Implementation()
{
}

void AGASCourseProjectile::ApplyDamagePipelineToHitTarget(const FHitResult& InHitResult) const
{
	AActor* HitActor = InHitResult.GetActor();
	if (!HitActor)
	{
		return;
	}
	
	if (GetWorld() == nullptr)
	{
		return;
	}
	
	const bool bCanDamageTarget = IsActorAnAlly(HitActor) ? ProjectileDamagePipelineData.DamageData.bCanDamageAllies : true;
	const bool bCanHealTarget = IsActorAnAlly(HitActor) ? true : ProjectileDamagePipelineData.HealingData.bCanHealEnemies;
	
	UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = GetWorld()->GetSubsystem<UGASC_DamagePipelineSubsystem>();
	if (!DamagePipelineSubsystem)
	{
		return;
	}
	
	if (ProjectileDamagePipelineData.DamageData.Damage > 0.0f)
	{
		if (bCanDamageTarget)
		{
			float InDamage = ProjectileDamagePipelineData.DamageData.Damage;
			FGameplayTag InDamageTypeTag = ProjectileDamagePipelineData.DamageData.DamageType;
			FGameplayTagContainer InDamageGrantedTags = ProjectileDamagePipelineData.DamageData.DamageGrantedTags;
			FDamagePipelineContext DamagePipelineContext;
			DamagePipelineContext.DamageType = InDamageTypeTag;
			DamagePipelineContext.GrantedTags = InDamageGrantedTags;
			DamagePipelineContext.HitResult = InHitResult;
		
			if (ProjectileDamagePipelineData.DamageData.bDamageOverTime)
			{
				FDamagePipelineEffectOverTimeContext OverTimeContext = ProjectileDamagePipelineData.DamageData.EffectOverTimeContext;
				DamagePipelineSubsystem->ApplyDamageOverTimeToTarget(HitActor, GetInstigator(), InDamage, DamagePipelineContext, OverTimeContext);
			}
			else
			{
				DamagePipelineSubsystem->ApplyDamageToTarget(HitActor, GetInstigator(), InDamage, DamagePipelineContext);
			}
		}
	}
	
	if (ProjectileDamagePipelineData.HealingData.Healing > 0.0f)
	{
		if (bCanHealTarget)
		{
			float InHealing = ProjectileDamagePipelineData.HealingData.Healing;
			FGameplayTag InHealingTypeTag = ProjectileDamagePipelineData.HealingData.HealingType;
			FGameplayTagContainer InHealingGrantedTags = ProjectileDamagePipelineData.HealingData.HealingGrantedTags;
			FDamagePipelineContext HealingPipelineContext;
			HealingPipelineContext.DamageType = InHealingTypeTag;
			HealingPipelineContext.GrantedTags = InHealingGrantedTags;
			HealingPipelineContext.HitResult = InHitResult;
		
			if (ProjectileDamagePipelineData.HealingData.bHealOverTime)
			{
				FDamagePipelineEffectOverTimeContext OverTimeContext = ProjectileDamagePipelineData.HealingData.EffectOverTimeContext;
				DamagePipelineSubsystem->ApplyHealOverTimeToTarget(HitActor, GetInstigator(), InHealing, HealingPipelineContext, OverTimeContext);
			}
			else
			{
				DamagePipelineSubsystem->ApplyHealToTarget(HitActor, GetInstigator(), InHealing, HealingPipelineContext);
			}
		}
	}
}

void AGASCourseProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                 FVector NormalImpulse, const FHitResult& Hit)
{
	HitTargets.Add(OtherActor);

	ProjectileCollisionComp->IgnoreActorWhenMoving(OtherActor, true);
	ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//TODO: WHY DOES GAME CRASH WHEN HITTING OBJECT/LANDSCAPE?
	if (bCanRicochet)
	{
		if(UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetInstigator()->GetWorld()))
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

	//TODO: Consider removing, might be redundant in new model
	OnProjectileHitEvent(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
	
	OnProjectileHitDelegate.Broadcast(Hit);
	OnProjectileHit(Hit);
}

void AGASCourseProjectile::OnTargetRequestCompleted(FTargetingRequestHandle TargetingRequestHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("TargetRequestCompleted"));

	if(UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetInstigator()->GetWorld()))
	{
		TargetingSubsystem->GetTargetingResultsActors(TargetingRequestHandle, FoundTargets);
		if(FoundTargets.IsEmpty())
		{
			Destroy();
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
			Destroy();
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
			Destroy();
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
		if (ProjectileDataAsset->ProjectileVisualData.ExpireGameplayCueTag.IsValid())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = GetActorLocation();
			Parameters.Instigator = GetOwner();
			Parameters.EffectCauser = GetOwner();
			
			UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), ProjectileDataAsset->ProjectileVisualData.ExpireGameplayCueTag, Parameters);
		}
		else
		{
			if (UNiagaraSystem* ProjectileExpireVFX = ProjectileDataAsset->ProjectileVisualData.ProjectileExpireVFX.LoadSynchronous())
			{
				ProjectileNiagaraComponent->SetAsset(ProjectileExpireVFX);
				ProjectileNiagaraComponent->Activate();
			}
		}
	}
	
	//Handle object pooling return logic in separate function?
	ReturnProjectileToPool();
}

void AGASCourseProjectile::OnProjectileHit(const FHitResult& InHitResult)
{
	if (ProjectileLifetimeTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ProjectileLifetimeTimer);
		ProjectileLifetimeTimer.Invalidate();
	}
	
	if (ProjectileDataAsset)
	{
		if (ProjectileDataAsset->ProjectileVisualData.ImpactGameplayCueTag.IsValid())
		{
			FGameplayCueParameters Parameters = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(InHitResult);
			UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), ProjectileDataAsset->ProjectileVisualData.ImpactGameplayCueTag, Parameters);
		}
		else
		{
			if (UNiagaraSystem* ProjectileImpactVFX = ProjectileDataAsset->ProjectileVisualData.ProjectileImpactVFX.LoadSynchronous())
			{
				ProjectileNiagaraComponent->SetAsset(ProjectileImpactVFX);
				ProjectileNiagaraComponent->Activate();
			}
		}
	}
	
	ApplyDamagePipelineToHitTarget(InHitResult);
	
	//Maybe not necessary right now, especially with bouncing
	ReturnProjectileToPool();
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
	
	if (InstantiateProjectileVisualData(ProjectileDataAsset->ProjectileVisualData))
	{
		//Do something later?
	}
	
	if (InstantiateProjectileCollisionData(ProjectileDataAsset->ProjectileCollisionData))
	{
		//Do something later?
	}
	
	if (InstantiateProjectileMovementData(ProjectileDataAsset->ProjectileMovementData))
	{
		// Do something later?
	}
	
	if (InstantiateProjectileHomingMovementData(ProjectileHomingMovementData))
	{
		// Do something later?
	}
	
	/** Instantiate base collision stuff here, outside of external data parameters */
	ProjectileCollisionComp->SetNotifyRigidBodyCollision(true);
	ProjectileCollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),AGASCourseCharacter::StaticClass(),FoundActors);
	for (AActor* Actor : FoundActors)
	{
		const bool bCanDamageTarget = IsActorAnAlly(Actor) ? ProjectileDamagePipelineData.DamageData.bCanDamageAllies : ProjectileDamagePipelineData.DamageData.Damage > 0.0f;
		const bool bCanHealTarget = IsActorAnAlly(Actor) ? ProjectileDamagePipelineData.HealingData.Healing > 0.0f : ProjectileDamagePipelineData.HealingData.bCanHealEnemies;
		if (!bCanDamageTarget && !bCanHealTarget)
		{
			ProjectileCollisionComp->IgnoreActorWhenMoving(Actor, true);
		}
	}
	
	if (!ProjectileCollisionComp->OnComponentHit.IsBound())
	{
		ProjectileCollisionComp->OnComponentHit.AddDynamic(this, &ThisClass::AGASCourseProjectile::OnHit);
	}
	
	SetActorEnableCollision(true);
	
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
	}
	
	if (ProjectileVisualMeshComponent)
	{
		ProjectileVisualMeshComponent->SetStaticMesh(nullptr);
		ProjectileVisualMeshComponent->Deactivate();
	}
	
	if (ProjectileCollisionComp)
	{
		ProjectileCollisionComp->SetSphereRadius(0.0f);
		ProjectileCollisionComp->ClearMoveIgnoreActors();
		//ProjectileCollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
		ProjectileCollisionComp->Deactivate();
	}

	SetOwner(nullptr);
	SetInstigator(nullptr);
	HitTargets.Empty();
	
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

bool AGASCourseProjectile::InstantiateProjectileVisualData(const FProjectileVisualData& InProjectileVisualData)
{
	if (InProjectileVisualData.ProjectileMesh)
	{
		UStaticMesh* ProjectileMesh = InProjectileVisualData.ProjectileMesh.LoadSynchronous();
		ProjectileVisualMeshComponent->SetStaticMesh(ProjectileMesh);
		ProjectileVisualMeshComponent->SetRelativeTransform(InProjectileVisualData.ProjectileMeshTransformOverride);
		ProjectileVisualMeshComponent->Activate();
	}
	if (InProjectileVisualData.ProjectileTrailVFX)
	{
		UNiagaraSystem* ProjectileTrailSystem = InProjectileVisualData.ProjectileTrailVFX.LoadSynchronous();
		ProjectileNiagaraComponent->SetAsset(ProjectileTrailSystem);
		ProjectileNiagaraComponent->SetRelativeTransform(InProjectileVisualData.ProjectileTrailTransformOverride);
		ProjectileNiagaraComponent->Activate();
	}
	
	if (InProjectileVisualData.ProjectileTravelSFX)
	{
		USoundBase* ProjectileTravelSFX = InProjectileVisualData.ProjectileTravelSFX.LoadSynchronous();
		ProjectileAudioComponent->SetSound(ProjectileTravelSFX);
		ProjectileAudioComponent->Activate();
		ProjectileAudioComponent->Play();
	}
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileCollisionData(const FProjectileCollisionData& InProjectileCollisionData)
{
	if (!ProjectileCollisionComp)
	{
		return false;
	}
	if (InProjectileCollisionData.bCollisionProfileOverride)
	{
		ProjectileCollisionComp->SetCollisionProfileName(InProjectileCollisionData.CollisionProfileName);
	}
	ProjectileCollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProjectileCollisionComp->SetNotifyRigidBodyCollision(true);
	ProjectileCollisionComp->SetSphereRadius(InProjectileCollisionData.ProjectileCollisionRadius);
	
	ProjectileCollisionComp->Activate();
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileMovementData(const FProjectileMovementData& InProjectileMovementData)
{
	if (!ProjectileMovementComp)
	{
		return false;
	}
	
	ProjectileMovementComp->InitialSpeed =
		ProjectileDataAsset->ProjectileMovementData.ProjectileInitialSpeed;
	ProjectileMovementComp->MaxSpeed =
		ProjectileDataAsset->ProjectileMovementData.ProjectileMaxSpeed;
	
	/** consider adding parameter later, TODO*/
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	
	ProjectileMovementComp->ProjectileGravityScale = ProjectileDataAsset->ProjectileMovementData.bUseGravity ? 
		ProjectileDataAsset->ProjectileMovementData.GravityScale : 0.0f;
	
		
	const FVector Direction = GetActorTransform().GetRotation().GetForwardVector();
	FVector NewVelocity = Direction * ProjectileMovementComp->InitialSpeed;
	ProjectileMovementComp->Velocity = NewVelocity;
	ProjectileMovementComp->SetComponentTickEnabled(true);
	
	ProjectileMovementComp->Activate();
	
	return true;
}

bool AGASCourseProjectile::InstantiateProjectileHomingMovementData(const FProjectileHomingMovementData& InProjectileHomingMovementData)
{
	if (!ProjectileMovementComp)
	{
		return false;
	}
	
	if (!ProjectileMovementComp->IsActive())
	{
		ProjectileMovementComp->Activate();
	}
	
	ProjectileHomingMovementData = InProjectileHomingMovementData;
	
	if (!ProjectileHomingMovementData.bUseHoming)
	{
		return false;
	}
	
	if (ProjectileHomingMovementData.HomingTarget.IsValid() || ProjectileHomingMovementData.HomingTargetComponent.IsValid())
	{
		TargetActor = ProjectileHomingMovementData.HomingTarget.Get();
		ProjectileMovementComp->bIsHomingProjectile = true;
		ProjectileMovementComp->HomingAccelerationMagnitude = ProjectileHomingMovementData.HomingAcceleration;
		ProjectileMovementComp->HomingTargetComponent = ProjectileHomingMovementData.HomingTargetComponent.IsValid() ?
			ProjectileHomingMovementData.HomingTargetComponent : ProjectileHomingMovementData.HomingTarget->GetRootComponent();
		
		if (ProjectileHomingMovementData.HomingDisableRule == EProjectileHomingDisableRules::DoTThreshold)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingDOTCheckTimer, this, &AGASCourseProjectile::CheckDOTToTargetHoming, 0.1f, true);
		}
		if (ProjectileHomingMovementData.HomingDisableRule == EProjectileHomingDisableRules::Timeout)
		{
			GetWorld()->GetTimerManager().SetTimer(ProjectileHomingTimeoutTimer, this, &AGASCourseProjectile::DisableProjectileHoming, ProjectileHomingMovementData.HomingTimeout, false);
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

	if(!bCanRicochet || !CanProjectileRicochet())
	{
		if(AGASCourseCharacter* InstigatorActor = Cast<AGASCourseCharacter>(GetInstigator()))
		{
			FGameplayTagContainer OwnedTags = InstigatorActor->GetAbilitySystemComponent()->GetOwnedGameplayTags();
			bCanRicochet = OwnedTags.MatchesQuery(RicochetTagRequirements);
		}
	}

	ProjectileMovementComp->bShouldBounce = bCanRicochet;
	ProjectileCollisionComp->IgnoreActorWhenMoving(GetInstigator(), true);
}