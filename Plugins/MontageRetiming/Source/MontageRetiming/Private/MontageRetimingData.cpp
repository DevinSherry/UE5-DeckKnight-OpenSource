#include "MontageRetimingData.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectSaveContext.h"
#if WITH_EDITOR
#include "Animation/AnimData/IAnimationDataModel.h"
#endif

IMPLEMENT_MODULE(FDefaultModuleImpl, MontageRetiming)

namespace
{
    bool HasTimeWarp(const UAnimSequenceBase& Asset)
    {
        for (const auto& Notify : Asset.Notifies)
            if (Notify.NotifyStateClass)
                for (const UClass* Class = Notify.NotifyStateClass->GetClass(); Class; Class = Class->GetSuperClass())
                    if (Class->GetFName() == FName(TEXT("GASC_TimeWarp_NotifyState"))) return true;
        return false;
    }
}

void UMontageRetimingSettings::PreSave(FObjectPreSaveContext SaveContext)
{
    Super::PreSave(SaveContext);
    RefreshCookedData();
    if (SaveContext.IsCooking() && bEnabled && !bValidForCookedPlayback)
        UE_LOG(LogAnimation, Error, TEXT("Cannot cook enabled retiming on %s: resolve Section Retiming warnings before packaging."), *GetPathNameSafe(GetOuter()));
}

void UMontageRetimingSettings::RefreshCookedData()
{
#if WITH_EDITOR
    if (const auto* Montage = Cast<UAnimMontage>(GetOuter()))
    {
        bValidForCookedPlayback = true;
        for (const auto& Row : MontageRetiming::Describe(*Montage))
        {
            auto* Data = MontageRetiming::FindSection(*Montage, Row.SectionIndex);
            if (!Row.Error.IsEmpty()) bValidForCookedPlayback = false;
            if (Data && Row.FramesPerSecond > 0)
            {
                // Preserve the exact rational source rate, including 30000/1001.
                for (const auto& Slot : Montage->SlotAnimTracks)
                    for (const auto& Segment : Slot.AnimTrack.AnimSegments)
                        if (Segment.StartPos < Row.End && Segment.GetEndPos() > Row.Start)
                            if (const auto* Seq = Cast<UAnimSequence>(Segment.GetAnimReference()))
                                if (Seq->GetDataModel()) Data->SourceFrameRate = Seq->GetDataModel()->GetFrameRate();
            }
        }
    }
#endif
}

const UMontageRetimingSettings* MontageRetiming::FindSettings(const UAnimMontage& Montage)
{
    for (const UAnimMetaData* Data : Montage.GetMetaData())
        if (const auto* Settings = Cast<UMontageRetimingSettings>(Data)) return Settings;
    return nullptr;
}

UMontageRetimingSection* MontageRetiming::FindSection(const UAnimMontage& Montage, int32 Index)
{
    if (!Montage.CompositeSections.IsValidIndex(Index)) return nullptr;
    for (UAnimMetaData* Data : Montage.CompositeSections[Index].MetaData)
        if (auto* Section = Cast<UMontageRetimingSection>(Data)) return Section;
    return nullptr;
}

bool MontageRetiming::IsEnabled(const UAnimMontage& Montage)
{
    const auto* Settings = FindSettings(Montage);
    return Settings && Settings->bEnabled;
}

TArray<FMontageRetimingRow> MontageRetiming::Describe(const UAnimMontage& Montage, bool bUseCookedRates)
{
#if !WITH_EDITOR
    bUseCookedRates = true;
#endif
    TArray<FMontageRetimingRow> Rows;
    for (int32 Index = 0; Index < Montage.CompositeSections.Num(); ++Index)
    {
        const auto& Section = Montage.CompositeSections[Index];
        auto& Row = Rows.AddDefaulted_GetRef();
        Row.SectionIndex = Index;
        Row.Name = Section.SectionName;
        Row.Next = Section.NextSectionName;
        Row.Start = Section.GetTime();
        Row.End = Montage.GetPlayLength();
    }
    Rows.Sort([](const auto& A, const auto& B) { return A.Start < B.Start; });
    TSet<FGuid> Identities;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        auto& Row = Rows[Index];
        if (Index + 1 < Rows.Num()) Row.End = Rows[Index + 1].Start;
        const double Duration = Row.End - Row.Start;
        if (Duration <= UE_SMALL_NUMBER || Row.Start < 0.0)
            Row.Error = TEXT("Section markers must have distinct positions and positive durations.");

        TOptional<FFrameRate> Rate;
        if (bUseCookedRates)
        {
            if (const auto* Data = FindSection(Montage, Row.SectionIndex)) Rate = Data->SourceFrameRate;
            // Unedited sections need no frame conversion at runtime. Their multiplier is 1.
            else Rate = FFrameRate(1, 1);
            const auto* Settings = FindSettings(Montage);
            if (!Settings || !Settings->bValidForCookedPlayback)
                Row.Error = TEXT("Retiming needs validation and saving in the editor before cooked playback.");
        }
        for (const auto& Slot : Montage.SlotAnimTracks)
        {
            if (!Montage.GetSkeleton() || Montage.GetSkeleton()->GetSlotGroupName(Slot.SlotName) != FName(TEXT("DefaultGroup")))
                Row.Error = TEXT("Retiming supports slot tracks in DefaultGroup only.");
            for (const auto& Segment : Slot.AnimTrack.AnimSegments)
            {
                if (Segment.StartPos >= Row.End || Segment.GetEndPos() <= Row.Start) continue;
                const auto* Sequence = Cast<UAnimSequence>(Segment.GetAnimReference());
                if (!Sequence)
                {
                    Row.Error = TEXT("Use animation sequence segments; nested composites are not supported.");
                    continue;
                }
#if WITH_EDITOR
                if (!bUseCookedRates)
                {
                const FFrameRate SourceRate = Sequence->GetDataModel() ? Sequence->GetDataModel()->GetFrameRate() : FFrameRate(0, 1);
                if (SourceRate.Numerator <= 0 || SourceRate.Denominator <= 0)
                    Row.Error = TEXT("Source animation has no valid sample rate.");
                else if (Rate.IsSet() && Rate.GetValue() != SourceRate)
                    Row.Error = TEXT("Mixed animation sample rates in this section. Use matching source rates across all slots.");
                else Rate = SourceRate;
                }
#endif
            }
        }
        if (!Rate.IsSet()) Row.Error = TEXT("Section must overlap a source animation with a valid sample rate.");
        Row.FramesPerSecond = Rate.IsSet() ? Rate.GetValue().AsDecimal() : 0.0;
        // Segment play rates already contribute to the authored timeline duration.
        const double Baseline = Montage.RateScale > UE_SMALL_NUMBER ? Duration / Montage.RateScale : 0.0;
        if (!FMath::IsFinite(Baseline) || Montage.RateScale <= UE_SMALL_NUMBER)
            Row.Error = TEXT("Montage Rate Scale must be positive for retiming.");
        if (Montage.TimeStretchCurve.IsValid())
            Row.Error = TEXT("Remove the existing montage Time Stretch Curve before using section retiming.");
        if (Montage.CanUseMarkerSync())
            Row.Error = TEXT("Marker-synchronized montages are not supported by this iteration tool.");
        bool bHasTimeWarp = HasTimeWarp(Montage);
        for (const auto& Slot : Montage.SlotAnimTracks)
            for (const auto& Segment : Slot.AnimTrack.AnimSegments)
                if (const auto* Source = Segment.GetAnimReference().Get()) bHasTimeWarp |= HasTimeWarp(*Source);
        if (bHasTimeWarp)
            Row.Error = TEXT("Time Warp notifies and section retiming are mutually exclusive. Remove Time Warp or disable retiming.");
        Row.OriginalFrames = Baseline * Row.FramesPerSecond;
        const auto* Data = FindSection(Montage, Row.SectionIndex);
        Row.bOverride = Data && Data->bOverride;
        Row.TargetSeconds = Row.bOverride ? Data->TargetSeconds : Baseline;
        Row.TargetFrames = Row.TargetSeconds * Row.FramesPerSecond;
        if (!FMath::IsFinite(Row.TargetSeconds) || Row.TargetSeconds <= UE_SMALL_NUMBER)
            Row.Error = TEXT("Target duration must be positive and finite. Reset this section.");
        else Row.Speed = Baseline / Row.TargetSeconds;
        if (!FMath::IsFinite(static_cast<float>(Row.Speed)) || Row.Speed <= UE_SMALL_NUMBER)
            Row.Error = TEXT("Target duration exceeds the supported playback speed range. Reset or enter a smaller target.");
        if (Data)
        {
            if (!Data->Identity.IsValid() || Identities.Contains(Data->Identity))
                Row.Error = TEXT("Ambiguous section identity after an edit. Reset this section to establish new timing.");
            Identities.Add(Data->Identity);
        }
        TSet<FName> Visited;
        FName Next = Row.Name;
        while (!Next.IsNone() && !Visited.Contains(Next))
        {
            Visited.Add(Next);
            const int32 NextIndex = Montage.GetSectionIndex(Next);
            if (NextIndex == INDEX_NONE) break;
            Next = Montage.CompositeSections[NextIndex].NextSectionName;
        }
        Row.bLoop = !Next.IsNone() && Visited.Contains(Next);
    }
    double Elapsed = 0;
    for (auto& Row : Rows)
    {
        Row.TimelineOrderStartSeconds = Elapsed;
        if (FMath::IsFinite(Row.TargetSeconds)) Elapsed += Row.TargetSeconds;
    }
    return Rows;
}
