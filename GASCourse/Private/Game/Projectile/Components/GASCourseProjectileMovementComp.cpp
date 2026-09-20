// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Projectile/Components/GASCourseProjectileMovementComp.h"

UGASCourseProjectileMovementComp::UGASCourseProjectileMovementComp()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASCourseProjectileMovementComp::InitializeBezierHoming(const FVector& LaunchLocation, float InLateralSign)
{
	BezierLateralSign = FMath::Sign(InLateralSign) != 0.0f ? FMath::Sign(InLateralSign) : 1.0f;
	BezierInitialDistance = HomingTargetComponent.IsValid()
		? FVector::Dist(LaunchLocation, HomingTargetComponent->GetComponentLocation())
		: 0.0f;
	bBezierInitialized = BezierInitialDistance > KINDA_SMALL_NUMBER;
}

void UGASCourseProjectileMovementComp::ResetBezierHoming()
{
	BezierInitialDistance = 0.0f;
	BezierLateralSign = 1.0f;
	bBezierInitialized = false;
}

FVector UGASCourseProjectileMovementComp::ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const
{
	const FVector HomingAcceleration = ComputeHomingAccelerationInternal(InVelocity, DeltaTime);

	if (!bConstrainHomingToHorizontalPlane)
	{
		return HomingAcceleration;
	}

	// Drop the vertical component, then restore the original magnitude: simply zeroing Z would also
	// weaken the horizontal steering whenever the target sits at a different height.
	const FVector FlattenedAcceleration(HomingAcceleration.X, HomingAcceleration.Y, 0.0f);
	if (FlattenedAcceleration.IsNearlyZero())
	{
		// Target is directly above or below - there is no horizontal direction to steer in.
		return FVector::ZeroVector;
	}

	return FlattenedAcceleration.GetSafeNormal() * HomingAcceleration.Size();
}

FVector UGASCourseProjectileMovementComp::ComputeHomingAccelerationInternal(const FVector& InVelocity, float DeltaTime) const
{
	if (!bUseBezierHoming || !bBezierInitialized)
	{
		return Super::ComputeHomingAcceleration(InVelocity, DeltaTime);
	}

	// Point blank shots get plain homing: below this range there is not enough distance to turn
	// through an arc, so any curve just throws the projectile sideways and it has to recover.
	// Gated on the launch distance, not the live one, so the curve cannot switch off part way
	// through a shot and put a visible kink in the path.
	if (BezierInitialDistance < BezierMinCurveDistance)
	{
		return Super::ComputeHomingAcceleration(InVelocity, DeltaTime);
	}

	const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();

	FVector TargetLocation = HomingTargetComponent->GetComponentLocation();
	if (bConstrainHomingToHorizontalPlane)
	{
		// Flatten the chord onto the projectile's own plane so the curve's geometry - bow width and
		// remaining distance - is measured across the ground rather than along a tilted line. The
		// wrapper strips Z from the result anyway; doing it here keeps the shape correct too.
		TargetLocation.Z = CurrentLocation.Z;
	}

	// The curve spans the projectile's CURRENT position to the target's CURRENT position, rebuilt
	// every frame. The endpoint therefore drives the shape directly, and any tracking error is
	// absorbed rather than accumulated - there is no stale launch anchor for the path to drift from.
	//
	// Progress comes from how much of the original distance is left, and it is the only reason this
	// converges: re-anchoring alone would present a full-strength bow forever, so the projectile
	// would spiral around the target and never close. Decaying the bow turns that into a spiral-in.
	const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
	const float RemainingFraction = (BezierInitialDistance > KINDA_SMALL_NUMBER)
		? FMath::Clamp(DistanceToTarget / BezierInitialDistance, 0.0f, 1.0f)
		: 0.0f;

	// Bow fades to nothing across the last BezierApproachStraightenRatio of the approach, which is
	// what forces the head-on arrival that keeps the projectile from crossing past the target.
	const float StraightenRatio = FMath::Clamp(BezierApproachStraightenRatio, 0.0f, 0.95f);
	const float BowScale = FMath::Clamp(
		(RemainingFraction - StraightenRatio) / (1.0f - StraightenRatio), 0.0f, 1.0f);

	FVector ControlPoint1;
	FVector ControlPoint2;
	EvaluateBezierControlPoints(CurrentLocation, TargetLocation, InVelocity.Size(), BowScale, ControlPoint1, ControlPoint2);

	// Sampled a short way along what remains, not at an absolute progress point, because the curve
	// now starts where the projectile already is.
	const float SampleAlpha = FMath::Clamp(BezierLookAhead, 0.01f, 1.0f);

	// Cubic: (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
	const float OneMinus = 1.0f - SampleAlpha;
	const FVector AimPoint = (OneMinus * OneMinus * OneMinus * CurrentLocation)
		+ (3.0f * OneMinus * OneMinus * SampleAlpha * ControlPoint1)
		+ (3.0f * OneMinus * SampleAlpha * SampleAlpha * ControlPoint2)
		+ (SampleAlpha * SampleAlpha * SampleAlpha * TargetLocation);

	const FVector ToAimPoint = AimPoint - CurrentLocation;
	if (ToAimPoint.IsNearlyZero())
	{
		return Super::ComputeHomingAcceleration(InVelocity, DeltaTime);
	}

	return ToAimPoint.GetSafeNormal() * HomingAccelerationMagnitude;
}

void UGASCourseProjectileMovementComp::EvaluateBezierControlPoints(const FVector& Start, const FVector& End,
	float CurrentSpeed, float BowScale, FVector& OutControlPoint1, FVector& OutControlPoint2) const
{
	// Degenerate chord: collapse both control points so the cubic reduces to a straight line.
	OutControlPoint1 = Start;
	OutControlPoint2 = End;

	const FVector Chord = End - Start;
	const float ChordLength = Chord.Size();
	if (ChordLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector ChordDir = Chord / ChordLength;

	// Perpendicular in the horizontal plane; fall back to an arbitrary axis for a near-vertical chord.
	FVector Side = FVector::CrossProduct(ChordDir, FVector::UpVector);
	if (!Side.Normalize())
	{
		Side = FVector::CrossProduct(ChordDir, FVector::ForwardVector).GetSafeNormal();
	}

	// Optionally reduce the bow toward an arc the projectile can hold perfectly. Peak deviation is
	// about (4/9 * Ratio * ChordLength), needing roughly (4 * Ratio * Speed^2 / ChordLength) of
	// lateral acceleration; inverting that gives the flyable ratio.
	//
	// Off by default and tolerance-scaled when on, because the limit falls with 1/Speed^2: applied
	// strictly it leaves a fast projectile following a near-straight line perfectly instead of
	// visibly bending through an arc it only partly tracks. The partial tracking IS the curve.
	float RatioLimit = TNumericLimits<float>::Max();
	if (bClampCurveToFlyableArc && CurrentSpeed > KINDA_SMALL_NUMBER && HomingAccelerationMagnitude > KINDA_SMALL_NUMBER)
	{
		RatioLimit = FMath::Max(BezierCurveTolerance, 1.0f)
			* (HomingAccelerationMagnitude * ChordLength) / (4.0f * CurrentSpeed * CurrentSpeed);
	}

	const float ClampedBowScale = FMath::Clamp(BowScale, 0.0f, 1.0f);
	const float EffectiveLateral = ClampedBowScale * FMath::Sign(BezierLateralRatio)
		* FMath::Min(FMath::Abs(BezierLateralRatio), RatioLimit);

	// No vertical bow at all when constrained to the horizontal plane, whatever the ratio says.
	const float EffectiveVertical = bConstrainHomingToHorizontalPlane
		? 0.0f
		: ClampedBowScale * FMath::Sign(BezierVerticalRatio)
			* FMath::Min(FMath::Abs(BezierVerticalRatio), RatioLimit);

	// Standard thirds placement. Only the first control point carries the bow; the second sits on
	// the chord line, which forces the end tangent - 3 * (End - P2) - to point straight at the
	// target. A quadratic could not express that: its end tangent was always atan(2 * Ratio)
	// off-axis, so the projectile reached the target still carrying sideways velocity and crossed
	// past it. Straightening is now handled twice over, by this and by BowScale reaching zero.
	OutControlPoint1 = Start + (Chord * (1.0f / 3.0f))
		+ (Side * BezierLateralSign * EffectiveLateral * ChordLength)
		+ (FVector::UpVector * EffectiveVertical * ChordLength);

	OutControlPoint2 = End - (Chord * (1.0f / 3.0f));
}
