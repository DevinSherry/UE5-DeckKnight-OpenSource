// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"
#include "GASC_NPC_ReactionBase.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_NPC_ReactionBase : public UGASCourseGameplayAbility
{
	GENERATED_BODY()

public:
	// Override the BlueprintNativeEvent
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability|Reaction")
	void FaceInstigator();

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Ability|Reaction")
	bool bFaceInstigatorOnReactionActivated = false;
};
