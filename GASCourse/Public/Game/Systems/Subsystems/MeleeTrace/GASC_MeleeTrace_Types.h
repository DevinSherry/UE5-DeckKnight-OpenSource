#pragma once

#include "CoreMinimal.h"
#include "CollisionShape.h"
#include "GASC_MeleeTrace_Types.generated.h"

UENUM(BlueprintType)
enum class EGASC_MeleeSocketMode : uint8
{
	StartAndEnd,
	/** Only StartSocket is used; EndSocket is ignored. */
	SingleSocket
};

UENUM(BlueprintType)
enum class EGASC_MeleeShapeAnchor : uint8
{
	Center UMETA(DisplayName="Center (Middle)"),
	/** The shape's local -Z extremity touches the anchor point. */
	Bottom,
	/** The shape's local +Z extremity touches the anchor point. */
	Top
};

UENUM(BlueprintType)
enum class EGASC_MeleeHitPolicy : uint8
{
	UseProjectDefault,
	/** Stop the entire notify window after its first accepted contact. */
	FirstHitOnly,
	/** Each group can hit each target once during the window. */
	OncePerTarget,
	/** Each group can hit each target once per update; samples never multiply damage. */
	Unlimited,
	/** Each group can hit each target again after the configured cooldown. */
	RehitAfterDelay
};

/** Shared by every shape in a group, scoped to a single notify activation. */
struct FGASC_MeleeHitLedger
{
	TMap<int32, TMap<TWeakObjectPtr<AActor>, double>> HitTimes;
	bool bFirstHitConsumed = false;

	bool CanHit(EGASC_MeleeHitPolicy Policy, int32 Group, AActor* Target, double Now, double Cooldown) const
	{
		if (Policy == EGASC_MeleeHitPolicy::FirstHitOnly && bFirstHitConsumed) return false;
		const auto* Targets = HitTimes.Find(Group);
		const double* LastHit = Targets ? Targets->Find(Target) : nullptr;
		if (!LastHit) return true;
		if (Policy == EGASC_MeleeHitPolicy::Unlimited) return Now > *LastHit;
		if (Policy == EGASC_MeleeHitPolicy::RehitAfterDelay)
			return Now > *LastHit && Now - *LastHit >= FMath::Max(0.0, Cooldown);
		return false;
	}

	void Record(int32 Group, AActor* Target, double Now)
	{
		HitTimes.FindOrAdd(Group).Add(Target, Now);
		bFirstHitConsumed = true;
	}
};

namespace GASC_MeleeTrace
{
	inline FVector ShapeAnchorOffset(const FCollisionShape& Shape, EGASC_MeleeShapeAnchor Anchor)
	{
		const float HalfHeight = Shape.IsSphere() ? Shape.GetSphereRadius() :
			Shape.IsCapsule() ? Shape.GetCapsuleHalfHeight() : Shape.GetExtent().Z;
		return FVector(0, 0, Anchor == EGASC_MeleeShapeAnchor::Bottom ? HalfHeight :
			Anchor == EGASC_MeleeShapeAnchor::Top ? -HalfHeight : 0.f);
	}
	/** Interpolate in socket space so points along a rotating weapon follow its arc. */
	inline FTransform InterpolateSocket(const FTransform& Previous, const FTransform& Current, float Alpha)
	{
		return FTransform(FQuat::Slerp(Previous.GetRotation(), Current.GetRotation(), Alpha).GetNormalized(),
			FMath::Lerp(Previous.GetLocation(), Current.GetLocation(), Alpha), FVector::OneVector);
	}

	inline FVector SampleSocketSegment(const FTransform& Socket, const FVector& EndLocal, const FVector& Offset, float Alpha)
	{
		return Socket.TransformPositionNoScale(EndLocal * Alpha + Offset);
	}

	inline bool PreferHit(int32 Rank, float Time, int32 ShapeIndex, int32 OtherRank, float OtherTime, int32 OtherShapeIndex)
	{
		return Rank < OtherRank || (Rank == OtherRank && (Time < OtherTime || (Time == OtherTime && ShapeIndex < OtherShapeIndex)));
	}
}
