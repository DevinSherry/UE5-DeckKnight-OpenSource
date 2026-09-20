// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Projectile/GASC_ProjectileEventListener.h"
#include "Game/Projectile/GASCourseProjectile.h"
#include "GASCourse/GASCourseCharacter.h"

UGASC_ProjectileEventListener::UGASC_ProjectileEventListener()
{
	UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Event Listener Created: {0}", this->GetName());
}

UGASC_ProjectileEventListener::~UGASC_ProjectileEventListener()
{
	UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Event Listener Destroyed: {0}", this->GetName());
}

void UGASC_ProjectileEventListener::OnListenerConstructed(TWeakObjectPtr<AActor> EventListenerInstigator)
{
	if (Projectiles.Num() <= 0)
	{
		return;
	}
	OwningActor = EventListenerInstigator.Get();
	for (AGASCourseProjectile* Projectile : Projectiles)
	{
		if (Projectile)
		{
			Projectile->OnProjectileCreatedDelegate.AddDynamic(this, &UGASC_ProjectileEventListener::OnProjectileSpawned);
			Projectile->OnProjectileHitDelegate.AddDynamic(this, &UGASC_ProjectileEventListener::OnProjectileHit);
			Projectile->OnProjectileReturnedToPoolDelegate.AddDynamic(this, &UGASC_ProjectileEventListener::OnProjectileReturned);
			Projectile->OnProjectileRicochetDelegate.AddDynamic(this, &UGASC_ProjectileEventListener::OnProjectileRicochet);
		}
	}
}

void UGASC_ProjectileEventListener::UnbindFromProjectiles()
{
	for (AGASCourseProjectile* Projectile : Projectiles)
	{
		if (!IsValid(Projectile))
		{
			continue;
		}

		Projectile->OnProjectileCreatedDelegate.RemoveAll(this);
		Projectile->OnProjectileHitDelegate.RemoveAll(this);
		Projectile->OnProjectileReturnedToPoolDelegate.RemoveAll(this);
		Projectile->OnProjectileRicochetDelegate.RemoveAll(this);
	}
}

void UGASC_ProjectileEventListener::OnProjectileRicochet_Implementation(AActor* OtherActor)
{
	RicochetCount++;
}

void UGASC_ProjectileEventListener::OnProjectileSpawned_Implementation(const AActor* InstigatorActor)
{
	
}

void UGASC_ProjectileEventListener::OnProjectileHit_Implementation(AActor* OtherActor, FHitResult HitResult)
{
	if (AGASCourseCharacter* HitCharacter = Cast<AGASCourseCharacter>(OtherActor))
	{
		HitCount++;
		UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Projectile Hit: {0}", HitCount);
	}
}

void UGASC_ProjectileEventListener::OnProjectileReturned_Implementation(AActor* Projectile)
{
	ReturnCount++;
	UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Projectile Returned: {0}", ReturnCount);
	if (ReturnCount >= Projectiles.Num())
	{
		UE_LOGFMT(LOG_GASC_Projectile, Verbose, "All Projectile: Returned");
		OnEventListenerEnd();
	}
}

void UGASC_ProjectileEventListener::OnEventListenerEnd_Implementation()
{
	UnbindFromProjectiles();
	Projectiles.Empty();
	MarkAsGarbage();
}

void UGASC_ProjectileEventListener::OnProjectileInstantiated(const AActor* InstigatorActor)
{
	if (InstigatorActor)
	{
		UE_LOGFMT(LOG_GASC_Projectile, Verbose, "Projectile: {0}", *InstigatorActor->GetName());
		OwningActor = InstigatorActor;
		OnProjectileSpawned(InstigatorActor);
	}
}
