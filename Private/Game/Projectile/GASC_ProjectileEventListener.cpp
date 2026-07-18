// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Projectile/GASC_ProjectileEventListener.h"
#include "Game/Projectile/GASCourseProjectile.h"
#include "GASCourse/GASCourseCharacter.h"

UGASC_ProjectileEventListener::UGASC_ProjectileEventListener()
{
	UE_LOGFMT(LogTemp, Warning, "Event Listener Created: {0}", this->GetName());
}

UGASC_ProjectileEventListener::~UGASC_ProjectileEventListener()
{
	UE_LOGFMT(LogTemp, Warning, "Event Listener Destroyed: {0}", this->GetName());
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
		UE_LOGFMT(LogTemp, Warning, "Projectile Hit: {0}", HitCount);
	}
}

void UGASC_ProjectileEventListener::OnProjectileReturned_Implementation(AActor* Projectile)
{
	ReturnCount++;
	UE_LOGFMT(LogTemp, Warning, "Projectile Returned: {0}", ReturnCount);
	if (ReturnCount >= Projectiles.Num())
	{
		UE_LOGFMT(LogTemp, Warning, "All Projectile: Returned");
		OnEventListenerEnd();
	}
}

void UGASC_ProjectileEventListener::OnEventListenerEnd_Implementation()
{
	MarkAsGarbage();
}

void UGASC_ProjectileEventListener::OnProjectileInstantiated(const AActor* InstigatorActor)
{
	if (InstigatorActor)
	{
		UE_LOGFMT(LogTemp, Warning, "Projectile: {0}", *InstigatorActor->GetName());
		OwningActor = InstigatorActor;
		OnProjectileSpawned(InstigatorActor);
	}
}
