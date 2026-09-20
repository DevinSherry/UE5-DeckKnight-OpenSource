// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/ProjectileMovementComponent.h"
#include "GASCourseProjectileMovementComp.generated.h"

/**
 * 
 */
UCLASS()
class UGASCourseProjectileMovementComp : public UProjectileMovementComponent
{
	GENERATED_BODY()
	
	UGASCourseProjectileMovementComp();

public:

	/**
	 * Keep homing steering entirely in the horizontal plane, so the projectile holds the height it
	 * launched at. Applies to both plain and bezier homing.
	 *
	 * Without this a target whose component sits at a different height than the projectile - a
	 * capsule origin at chest height versus a muzzle at waist height, say - pulls the projectile up
	 * or down as it homes, because the steering simply aims at the target's location.
	 *
	 * This does NOT counteract gravity: ComputeAcceleration adds that separately, so leave
	 * bUseGravity off on the movement fragment too.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing")
	bool bConstrainHomingToHorizontalPlane = true;

	/** Gates the dot-product homing cancel. Driven from the homing fragment's DoTThreshold disable rule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing")
	bool bDisableHomingBasedOnDotProduct = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing", meta=(EditCondition="bDisableHomingBasedOnDotProduct", ClampMin="-1.0", ClampMax="1.0"))
	float DisableHomingDotProductMin = 0.0f;

	/** Steer along a cubic bezier toward the homing target instead of straight at it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier")
	bool bUseBezierHoming = false;

	// The values below are overwritten from FProjectileHomingBezierMovementFragment on every
	// activation, so these defaults and ranges only apply when authoring a projectile's component
	// directly. Kept in step with the fragment so both panels agree.

	/** Sideways bow of the curve, as a fraction of the launch-to-target distance. Negative curves the other way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming", ClampMin="-1.5", ClampMax="1.5", UIMin="-1.0", UIMax="1.0"))
	float BezierLateralRatio = 0.55f;

	/** Vertical lift of the control point. Ignored entirely while bConstrainHomingToHorizontalPlane is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming && !bConstrainHomingToHorizontalPlane", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="0.5"))
	float BezierVerticalRatio = 0.0f;

	/**
	 * How far along the REMAINING curve to aim, as a fraction. Because the curve is rebuilt from the
	 * projectile's current position each frame, this is a lead distance relative to what is left to
	 * travel, so it tightens naturally as the target is approached. Too low and the steering
	 * oscillates; too high and it cuts the corner.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming", ClampMin="0.06", ClampMax="0.5", UIMin="0.08", UIMax="0.25"))
	float BezierLookAhead = 0.10f;

	/**
	 * Fraction of the original launch distance over which the bow has fully decayed to nothing, so
	 * the final stretch is a straight run-in. This is what guarantees a head-on arrival, and it is
	 * also what makes the re-anchored curve converge instead of orbiting the target.
	 *
	 * Larger values give a longer straight approach and therefore more reliable hits; smaller values
	 * keep the arc alive closer to impact.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming", ClampMin="0.05", ClampMax="0.9", UIMin="0.15", UIMax="0.6"))
	float BezierApproachStraightenRatio = 0.35f;

	/** Launch-to-target distance below which the curve is skipped and plain homing is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming", ClampMin="0.0", UIMin="0.0", UIMax="2000.0"))
	float BezierMinCurveDistance = 200.0f;

	/**
	 * Opt-in safety net: reduce the bow toward an arc the current acceleration can hold perfectly.
	 *
	 * Off by default, and it should stay off unless a specific fast projectile misbehaves. Demanding
	 * perfect tracking removes the partially-tracked curvature that produces most of the visible
	 * arc, and because the limit scales with 1/Speed^2 it flattens the path to nothing on anything
	 * quick. Head-on arrival - the cubic's straightened approach - is what prevents overshoot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming"))
	bool bClampCurveToFlyableArc = false;

	/** Multiplier on the flyable limit. Above 1 permits an arc the projectile only partly tracks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Homing|Bezier", meta=(EditCondition="bUseBezierHoming && bClampCurveToFlyableArc", ClampMin="1.0", ClampMax="10.0", UIMin="1.0", UIMax="5.0"))
	float BezierCurveTolerance = 2.0f;

	UFUNCTION()
	void InitializeBezierHoming(const FVector& LaunchLocation, float InLateralSign = 1.0f);

	UFUNCTION()
	void ResetBezierHoming();

protected:

	virtual FVector ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const override;

	/**
	 * The unconstrained steering result - bezier when enabled, otherwise the engine's straight-line
	 * homing. ComputeHomingAcceleration wraps this and applies the horizontal-plane constraint, so
	 * the constraint covers both homing styles from one place.
	 */
	FVector ComputeHomingAccelerationInternal(const FVector& InVelocity, float DeltaTime) const;

	/**
	 * Builds the two interior control points of the cubic spanning Start to End. BowScale fades the
	 * sideways offset out over the approach; CurrentSpeed is only used by the optional flyable-arc
	 * clamp.
	 */
	void EvaluateBezierControlPoints(const FVector& Start, const FVector& End, float CurrentSpeed,
		float BowScale, FVector& OutControlPoint1, FVector& OutControlPoint2) const;

	/** Launch-to-target distance, captured once. The denominator for remaining-approach progress. */
	float BezierInitialDistance = 0.0f;
	float BezierLateralSign = 1.0f;
	bool bBezierInitialized = false;
	
};
