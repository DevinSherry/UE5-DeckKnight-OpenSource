#include "MontageRetimingAnimInstance.h"
#include "MontageRetimingData.h"
#include "Animation/AnimMontage.h"

void UMontageRetimingAnimInstance::Montage_Advance(float DeltaSeconds)
{
    MontageRetiming::Advance(*this, DeltaSeconds, [this](float Step) { Super::Montage_Advance(Step); });
}

void MontageRetiming::Advance(UAnimInstance& Instance, float DeltaSeconds, TFunctionRef<void(float)> NativeAdvance)
{
    if (DeltaSeconds <= 0 || !FMath::IsFinite(DeltaSeconds)) { NativeAdvance(DeltaSeconds); return; }
    bool bAnyRetiming = false;
    for (const auto* MI : Instance.MontageInstances)
        bAnyRetiming |= MI && MI->Montage && IsEnabled(*MI->Montage);
    if (!bAnyRetiming) { NativeAdvance(DeltaSeconds); return; }

    struct FSavedRate { int32 Id; float External; float Applied; };
    struct FMovement { float Previous = 0; float Delta = 0; bool bInitialized = false; TArray<FPassedMarker> Markers; };
    TMap<int32, FMovement> Movement;
    float Remaining = DeltaSeconds;
    // Bounded work for pathological tiny looping sections or extreme external rates.
    constexpr int32 MaxSteps = 512;
    for (int32 StepIndex = 0; Remaining > UE_SMALL_NUMBER && StepIndex < MaxSteps; ++StepIndex)
    {
        TArray<FSavedRate> Rates;
        TSet<int32> AdvancedInstances;
        float Step = Remaining;
        for (auto* MI : Instance.MontageInstances)
        {
            if (!MI || !MI->Montage || !MI->IsPlaying()) continue;
            AdvancedInstances.Add(MI->GetInstanceID());
            MI->DeltaTimeRecord.Set(MI->GetPosition(), 0.f);
            auto& Record = Movement.FindOrAdd(MI->GetInstanceID());
            if (!Record.bInitialized) { Record.Previous = MI->GetPosition(); Record.bInitialized = true; }
            if (!IsEnabled(*MI->Montage)) continue;
            const auto Rows = Describe(*MI->Montage);
            // Fail closed if any section cannot be interpreted consistently.
            if (Rows.ContainsByPredicate([](const auto& R) { return !R.Error.IsEmpty(); })) continue;
            const double Position = MI->GetPosition();
            for (const auto& Row : Rows)
            {
                if (Position < Row.Start || Position >= Row.End) continue;
                const float External = MI->GetPlayRate();
                const float Applied = External * static_cast<float>(Row.Speed);
                Rates.Add({MI->GetInstanceID(), External, Applied});
                MI->SetPlayRate(Applied);
                const float Effective = Applied * MI->Montage->RateScale;
                if (FMath::Abs(Effective) <= UE_SMALL_NUMBER) break;
                const bool bForward = Effective > 0;
                double Boundary = bForward ? Row.End : Row.Start;
                // Branching callbacks may jump, stop, start, or change playback rate.
                // Stop the outer slice there so the next slice can reread current state.
                for (const FAnimNotifyEvent& Notify : MI->Montage->Notifies)
                {
                    if (!Notify.IsBranchingPoint()) continue;
                    const float Times[] = { Notify.GetTriggerTime(), Notify.GetEndTriggerTime() };
                    for (float Time : Times)
                    {
                        if (bForward && Time > Position && Time < Boundary) Boundary = Time;
                        if (!bForward && Time < Position && Time > Boundary) Boundary = Time;
                    }
                }
                const double UntilBoundary = FMath::Abs((Boundary - Position) / Effective);
                // Small progress tolerance permits UE's own section routing at an exact boundary.
                Step = FMath::Min(Step, FMath::Max(static_cast<float>(UntilBoundary), UE_SMALL_NUMBER * 2));
                break;
            }
        }

        NativeAdvance(Step);
        for (auto* MI : Instance.MontageInstances)
            if (MI && AdvancedInstances.Contains(MI->GetInstanceID()))
            {
                auto& Record = Movement.FindOrAdd(MI->GetInstanceID());
                Record.Delta += MI->GetDeltaMoved();
                Record.Markers.Append(MI->MarkersPassedThisTick);
            }
        for (const auto& Rate : Rates)
        {
            if (auto* MI = Instance.GetMontageInstanceForID(Rate.Id))
            {
                // Preserve new rates explicitly assigned by native branching callbacks.
                if (MI->GetPlayRate() == Rate.Applied) MI->SetPlayRate(Rate.External);
            }
        }
        Remaining = FMath::Max(0.f, Remaining - Step);
    }
    if (Remaining > UE_SMALL_NUMBER)
        UE_LOG(LogAnimation, Warning, TEXT("Montage Retiming exhausted its substep budget; %.6f seconds were not advanced."), Remaining);
    // Advance resets its delta record each call. Preserve the complete tick for consumers.
    for (auto* MI : Instance.MontageInstances)
        if (MI) if (auto* Record = Movement.Find(MI->GetInstanceID()))
        {
            MI->DeltaTimeRecord.Set(Record->Previous, Record->Delta);
            MI->MarkersPassedThisTick = MoveTemp(Record->Markers);
        }
}
