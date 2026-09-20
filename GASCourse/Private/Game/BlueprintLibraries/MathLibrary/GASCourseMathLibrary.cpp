// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/BlueprintLibraries/MathLibrary/GASCourseMathLibrary.h"
#include "Kismet/KismetMathLibrary.h"

namespace {
	/**
	 * @brief Defines the threshold angle in front of the object.
	 *
	 * This constant is used as a threshold angle to determine
	 * certain calculations or conditions related to the front-facing
	 * angle of an object. The value is typically used in comparisons
	 * to decide whether an object falls within the frontal angle boundary.
	 */
	constexpr float kFrontAngleThreshold = 45.0f;
	/**
	 * @brief Represents the threshold value used to determine the angle on the sides
	 *        for a given calculation or operation.
	 *
	 * This constant is typically used to define a minimum or maximum angle, beyond
	 * which specific conditions, constraints, or behaviors might be triggered.
	 *
	 * @note Ensure the value of this constant is set appropriately to match the
	 *       requirements of the context in which it is being used.
	 */
	constexpr float kSideAngleThreshold = 135.0f;
}

/**
 * Calculates the signed angle in degrees between two actors in the world.
 * The method determines the angle formed by the two actors relative to the world
 * origin or a reference point, taking into account the directionality for a signed result.
 *
 * @param ActorA The first actor serving as the base reference for angle calculation.
 * @param ActorB The second actor relative to which the angle is calculated.
 * @return The signed angle in degrees between the two actors.
 *         The sign indicates the direction of the angle (e.g., clockwise or counter-clockwise).
 */
float UGASCourseMathLibrary::GetSignedAngleBetweenActors(const AActor* InstigatorActor, const AActor* TargetActor)
{
	// Early validation
	if (!InstigatorActor || !TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid actor parameters provided"));
		return 0.0f;
	}

	if (InstigatorActor == TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Instigator and Target are the same actor"));
		return 0.0f;
	}

	const FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector DirectionVector = UKismetMathLibrary::GetDirectionUnitVector(TargetLocation, InstigatorLocation);
	const FVector TargetForward = TargetActor->GetActorForwardVector().GetSafeNormal();

	const float DotProduct = FVector::DotProduct(DirectionVector, TargetForward);
	const float CrossProductZ = FVector::CrossProduct(DirectionVector, TargetForward).Z;
	const float AngleSign = CrossProductZ < 0.0f ? -1.0f : 1.0f;
    
	return AngleSign * FMath::RadiansToDegrees(FMath::Acos(DotProduct));

}

/**
 * Calculates the direction based on the signed angle.
 *
 * This function determines the direction (e.g., left, right, or neutral)
 * based on the provided signed angle value. A signed angle typically
 * represents angular displacement where positive values indicate
 * one direction (e.g., counterclockwise), negative values indicate the
 * opposite direction (e.g., clockwise), and zero indicates no angular
 * displacement.
 *
 * @param angle The signed angle in degrees or radians, depending on the
 * calculation context. A positive value indicates one direction, and a
 * negative value indicates the opposite direction. Zero signifies no
 * direction change.
 * @return An integer or enumeration representing the resolved direction
 * based on the signed angle.
 */
EDirections UGASCourseMathLibrary::GetDirectionFromSignedAngle(float SignedAngle)
{
	if (SignedAngle >= -kFrontAngleThreshold && SignedAngle <= kFrontAngleThreshold)
		return EDirections::Front;
    
	if (SignedAngle > kFrontAngleThreshold && SignedAngle <= kSideAngleThreshold)
		return EDirections::Left;
    
	if (SignedAngle < -kFrontAngleThreshold && SignedAngle >= -kSideAngleThreshold)
		return EDirections::Right;
    
	return EDirections::Back;

}
