// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GASC_PlayerTargetingComponent.generated.h"

/**
 * @class UGASC_PlayerTargetingComponent
 * @brief A component responsible for managing the player's targeting system in the game.
 *
 * The UGASC_PlayerTargetingComponent provides functionality for handling player targeting mechanics.
 * It enables targeting other players, objects, or enemies within the game. The component is typically
 * used to locate targets based on specified criteria, manage targeting locks, and interact with gameplay
 * systems related to aiming and selection.
 *
 * Features:
 * - Target acquisition based on proximity, visibility, or other customizable rules.
 * - Management of current target selection logic.
 * - Integration with game rules for locking or switching targets.
 * - Notifications or events for when targets are acquired, lost, or interacted with.
 *
 * Usage:
 * This component is intended to be used as part of a player's actor in the game.
 * It relies on other components or systems in the game to provide environmental awareness
 * such as line-of-sight checks or object queries.
 *
 * Note:
 * Ensure that this component is properly initialized and configured before use.
 * Targeting rules and thresholds can be customized to fit the specific needs of the gameplay.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class GASCOURSE_API UGASC_PlayerTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGASC_PlayerTargetingComponent();
	
	UFUNCTION(BlueprintCallable)
	AActor* GetRecentMeleeTargetActor() {return RecentMeleeTargetActor;}
	
	UFUNCTION(BlueprintCallable)
	void UpdateRecentMeleeTarget(AActor* TargetActor);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GASCourse|Targeting|Melee")
	float RecentMeleeTargetRecentTimer = 2.0f;
	
	UFUNCTION()
	bool IsActorRecentMeleeTarget(const AActor* TargetActor) const;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY()
	AActor* RecentMeleeTargetActor;
	
	UFUNCTION()
	void ClearRecentMeleeTarget();
	
private:
	
	FTimerHandle InvalidateRecentMeleeTargetTimerHandle;
	
};
