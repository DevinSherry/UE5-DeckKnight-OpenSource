// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "GASCourse_ShakeCharacter.generated.h"

class UCurveFloat;
class UCurveVector;
class USkeletalMeshComponent;

GASCOURSE_API DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_ShakeCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGASCourseShakeCharacterDelegate);

/**
 * Which axes a scalar shake curve is applied to. Only meaningful for the float-curve variant;
 * the vector-curve variant supplies all three axes itself.
 */
UENUM(BlueprintType, meta = (BitFlags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EGASCourseShakeTaskAxes : uint8
{
	None	= 0 UMETA(Hidden),
	X		= 1 << 0,
	Y		= 1 << 1,
	Z		= 1 << 2,
};
ENUM_CLASS_FLAGS(EGASCourseShakeTaskAxes)

/** Number of points the debug graph samples the curve into for the current amplitude. */
#define GASCOURSE_SHAKE_DEBUG_SAMPLE_COUNT 128

/**
 * Shakes a target actor's skeletal mesh by driving its LOCAL POSITION OFFSET from an authored
 * curve. Rotation and scale are never touched.
 *
 * The shake can be defined two ways, one per factory function:
 *   - A UCurveVector, whose X/Y/Z channels are the offset directly.
 *   - A UCurveFloat plus an axis mask, where the scalar is applied to the selected axes.
 *
 * Blueprint node pins cannot use EditCondition, so the two definitions are separate entry
 * points rather than one function with a conditionally hidden mask pin. That also makes the
 * curve a required input by construction.
 *
 * Collision on the skeletal mesh is disabled for the duration and restored on completion or
 * interruption, along with the mesh's original relative location.
 */
UCLASS()
class GASCOURSE_API UGASCourse_ShakeCharacter : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

protected:

	virtual void Activate() override;

public:

	virtual void OnDestroy(bool AbilityEnded) override;

	virtual void TickTask(float DeltaTime) override;

	/** Fired once the curve has played to its end. */
	UPROPERTY(BlueprintAssignable)
	FGASCourseShakeCharacterDelegate OnShakeCompleted;

	/**
	 * Fired when the shake could not start at all - no target, no skeletal mesh, no curve, or a
	 * degenerate curve. Distinguishing this from OnShakeCompleted lets a graph react to a
	 * misconfigured shake instead of silently continuing.
	 */
	UPROPERTY(BlueprintAssignable)
	FGASCourseShakeCharacterDelegate OnShakeFailed;

	/**
	 * Shake using a vector curve, whose X/Y/Z channels drive the offset on each axis directly.
	 *
	 * @param ShakeCurve             Required. Offset per axis over time.
	 * @param ShakeDurationOverride  Meaning depends on bPingPong. Without ping-pong it scales the
	 *                               curve time axis, so the curve is stretched or compressed to
	 *                               fit. With ping-pong the curve keeps its authored speed and this
	 *                               becomes the TOTAL time the ping-pong repeats for, after which
	 *                               the task ends. -1 for neither.
	 * @param ShakeAmplitudeOverride Scales the curve value axis. -1 uses the authored values.
	 * @param ShakeAmplitudeDeviation Random amplitude fraction, clamped to 0..1; 0.2 means +/-20%.
	 *                               Sampled initially and each loop, or each full ping-pong round trip.
	 *                               Zero disables deviation. Uses the override or authored amplitude as its base.
	 * @param bLoop                  Repeat instead of playing once. Without a duration this runs
	 *                               indefinitely, holding the mesh - and so keeping its collision
	 *                               disabled - until something ends the task: StopShake, a newer
	 *                               shake on the same mesh, or the owning ability ending.
	 * @param bPingPong              Play the curve forward then backward. Use this when the curve
	 *                               does not return to its starting value, which otherwise pops at
	 *                               every loop seam. Works with or without bLoop: on its own it
	 *                               performs exactly one forward-and-back pass then ends, and with
	 *                               bLoop it repeats. A round trip takes twice the authored length.
	 * @param bDrawDebug             Draw this instance in the Shake Character debug panel, and
	 *                               open that panel automatically if it is not already open.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DisplayName = "Shake Target Character (Vector Curve)", AdvancedDisplay = "bPingPong,bDrawDebug"))
	static UGASCourse_ShakeCharacter* ShakeTargetCharacterWithVectorCurve(UGameplayAbility* OwningAbility,
		UCurveVector* ShakeCurve,
		float ShakeDurationOverride = -1.0f,
		float ShakeAmplitudeOverride = -1.0f,
		bool bLoop = false,
		bool bPingPong = false,
		bool bDrawDebug = false,
		UPARAM(meta = (ClampMin = "0.0", ClampMax = "1.0")) float ShakeAmplitudeDeviation = 0.0f);

	/**
	 * Shake using a float curve applied to the axes selected by ShakeAxesMask.
	 *
	 * @param ShakeCurve             Required. Scalar offset over time.
	 * @param ShakeAxesMask          Which axes the scalar is applied to.
	 * @param ShakeDurationOverride  Meaning depends on bPingPong. Without ping-pong it scales the
	 *                               curve time axis, so the curve is stretched or compressed to
	 *                               fit. With ping-pong the curve keeps its authored speed and this
	 *                               becomes the TOTAL time the ping-pong repeats for, after which
	 *                               the task ends. -1 for neither.
	 * @param ShakeAmplitudeOverride Scales the curve value axis. -1 uses the authored values.
	 * @param ShakeAmplitudeDeviation Random amplitude fraction, clamped to 0..1; 0.2 means +/-20%.
	 *                               Sampled initially and each loop, or each full ping-pong round trip.
	 *                               Zero disables deviation. Uses the override or authored amplitude as its base.
	 * @param bLoop                  Repeat instead of playing once. Without a duration this runs
	 *                               indefinitely, holding the mesh - and so keeping its collision
	 *                               disabled - until something ends the task: StopShake, a newer
	 *                               shake on the same mesh, or the owning ability ending.
	 * @param bPingPong              Play the curve forward then backward. Use this when the curve
	 *                               does not return to its starting value, which otherwise pops at
	 *                               every loop seam. Works with or without bLoop: on its own it
	 *                               performs exactly one forward-and-back pass then ends, and with
	 *                               bLoop it repeats. A round trip takes twice the authored length.
	 * @param bDrawDebug             Draw this instance in the Shake Character debug panel, and
	 *                               open that panel automatically if it is not already open.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DisplayName = "Shake Target Character (Float Curve)", AdvancedDisplay = "bPingPong,bDrawDebug"))
	static UGASCourse_ShakeCharacter* ShakeTargetCharacterWithFloatCurve(UGameplayAbility* OwningAbility,
		UCurveFloat* ShakeCurve,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/GASCourse.EGASCourseShakeTaskAxes"))
		int32 ShakeAxesMask,
		float ShakeDurationOverride = -1.0f,
		float ShakeAmplitudeOverride = -1.0f,
		bool bLoop = false,
		bool bPingPong = false,
		bool bDrawDebug = false,
		UPARAM(meta = (ClampMin = "0.0", ClampMax = "1.0")) float ShakeAmplitudeDeviation = 0.0f);

	/**
	 * Ends a shake early, restoring the mesh through the normal OnDestroy path. This is the only
	 * way to stop a looping shake without ending the owning ability.
	 *
	 * Deliberately does not broadcast OnShakeCompleted - the caller already knows it stopped, and
	 * that delegate is reserved for a curve reaching its natural end. Safe to call on a
	 * non-looping or already-finished task.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks")
	void StopShake();

	/**
	 * Deprecated. The shake is now curve-driven rather than frequency/amplitude driven.
	 *
	 * Retained only so existing Blueprints keep compiling instead of turning into error nodes;
	 * it cannot do anything useful because it has no curve to play. Rewire to
	 * ShakeTargetCharacterWithFloatCurve or ShakeTargetCharacterWithVectorCurve.
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DeprecatedFunction, DeprecationMessage = "Shake is now curve-driven. Use Shake Target Character (Float Curve) or (Vector Curve) and supply a curve asset."))
	static UGASCourse_ShakeCharacter* ShakeTargetCharacter(UGameplayAbility* OwningAbility,
		float ShakeFrequency,
		float ShakeAmplitude,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/GASCourse.EGASCourseShakeTaskAxes"))
		int32 ShakeAxesMask = 0);

#if !UE_BUILD_SHIPPING

	/** Every live task, for the debug panel to enumerate. Weak, so it never affects lifetime. */
	static const TArray<TWeakObjectPtr<UGASCourse_ShakeCharacter>>& GetActiveShakeTasksForDebug();

	/** True when this instance opted in, or when the CVar is forcing every instance on. */
	bool ShouldDrawDebug() const;

	/** Pre-sampled curve shape. Empty until activation. */
	const TArray<FVector>& GetDebugCurveSamples() const { return DebugCurveSamples; }

	/** 0..1 playhead into GetDebugCurveSamples. */
	float GetDebugPlayheadAlpha() const;

	/** The offset applied this frame, before being added to the cached base location. */
	FVector GetDebugCurrentOffset() const { return DebugCurrentOffset; }

	/** Current loop's signed fractional deviation from the override or authored amplitude. */
	float GetDebugCurrentAmplitudeDeviation() const
	{
		const float BaseAmplitudeScale = ShakeAmplitudeOverride > 0.0f ? ShakeAmplitudeOverride : 1.0f;
		return CurrentAmplitudeScale / BaseAmplitudeScale - 1.0f;
	}

	/** Largest absolute per-axis value across the sampled curve, for graph scaling. */
	FVector GetDebugPeakOffset() const { return DebugPeakOffset; }

	FString GetDebugTargetActorName() const;
	FString GetDebugMeshComponentName() const;

	/** Ability class that started this task, with any Blueprint "_C" suffix stripped. */
	FString GetDebugInvokingClassName() const;

	/** Full object path of the invoking ability class, for a tooltip. */
	FString GetDebugInvokingClassPath() const;

	bool IsVectorCurveMode() const { return ShakeVectorCurve != nullptr; }

	int32 GetShakeAxesMask() const { return ShakeAxesMask; }

	bool IsLooping() const { return bLoop; }

	bool IsPingPong() const { return bPingPong; }

	/** Total lifetime in seconds, or a negative value when this shake runs indefinitely. */
	float GetDebugTotalLifetime() const { return GetTotalLifetime(); }

#endif

private:

	/** Shared tail of both factory functions. */
	static UGASCourse_ShakeCharacter* CreateShakeTask(UGameplayAbility* OwningAbility,
		float InDurationOverride, float InAmplitudeOverride, bool bInLoop, bool bInPingPong, bool bInDrawDebug);

	/** Logs, broadcasts OnShakeFailed and ends. Used by every Activate() guard clause. */
	void FailAndEnd(const FString& Reason);

	/** Offset for the given curve time, before the amplitude override is applied. */
	FVector EvaluateShakeOffset(float CurveTime) const;

	/** Pick a scale for the current loop, shared by all axes and both ping-pong directions. */
	void RandomizeAmplitudeScale();

	/**
	 * One-way time the curve is played over. Ping-pong always plays the curve at its authored
	 * length, because there ShakeDurationOverride means a total lifetime rather than a time scale.
	 */
	float GetCurveCycleLength() const;

	/** Total time before the task ends itself. Negative means it never does. */
	float GetTotalLifetime() const;

	UPROPERTY()
	TObjectPtr<UCurveVector> ShakeVectorCurve = nullptr;

	UPROPERTY()
	TObjectPtr<UCurveFloat> ShakeFloatCurve = nullptr;

	UPROPERTY()
	int32 ShakeAxesMask = 0;

	/** Scales the curve time axis. Negative means "no override". */
	UPROPERTY()
	float ShakeDurationOverride = -1.0f;

	/** Multiplies the curve value. Negative means "no override". */
	UPROPERTY()
	float ShakeAmplitudeOverride = -1.0f;

	/** Fractional variation around the base amplitude, clamped to 0..1 at activation. */
	UPROPERTY()
	float ShakeAmplitudeDeviation = 0.0f;

	/** Base amplitude multiplied by the current loop's random deviation. */
	UPROPERTY()
	float CurrentAmplitudeScale = 1.0f;

	/** Repeat the curve indefinitely. The task then only ends when something else ends it. */
	UPROPERTY()
	bool bLoop = false;

	/**
	 * Reverse the curve on the way back so the motion has no seam. Independent of bLoop: alone it
	 * is a single forward-and-back pass, with bLoop it repeats.
	 */
	UPROPERTY()
	bool bPingPong = false;

	UPROPERTY()
	bool bDrawDebug = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> TargetMesh;

	/**
	 * The mesh relative location as read at activation. Never assume zero - a Character mesh
	 * carries a baked-in Z offset and -90 degree yaw from its Blueprint.
	 */
	UPROPERTY()
	FVector CachedRelativeLocation = FVector::ZeroVector;

	UPROPERTY()
	TEnumAsByte<ECollisionEnabled::Type> CachedCollisionEnabled = ECollisionEnabled::NoCollision;

	/** Guards the restore path so it is safe to run on both normal end and interruption. */
	UPROPERTY()
	bool bHasAppliedShake = false;

	UPROPERTY()
	float ElapsedTime = 0.0f;

	UPROPERTY()
	float CurveMinTime = 0.0f;

	UPROPERTY()
	float CurveMaxTime = 0.0f;

	/**
	 * The 0..1 curve position resolved by the most recent tick. Cached rather than recomputed so
	 * the debug playhead is exactly what was played - ping-pong in particular cannot be derived
	 * from ElapsedTime alone without duplicating the wrap logic.
	 */
	UPROPERTY()
	float CurrentCurveAlpha = 0.0f;

#if !UE_BUILD_SHIPPING

	void RegisterForDebug();
	void UnregisterFromDebug();
	void BuildDebugCurveSamples();

	TArray<FVector> DebugCurveSamples;
	FVector DebugCurrentOffset = FVector::ZeroVector;
	FVector DebugPeakOffset = FVector::ZeroVector;

#endif
};
