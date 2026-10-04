#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMetaData.h"
#include "MontageRetimingData.generated.h"

class UAnimMontage;

/** Asset-owned opt-in. Never modifies montage markers, segments or notifies. */
UCLASS(NotBlueprintable)
class MONTAGERETIMING_API UMontageRetimingSettings : public UAnimMetaData
{
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bEnabled = false;

    UPROPERTY()
    bool bValidForCookedPlayback = false;

    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
    void RefreshCookedData();
};

/** Stored on FCompositeSection::MetaData so identity survives renames and sorting. */
UCLASS(NotBlueprintable)
class MONTAGERETIMING_API UMontageRetimingSection : public UAnimMetaData
{
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid Identity;

    // Seconds preserve intent if source sample rate changes; UI edits use source frames.
    UPROPERTY()
    double TargetSeconds = 0.0;

    UPROPERTY()
    bool bOverride = false;

    UPROPERTY()
    FFrameRate SourceFrameRate = FFrameRate(0, 1);
};

struct MONTAGERETIMING_API FMontageRetimingRow
{
    int32 SectionIndex = INDEX_NONE;
    FName Name;
    FName Next;
    double Start = 0.0;
    double End = 0.0;
    double FramesPerSecond = 0.0;
    double OriginalFrames = 0.0;
    double TargetFrames = 0.0;
    double TargetSeconds = 0.0;
    double Speed = 1.0;
    double TimelineOrderStartSeconds = 0.0;
    bool bOverride = false;
    bool bLoop = false;
    FString Error;
};

namespace MontageRetiming
{
    MONTAGERETIMING_API const UMontageRetimingSettings* FindSettings(const UAnimMontage& Montage);
    MONTAGERETIMING_API UMontageRetimingSection* FindSection(const UAnimMontage& Montage, int32 SectionIndex);
    MONTAGERETIMING_API TArray<FMontageRetimingRow> Describe(const UAnimMontage& Montage, bool bUseCookedRates = false);
    MONTAGERETIMING_API bool IsEnabled(const UAnimMontage& Montage);
}
