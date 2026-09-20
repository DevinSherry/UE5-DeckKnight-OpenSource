// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GASCourseMathLibrary.generated.h"

UENUM(BlueprintType)
enum EDirections
{
	Front,
	Right,
	Left,
	Back,
	None
};

/**
 * A math utility library for the UGASCourse project, providing various helper functions
 * and algorithms related to mathematical calculations.
 */
UCLASS()
class GASCOURSE_API UGASCourseMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculates the signed angle in degrees between two actors in a 2D plane, taking into account their positions and orientations.
	 *
	 * @param InstigatorActor The first actor used to calculate the angle.
	 * @param TargetActor The second actor used to calculate the angle.
	 * @return The signed angle in degrees between the two actors.
	 */
	UFUNCTION(BlueprintPure, Category = "GASCourse|Math|2D")
	static float GetSignedAngleBetweenActors(const AActor* InstigatorActor, const AActor* TargetActor);

	/**
	 * Determines the cardinal direction based on the provided signed angle in degrees.
	 *
	 * @param SignedAngle The signed angle in degrees to determine the direction. The angle is interpreted as follows:
	 *                    - Front: -45 to 45 degrees.
	 *                    - Right: 45 to 135 degrees.
	 *                    - Left: -45 to -135 degrees.
	 *                    - Back: Remaining angles outside the above ranges.
	 * @return The cardinal direction corresponding to the signed angle as an enum value of EDirection.
	 */
	UFUNCTION(BlueprintPure, Category = "GASCourse|Math|2D")
	static EDirections GetDirectionFromSignedAngle(float SignedAngle);
};
