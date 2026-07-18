// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Object.h"
#include "GASC_ProjectileEventListener.generated.h"

class AGASCourseProjectile;

/**
 * @class UGASC_ProjectileEventListener
 * @brief A class designed to handle and respond to projectile-related events.
 *
 * This class serves as an event listener for projectile events within the system.
 * It provides mechanisms for subscribing to, processing, and managing events
 * triggered by projectiles during runtime.
 *
 * Events handled by this class may include but are not limited to:
 * - Projectile launch
 * - Projectile impact
 * - Projectile destruction
 *
 * The goal of this class is to allow seamless integration of projectile behavior
 * into the broader system. It ensures that relevant systems are notified when
 * specific projectile events occur.
 *
 * The class can be extended or modified to accommodate additional event types
 * or add custom logic for handling existing events.
 */
UCLASS(Blueprintable)
class GASCOURSE_API UGASC_ProjectileEventListener : public UObject
{
	GENERATED_BODY()
	
	UGASC_ProjectileEventListener();
	~UGASC_ProjectileEventListener();

public:
	
	UFUNCTION()
	void OnListenerConstructed(TWeakObjectPtr<AActor> EventListenerInstigator);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnProjectileHit(AActor* OtherActor, FHitResult HitResult);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnProjectileRicochet(AActor* OtherActor);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnProjectileSpawned(const AActor* InstigatorActor);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnProjectileReturned(AActor* Projectile);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnEventListenerEnd();
	
	UPROPERTY()
	TArray<TObjectPtr<AGASCourseProjectile>> Projectiles;
	
	UFUNCTION()
	void OnProjectileInstantiated(const AActor* InstigatorActor);
	
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	int32 RicochetCount;
	
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	int32 HitCount;
	
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	int32 ReturnCount;
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	const AActor* OwningActor;
	
};
