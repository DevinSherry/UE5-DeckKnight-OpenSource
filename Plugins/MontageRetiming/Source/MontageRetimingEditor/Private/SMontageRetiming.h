#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "MontageRetimingData.h"

class IAnimationEditor;
class UAnimMontage;
class SVerticalBox;
class FScopedTransaction;

class SMontageRetiming : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMontageRetiming) {}
        SLATE_ARGUMENT(TWeakPtr<IAnimationEditor>, Editor)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual void Tick(const FGeometry&, double, float) override;
private:
    UAnimMontage* GetMontage() const;
    void Refresh();
    void Rebuild();
    void Edit(TFunctionRef<void(UAnimMontage&)> Change, const FText& Description, bool bValidate = true);
    void SetTarget(int32 Index, double Frames);
    void Reset(int32 Index);
    void Batch(bool bScale);
    void EnsurePreview();
    class UMontageRetimingPreviewInstance* GetPreview() const;
    void PreviewSection(int32 Index);
    void ToggleSectionLoop(int32 Index);
    static UMontageRetimingSection* GetOrAddSection(UAnimMontage&, int32);
    static void ResetSection(UAnimMontage&, int32);
    TWeakPtr<IAnimationEditor> Editor;
    TWeakObjectPtr<UAnimMontage> DisplayedMontage;
    TSharedPtr<SVerticalBox> RowsBox;
    TArray<TSharedPtr<FMontageRetimingRow>> Rows;
    TSet<int32> Selected;
    FString LayoutSignature;
    FString LastWarning;
    double BatchFrames = 20;
    double BatchPercent = 100;
    double LastRefresh = 0;
    TSharedPtr<FScopedTransaction> LiveTransaction;
};
