// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_NotifyState.h"
#include "GASC_Locomotion_FootPlant_Notify.generated.h"

/**
 * @class UGASC_Locomotion_FootPlant_Notify
 * @brief An animation notify class that triggers foot plant logic during an animation sequence.
 *
 * This class allows integration of foot plant mechanics within animation sequences. It provides a mechanism
 * to trace for foot placement and execute gameplay cues when a foot plant is detected.
 *
 * Features include:
 * - Customizable socket names for foot placement detection.
 * - Trace functionalities to detect valid ground placement.
 * - Gameplay cue triggering through specified tags.
 * - Debugging utilities for visualizing trace operations.
 */
UCLASS()
class GASCOURSE_API UGASC_Locomotion_FootPlant_Notify : public UAnimNotify
{
	GENERATED_BODY()

	UGASC_Locomotion_FootPlant_Notify();

public:
	
	/**
	 * @property FootPlantNotifyName
	 * @brief Name of the socket used to identify a foot plant location during the animation sequence.
	 *
	 * This property defines a custom socket name for detecting foot plant locations on the skeletal mesh.
	 * It is used during runtime to trace for valid ground placement and trigger related gameplay cues.
	 *
	 * Characteristics:
	 * - Configurable via editor or Blueprint.
	 * - Must match an existing socket on the skeletal mesh component.
	 * - Acts as the starting reference point for trace operations.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant")
	FName FootPlantNotifyName;

	/**
	 * @property TraceDistance
	 * @brief Defines the maximum distance for the foot placement trace.
	 *
	 * This variable specifies the length of the raycast (or line trace) used to determine valid ground placement
	 * during the foot plant detection process. It helps ensure proper alignment and positioning of the foot
	 * on the terrain or floor surface.
	 *
	 * Characteristics:
	 * - Measured in Unreal units.
	 * - Customizable via the editor or Blueprint.
	 * - Determines the endpoint of the trace relative to the starting socket location.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant")
	float TraceDistance = 100.0f;

	/**
	 * @property FootPlantGameplayCueTag
	 * @brief Gameplay cue tag associated with the foot plant event in the locomotion system.
	 *
	 * This variable defines the specific gameplay cue tag that gets triggered when a foot plant event occurs.
	 * It is used as part of the animation notify system to invoke gameplay logic, such as effects or actions,
	 * at the precise moment of foot contact with the ground during an animation.
	 *
	 * Characteristics:
	 * - Configurable via editor or Blueprint.
	 * - Categorized under GameplayCue for easier filtering and management.
	 * - Must reference a valid gameplay cue tag within the project to function correctly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant", meta=(Categories="GameplayCue"))
	FGameplayTag FootPlantGameplayCueTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	// UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	// End of UAnimNotify interface

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant|Debug")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant|Debug", meta = (EditCondition = "bDebug", EditConditionHides = true))
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugTraceType = EDrawDebugTrace::ForOneFrame;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Locomotion|FootPlant|Debug", meta = (EditCondition = "bDebug", EditConditionHides = true))
	float DebugDrawTime = 1.0f;

private:

	FTraceHandle TraceHandle;
	void TraceFootPlant(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum, AActor* InOwner) const;
	
};
