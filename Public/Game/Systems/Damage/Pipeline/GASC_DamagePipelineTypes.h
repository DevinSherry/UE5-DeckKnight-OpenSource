// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "GASC_DamagePipelineTypes.generated.h"

/**
 * @enum EHitEventType
 * @brief Enumerates different types of hit events in a system.
 *
 * This enumeration defines the types of hit-related events that can occur, providing
 * a distinction between events where a hit is received and those where a hit is applied.
 *
 * It is commonly used in game systems, damage processing pipelines, or collision systems to
 * handle and differentiate between incoming and outgoing hit events.
 *
 * - OnHitReceived: Represents an event where a hit is received by an object or entity.
 * - OnHitApplied: Represents an event where a hit is applied to another object or entity.
 */
UENUM(BlueprintType)
enum EHitEventType
{
	OnHitReceived,
	OnHitApplied
};

/**
 * @class FHitContext
 * @brief Represents the context of a hit or interaction in a physics or collision system.
 *
 * The FHitContext class provides detailed information about a hit event, such as position,
 * direction, surface properties, and additional metadata related to the hit. It is typically
 * used in collision detection, ray tracing, or other systems where hit information must
 * be queried and processed.
 *
 * This class is often utilized in game development, simulations, and physics engines to handle
 * interactions between objects or responses to collision events.
 *
 * The specific properties and methods of this class offer access to hit data like impact point,
 * normal vector, collision object references, and other relevant details to further process
 * the hit results or apply effects.
 */

USTRUCT(Blueprintable)
struct FHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<class AActor> HitTarget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<class AActor> HitInstigator = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<class AActor> OptionalSourceObject = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	FGameplayTagContainer HitTargetTagsContainer;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	FGameplayTagContainer HitInstigatorTagsContainer;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	FGameplayTagContainer HitContextTagsContainer;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	FHitResult HitResult;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	float HitTimeStamp = 0.0f;

	FHitContext() {}
};

USTRUCT(BlueprintType)
struct FHitLogEntry
{
	GENERATED_BODY()

	FGuid HitTargetID;
	FGuid HitInstigatorID;

	uint32 HitID = 0;
	float HitTimeStamp = 0.0f;
	FHitResult HitResult;

	FString HitTargetName;
	FString HitInstigatorName;
	FString HitTargetClassName;
	FString HitInstigatorClassName;

	FGameplayTagContainer HitTargetTagsContainer;
	FGameplayTagContainer HitInstigatorTagsContainer;
	FGameplayTagContainer HitContextTagsContainer;

	FHitLogEntry() {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitApplied, const FHitContext&, OutHitContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitReceived, const FHitContext&, OutHitContext);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHitApplied_Event, const FHitContext&, OutHitContext);