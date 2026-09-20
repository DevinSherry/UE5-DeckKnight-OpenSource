// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/Gameplay/GASCourse_ShakeCharacter.h"

#include "Abilities/GameplayAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LOG_GASC_ShakeCharacter);

namespace GASCourse_ShakeCharacterCVars
{
#if !UE_BUILD_SHIPPING
	static TAutoConsoleVariable<bool> CvarEnableShakeCurveGraph(TEXT("GASCourseDebug.ShakeCharacter.CurveGraph"),
		false,
		TEXT("Draw the shake curve graph for every active shake character task, including instances that did not opt in. (Enabled: true, Disabled: false)"));
#endif
}

namespace GASCourse_ShakeCharacterInternal
{
	/**
	 * Which task currently owns each skeletal mesh. A mesh can only be shaken by one task at a
	 * time, because they all write the same relative location - overlapping writes would compound
	 * and the later restore would snap the mesh to a shaken value rather than its true base.
	 */
	static TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UGASCourse_ShakeCharacter>> ActiveShakesByMesh;

#if !UE_BUILD_SHIPPING
	/** Every live task, in activation order, so the debug panel lays them out deterministically. */
	static TArray<TWeakObjectPtr<UGASCourse_ShakeCharacter>> ActiveShakeTasks;
#endif
}

UGASCourse_ShakeCharacter::UGASCourse_ShakeCharacter(const FObjectInitializer& ObjectInitializer)
{
	bTickingTask = true;
}

UGASCourse_ShakeCharacter* UGASCourse_ShakeCharacter::CreateShakeTask(UGameplayAbility* OwningAbility,
	float InDurationOverride, float InAmplitudeOverride, bool bInLoop, bool bInPingPong, bool bInDrawDebug)
{
	if (!OwningAbility)
	{
		UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning, "Shake character task created with no owning ability.");
		return nullptr;
	}

	UGASCourse_ShakeCharacter* Task = NewAbilityTask<UGASCourse_ShakeCharacter>(OwningAbility);
	Task->ShakeDurationOverride = InDurationOverride;
	Task->ShakeAmplitudeOverride = InAmplitudeOverride;
	Task->bLoop = bInLoop;
	Task->bPingPong = bInPingPong;
	Task->bDrawDebug = bInDrawDebug;

	// Default target is the ability avatar.
	Task->TargetActor = OwningAbility->GetAvatarActorFromActorInfo();

	return Task;
}

UGASCourse_ShakeCharacter* UGASCourse_ShakeCharacter::ShakeTargetCharacterWithVectorCurve(UGameplayAbility* OwningAbility,
	UCurveVector* ShakeCurve, float ShakeDurationOverride, float ShakeAmplitudeOverride,
	bool bLoop, bool bPingPong, bool bDrawDebug, float ShakeAmplitudeDeviation)
{
	UGASCourse_ShakeCharacter* Task = CreateShakeTask(OwningAbility, ShakeDurationOverride, ShakeAmplitudeOverride, bLoop, bPingPong, bDrawDebug);
	if (Task)
	{
		Task->ShakeVectorCurve = ShakeCurve;
		Task->ShakeAmplitudeDeviation = ShakeAmplitudeDeviation;
	}

	return Task;
}

UGASCourse_ShakeCharacter* UGASCourse_ShakeCharacter::ShakeTargetCharacterWithFloatCurve(UGameplayAbility* OwningAbility,
	UCurveFloat* ShakeCurve, int32 ShakeAxesMask, float ShakeDurationOverride, float ShakeAmplitudeOverride,
	bool bLoop, bool bPingPong, bool bDrawDebug, float ShakeAmplitudeDeviation)
{
	UGASCourse_ShakeCharacter* Task = CreateShakeTask(OwningAbility, ShakeDurationOverride, ShakeAmplitudeOverride, bLoop, bPingPong, bDrawDebug);
	if (Task)
	{
		Task->ShakeFloatCurve = ShakeCurve;
		Task->ShakeAmplitudeDeviation = ShakeAmplitudeDeviation;
		Task->ShakeAxesMask = ShakeAxesMask;
	}

	return Task;
}

UGASCourse_ShakeCharacter* UGASCourse_ShakeCharacter::ShakeTargetCharacter(UGameplayAbility* OwningAbility,
	float ShakeFrequency, float ShakeAmplitude, int32 ShakeAxesMask)
{
	UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning,
		"ShakeTargetCharacter is deprecated and does nothing: the shake is now curve-driven. Rewire {0} to use the float-curve or vector-curve variant and supply a curve asset.",
		OwningAbility ? OwningAbility->GetClass()->GetName() : FString(TEXT("Unknown ability")));

	// Forwarded with no curve on purpose. Activate() will fail loudly and broadcast OnShakeFailed,
	// which is more useful than silently doing nothing.
	UGASCourse_ShakeCharacter* Task = CreateShakeTask(OwningAbility, -1.0f, ShakeAmplitude, false, false, false);
	if (Task)
	{
		Task->ShakeAxesMask = ShakeAxesMask;
	}

	return Task;
}

void UGASCourse_ShakeCharacter::FailAndEnd(const FString& Reason)
{
	UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning, "Shake character task aborted ({0}). Ability: {1}, Target: {2}",
		Reason,
		Ability ? Ability->GetClass()->GetName() : FString(TEXT("None")),
		GetNameSafe(TargetActor.Get()));

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnShakeFailed.Broadcast();
	}

	EndTask();
}

void UGASCourse_ShakeCharacter::Activate()
{
	Super::Activate();

	AActor* Actor = TargetActor.Get();
	if (!IsValid(Actor))
	{
		FailAndEnd(TEXT("invalid target actor"));
		return;
	}

	// Deliberately a component lookup rather than an AGASCourseCharacter cast, matching
	// GASCourseGCNotify_Looping and GASC_MeleeTrace_Subsystem, so the task also works on avatars
	// that are not Characters.
	USkeletalMeshComponent* SkeletalMesh = Actor->GetComponentByClass<USkeletalMeshComponent>();
	if (!IsValid(SkeletalMesh))
	{
		FailAndEnd(TEXT("target actor has no skeletal mesh component"));
		return;
	}

	const bool bHasVectorCurve = ShakeVectorCurve != nullptr;
	const bool bHasFloatCurve = ShakeFloatCurve != nullptr;
	if (!bHasVectorCurve && !bHasFloatCurve)
	{
		FailAndEnd(TEXT("no shake curve provided - a valid curve is required"));
		return;
	}

	if (bHasFloatCurve && ShakeAxesMask == 0)
	{
		FailAndEnd(TEXT("float curve supplied with an empty axis mask, so the shake would have no effect"));
		return;
	}

	// GetTimeRange lives on UCurveBase, so it serves both curve types and handles the empty-key
	// case, unlike reaching into the rich curve keys directly.
	if (bHasVectorCurve)
	{
		ShakeVectorCurve->GetTimeRange(CurveMinTime, CurveMaxTime);
	}
	else
	{
		ShakeFloatCurve->GetTimeRange(CurveMinTime, CurveMaxTime);
	}

	if ((CurveMaxTime - CurveMinTime) <= KINDA_SMALL_NUMBER)
	{
		FailAndEnd(TEXT("shake curve has no usable time range"));
		return;
	}

	// An override of exactly -1 means "use the curve as authored". Any other non-positive value is
	// almost certainly a mistake - zero amplitude silently disables the shake - so say so rather
	// than playing something invisible.
	if (ShakeDurationOverride <= 0.0f && ShakeDurationOverride != -1.0f)
	{
		UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning,
			"Shake duration override of {0} is not positive and is not the -1 sentinel; ignoring it and using the authored curve length.",
			ShakeDurationOverride);
		ShakeDurationOverride = -1.0f;
	}

	// With ping-pong the override is a total repeat duration, which only has meaning when the
	// ping-pong actually repeats. A single pass always takes twice the authored length.
	if (bPingPong && !bLoop && ShakeDurationOverride > 0.0f)
	{
		UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning,
			"Shake duration override of {0} is ignored: a ping-pong shake without bLoop is a single forward-and-back pass over the authored curve length. Enable bLoop to have it repeat for that duration.",
			ShakeDurationOverride);
		ShakeDurationOverride = -1.0f;
	}

	if (ShakeAmplitudeOverride <= 0.0f && ShakeAmplitudeOverride != -1.0f)
	{
		UE_LOGFMT(LOG_GASC_ShakeCharacter, Warning,
			"Shake amplitude override of {0} is not positive and is not the -1 sentinel; ignoring it and using the authored curve values.",
			ShakeAmplitudeOverride);
		ShakeAmplitudeOverride = -1.0f;
	}

	ShakeAmplitudeDeviation = FMath::IsFinite(ShakeAmplitudeDeviation)
		? FMath::Clamp(ShakeAmplitudeDeviation, 0.0f, 1.0f) : 0.0f;
	RandomizeAmplitudeScale();

	// Newest wins: end any task already shaking this mesh so it restores the true base location
	// first. Doing this before caching ours is what stops a restore-to-a-shaken-value bug.
	if (TWeakObjectPtr<UGASCourse_ShakeCharacter>* ExistingShake = GASCourse_ShakeCharacterInternal::ActiveShakesByMesh.Find(SkeletalMesh))
	{
		if (UGASCourse_ShakeCharacter* PreviousTask = ExistingShake->Get())
		{
			if (PreviousTask != this)
			{
				UE_LOGFMT(LOG_GASC_ShakeCharacter, Verbose, "Superseding an in-progress shake on {0}.", GetNameSafe(SkeletalMesh));

				// EndTask runs the previous task's OnDestroy, which removes its entry and so
				// invalidates ExistingShake. PreviousTask is already extracted and ExistingShake is
				// not touched again - do not add a dereference of it after this line.
				PreviousTask->EndTask();
			}
		}
	}

	TargetMesh = SkeletalMesh;
	CachedRelativeLocation = SkeletalMesh->GetRelativeLocation();
	CachedCollisionEnabled = SkeletalMesh->GetCollisionEnabled();
	bHasAppliedShake = true;

	// Often a no-op: a Character does its collision with the capsule and leaves the mesh set to
	// NoCollision. It matters for actors whose skeletal mesh is itself the collider.
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GASCourse_ShakeCharacterInternal::ActiveShakesByMesh.Add(SkeletalMesh, this);

#if !UE_BUILD_SHIPPING
	RegisterForDebug();
	if (ShouldDrawDebug())
	{
		BuildDebugCurveSamples();
	}
#endif
}

void UGASCourse_ShakeCharacter::RandomizeAmplitudeScale()
{
	const float BaseAmplitudeScale = ShakeAmplitudeOverride > 0.0f ? ShakeAmplitudeOverride : 1.0f;
	CurrentAmplitudeScale = BaseAmplitudeScale;
	if (ShakeAmplitudeDeviation > 0.0f)
	{
		CurrentAmplitudeScale *= FMath::FRandRange(1.0f - ShakeAmplitudeDeviation, 1.0f + ShakeAmplitudeDeviation);
	}

#if !UE_BUILD_SHIPPING
	// Rebuild lazily with the new amplitude when the graph is next drawn.
	DebugCurveSamples.Reset();
#endif
}

float UGASCourse_ShakeCharacter::GetCurveCycleLength() const
{
	const float CurveSpan = CurveMaxTime - CurveMinTime;

	// Ping-pong deliberately ignores the override here: there it means a total lifetime, so the
	// curve has to keep its authored speed rather than being stretched to fit.
	if (bPingPong)
	{
		return CurveSpan;
	}

	return ShakeDurationOverride > 0.0f ? ShakeDurationOverride : CurveSpan;
}

float UGASCourse_ShakeCharacter::GetTotalLifetime() const
{
	if (bPingPong)
	{
		if (bLoop)
		{
			// The override is how long the ping-pong keeps repeating; without one it is indefinite.
			return ShakeDurationOverride > 0.0f ? ShakeDurationOverride : -1.0f;
		}

		// Standalone ping-pong is exactly one forward-and-back pass.
		return GetCurveCycleLength() * 2.0f;
	}

	return bLoop ? -1.0f : GetCurveCycleLength();
}

FVector UGASCourse_ShakeCharacter::EvaluateShakeOffset(float CurveTime) const
{
	if (ShakeVectorCurve)
	{
		return ShakeVectorCurve->GetVectorValue(CurveTime);
	}

	if (!ShakeFloatCurve)
	{
		return FVector::ZeroVector;
	}

	const float CurveValue = ShakeFloatCurve->GetFloatValue(CurveTime);
	const EGASCourseShakeTaskAxes Axes = static_cast<EGASCourseShakeTaskAxes>(ShakeAxesMask);

	return FVector(
		EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::X) ? CurveValue : 0.0f,
		EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::Y) ? CurveValue : 0.0f,
		EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::Z) ? CurveValue : 0.0f);
}

void UGASCourse_ShakeCharacter::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!IsActive())
	{
		return;
	}

	USkeletalMeshComponent* SkeletalMesh = TargetMesh.Get();
	if (!IsValid(SkeletalMesh))
	{
		// The mesh went away mid-shake; nothing left to restore.
		bHasAppliedShake = false;
		EndTask();
		return;
	}

	const float PreviousElapsedTime = ElapsedTime;
	ElapsedTime += DeltaTime;

	const float CurveCycleLength = GetCurveCycleLength();
	if (CurveCycleLength <= KINDA_SMALL_NUMBER)
	{
		EndTask();
		return;
	}

	const float TotalLifetime = GetTotalLifetime();

	// Check before wrapping elapsed time. Ping-pong keeps one amplitude through both directions.
	// If a long frame skips whole loops, only the loop being displayed needs a new sample.
	const float LoopLength = CurveCycleLength * (bPingPong ? 2.0f : 1.0f);
	if (bLoop && ShakeAmplitudeDeviation > 0.0f
		&& (TotalLifetime <= 0.0f || ElapsedTime < TotalLifetime)
		&& FMath::FloorToFloat(ElapsedTime / LoopLength) > FMath::FloorToFloat(PreviousElapsedTime / LoopLength))
	{
		RandomizeAmplitudeScale();
	}

	float Alpha;
	if (bPingPong)
	{
		const float RoundTrip = CurveCycleLength * 2.0f;

		// Wrap ElapsedTime only when no lifetime needs its absolute value, so an indefinite
		// ping-pong cannot accumulate a large accumulator and lose precision in the division. A
		// bounded one must keep absolute elapsed for the termination check, and is bounded anyway.
		if (TotalLifetime <= 0.0f)
		{
			ElapsedTime = FMath::Fmod(ElapsedTime, RoundTrip);
		}

		// 0..2 across a round trip, with the second half reflected so it plays in reverse.
		const float CycleAlpha = FMath::Fmod(ElapsedTime, RoundTrip) / CurveCycleLength;
		Alpha = CycleAlpha > 1.0f ? (2.0f - CycleAlpha) : CycleAlpha;
	}
	else if (bLoop)
	{
		// Wrap ElapsedTime itself rather than only the ratio, for the same precision reason.
		ElapsedTime = FMath::Fmod(ElapsedTime, CurveCycleLength);
		Alpha = ElapsedTime / CurveCycleLength;
	}
	else
	{
		Alpha = FMath::Clamp(ElapsedTime / CurveCycleLength, 0.0f, 1.0f);
	}

	CurrentCurveAlpha = Alpha;

	// Remap the cycle position into curve time so the authored curve fits the cycle length,
	// matching the GASC_TimeWarp_NotifyState idiom.
	const float CurveTime = CurveMinTime + Alpha * (CurveMaxTime - CurveMinTime);

	const FVector Offset = EvaluateShakeOffset(CurveTime) * CurrentAmplitudeScale;

	// Position offset only - rotation and scale are never touched.
	SkeletalMesh->SetRelativeLocation(CachedRelativeLocation + Offset);

#if !UE_BUILD_SHIPPING
	DebugCurrentOffset = Offset;

	// Build samples after an amplitude change or after opting in via the CVar.
	if (ShouldDrawDebug() && DebugCurveSamples.Num() == 0)
	{
		BuildDebugCurveSamples();
	}
#endif

	// One rule for every mode: a negative lifetime means indefinite, so OnShakeCompleted never
	// fires for those and they end only via StopShake, a newer shake claiming the same mesh, or
	// the owning ability ending.
	if (TotalLifetime > 0.0f && ElapsedTime >= TotalLifetime)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnShakeCompleted.Broadcast();
		}

		EndTask();
	}
}

void UGASCourse_ShakeCharacter::StopShake()
{
	if (!IsActive())
	{
		return;
	}

	// Restoration is entirely OnDestroy's job, so there is nothing to undo here. Intentionally
	// silent on OnShakeCompleted - see the declaration for why.
	EndTask();
}

void UGASCourse_ShakeCharacter::OnDestroy(bool AbilityEnded)
{
	// Restore before Super, and guard with bHasAppliedShake so this is safe on both normal
	// completion and ability interruption.
	if (bHasAppliedShake)
	{
		if (USkeletalMeshComponent* SkeletalMesh = TargetMesh.Get())
		{
			SkeletalMesh->SetRelativeLocation(CachedRelativeLocation);
			SkeletalMesh->SetCollisionEnabled(CachedCollisionEnabled);

			// Only clear the registry slot if it is still ours. A newer task may have taken the
			// mesh already, and stomping its entry would leak the mesh out of the newest-wins rule.
			if (TWeakObjectPtr<UGASCourse_ShakeCharacter>* Owner = GASCourse_ShakeCharacterInternal::ActiveShakesByMesh.Find(SkeletalMesh))
			{
				if (Owner->Get() == this)
				{
					GASCourse_ShakeCharacterInternal::ActiveShakesByMesh.Remove(SkeletalMesh);
				}
			}
		}

		bHasAppliedShake = false;
	}

	// Drop any entries whose mesh has been destroyed, so the map cannot grow without bound.
	for (auto It = GASCourse_ShakeCharacterInternal::ActiveShakesByMesh.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}

#if !UE_BUILD_SHIPPING
	UnregisterFromDebug();
#endif

	Super::OnDestroy(AbilityEnded);
}

#if !UE_BUILD_SHIPPING

const TArray<TWeakObjectPtr<UGASCourse_ShakeCharacter>>& UGASCourse_ShakeCharacter::GetActiveShakeTasksForDebug()
{
	return GASCourse_ShakeCharacterInternal::ActiveShakeTasks;
}

void UGASCourse_ShakeCharacter::RegisterForDebug()
{
	GASCourse_ShakeCharacterInternal::ActiveShakeTasks.AddUnique(this);

	// Compact stale entries here rather than in the panel, so the cost lands on activation
	// instead of every frame of drawing.
	GASCourse_ShakeCharacterInternal::ActiveShakeTasks.RemoveAll([](const TWeakObjectPtr<UGASCourse_ShakeCharacter>& Entry)
	{
		return !Entry.IsValid();
	});
}

void UGASCourse_ShakeCharacter::UnregisterFromDebug()
{
	GASCourse_ShakeCharacterInternal::ActiveShakeTasks.Remove(this);
}

bool UGASCourse_ShakeCharacter::ShouldDrawDebug() const
{
	return bDrawDebug || GASCourse_ShakeCharacterCVars::CvarEnableShakeCurveGraph.GetValueOnGameThread();
}

void UGASCourse_ShakeCharacter::BuildDebugCurveSamples()
{
	// Sample the whole curve at the current loop's amplitude so the graph matches playback.
	DebugCurveSamples.Reset(GASCOURSE_SHAKE_DEBUG_SAMPLE_COUNT);
	DebugPeakOffset = FVector::ZeroVector;

	const float CurveTimeSpan = CurveMaxTime - CurveMinTime;

	for (int32 SampleIndex = 0; SampleIndex < GASCOURSE_SHAKE_DEBUG_SAMPLE_COUNT; ++SampleIndex)
	{
		const float SampleAlpha = static_cast<float>(SampleIndex) / static_cast<float>(GASCOURSE_SHAKE_DEBUG_SAMPLE_COUNT - 1);
		const FVector Sample = EvaluateShakeOffset(CurveMinTime + SampleAlpha * CurveTimeSpan) * CurrentAmplitudeScale;

		DebugCurveSamples.Add(Sample);

		DebugPeakOffset.X = FMath::Max(DebugPeakOffset.X, FMath::Abs(Sample.X));
		DebugPeakOffset.Y = FMath::Max(DebugPeakOffset.Y, FMath::Abs(Sample.Y));
		DebugPeakOffset.Z = FMath::Max(DebugPeakOffset.Z, FMath::Abs(Sample.Z));
	}
}

float UGASCourse_ShakeCharacter::GetDebugPlayheadAlpha() const
{
	// Read the value the tick resolved rather than recomputing it. Ping-pong cannot be derived
	// from ElapsedTime without repeating the wrap logic, and a second implementation would drift.
	return CurrentCurveAlpha;
}

FString UGASCourse_ShakeCharacter::GetDebugTargetActorName() const
{
	return GetNameSafe(TargetActor.Get());
}

FString UGASCourse_ShakeCharacter::GetDebugMeshComponentName() const
{
	return GetNameSafe(TargetMesh.Get());
}

FString UGASCourse_ShakeCharacter::GetDebugInvokingClassName() const
{
	if (!Ability)
	{
		return TEXT("Unknown");
	}

	// Blueprint-generated classes carry a trailing "_C"; strip it so the name matches what the
	// designer sees in the content browser.
	FString ClassName = Ability->GetClass()->GetName();
	ClassName.RemoveFromEnd(TEXT("_C"));

	return ClassName;
}

FString UGASCourse_ShakeCharacter::GetDebugInvokingClassPath() const
{
	return Ability ? Ability->GetClass()->GetPathName() : TEXT("Unknown");
}

#endif
