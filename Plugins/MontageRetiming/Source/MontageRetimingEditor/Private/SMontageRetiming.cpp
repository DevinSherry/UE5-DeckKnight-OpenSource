#include "SMontageRetiming.h"
#include "Animation/AnimMontage.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "MontageRetimingPreviewInstance.h"
#include "IAnimationEditor.h"
#include "IPersonaToolkit.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MontageRetiming"

UAnimMontage* SMontageRetiming::GetMontage() const
{
    const auto Pinned = Editor.Pin();
    return Pinned ? Cast<UAnimMontage>(Pinned->GetPersonaToolkit()->GetAnimationAsset()) : nullptr;
}

void SMontageRetiming::Construct(const FArguments& Args)
{
    Editor = Args._Editor;
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [ SNew(STextBlock).AutoWrapText(true).Text(LOCTEXT("Help",
            "Targets use source animation frames. Timeline markers and notifies stay in place. Seconds assume an external playback rate of 1.0x. Enabled retiming also applies in Development and Shipping builds.")) ]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,20,0)
            [ SNew(SCheckBox)
                .IsChecked_Lambda([this] { const auto* M = GetMontage(); return M && MontageRetiming::IsEnabled(*M) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                {
                    Edit([State](UAnimMontage& M)
                    {
                        auto* Settings = const_cast<UMontageRetimingSettings*>(MontageRetiming::FindSettings(M));
                        if (!Settings) { Settings = NewObject<UMontageRetimingSettings>(&M, NAME_None, RF_Transactional); M.AddMetaData(Settings); }
                        Settings->Modify();
                        Settings->bEnabled = State == ECheckBoxState::Checked;
                        for (int32 I = 0; I < M.CompositeSections.Num(); ++I) GetOrAddSection(M, I);
                    }, LOCTEXT("Toggle", "Toggle montage retiming"), State == ECheckBoxState::Checked);
                })
                [ SNew(STextBlock).Text(LOCTEXT("Enable", "Enable Retiming")) ] ]
            + SHorizontalBox::Slot().AutoWidth()
            [ SNew(SButton).Text(LOCTEXT("ResetAll", "Reset All"))
                .OnClicked_Lambda([this] { Edit([](UAnimMontage& M) {
                    for (int32 I = 0; I < M.CompositeSections.Num(); ++I)
                        ResetSection(M, I);
                }, LOCTEXT("ResetAllTransaction", "Reset all section timings"), false); return FReply::Handled(); }) ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [ SNew(STextBlock).Text(LOCTEXT("Selected", "Selected sections:  ")) ]
            + SHorizontalBox::Slot().AutoWidth()
            [ SNew(SBox).WidthOverride(85)[ SNew(SNumericEntryBox<double>).MinValue(1).AllowSpin(false)
                .Value_Lambda([this] { return BatchFrames; }).OnValueChanged_Lambda([this](double V) { BatchFrames = V; }) ] ]
            + SHorizontalBox::Slot().AutoWidth().Padding(4,0,16,0)
            [ SNew(SButton).Text(LOCTEXT("SetFrames", "Set frames")).OnClicked_Lambda([this] { Batch(false); return FReply::Handled(); }) ]
            + SHorizontalBox::Slot().AutoWidth()
            [ SNew(SBox).WidthOverride(85)[ SNew(SNumericEntryBox<double>).MinValue(0.01).AllowSpin(false)
                .Value_Lambda([this] { return BatchPercent; }).OnValueChanged_Lambda([this](double V) { BatchPercent = V; }) ] ]
            + SHorizontalBox::Slot().AutoWidth().Padding(4,0)
            [ SNew(SButton).Text(LOCTEXT("Scale", "Scale duration %")).OnClicked_Lambda([this] { Batch(true); return FReply::Handled(); }) ]
        ]
        + SVerticalBox::Slot().FillHeight(1).Padding(8)
        [ SNew(SScrollBox) + SScrollBox::Slot()[ SAssignNew(RowsBox, SVerticalBox) ] ]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [ SNew(STextBlock).Text(LOCTEXT("PreviewHelp", "Changing Target frames restarts that section's preview.")) ]
    ];
    Refresh();
}

void SMontageRetiming::Tick(const FGeometry& Geometry, double Now, float Delta)
{
    SCompoundWidget::Tick(Geometry, Now, Delta);
    if (Now - LastRefresh > 0.2) { LastRefresh = Now; Refresh(); }
}

void SMontageRetiming::Refresh()
{
    auto* Montage = GetMontage();
    if (!Montage) { Rows.Empty(); RowsBox->ClearChildren(); return; }
    const auto Descriptions = MontageRetiming::Describe(*Montage);
    FString Signature = Montage->GetPathName();
    FString Warning;
    for (const auto& Row : Descriptions)
    {
        Signature += FString::Printf(TEXT("|%d:%s:%f"), Row.SectionIndex, *Row.Name.ToString(), Row.Start);
        if (!Row.Error.IsEmpty()) Warning += Row.Name.ToString() + TEXT(": ") + Row.Error + TEXT("\n");
    }
    if (LayoutSignature != Signature)
    {
        LayoutSignature = Signature;
        DisplayedMontage = Montage;
        Selected.Empty();
        Rows.Empty();
        for (const auto& Row : Descriptions) Rows.Add(MakeShared<FMontageRetimingRow>(Row));
        Rebuild();
    }
    else for (int32 I = 0; I < Rows.Num(); ++I) *Rows[I] = Descriptions[I];
    EnsurePreview();
    if (!Warning.IsEmpty() && Warning != LastWarning)
    {
        LastWarning = Warning;
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Montage retiming needs attention:\n\n") + Warning));
    }
    if (Warning.IsEmpty()) LastWarning.Empty();
}

void SMontageRetiming::Rebuild()
{
    RowsBox->ClearChildren();
    for (const auto& Row : Rows)
    {
        RowsBox->AddSlot().AutoHeight().Padding(0,6)
        [ SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                [ SNew(SCheckBox).IsChecked_Lambda([this, Row] { return Selected.Contains(Row->SectionIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                    .OnCheckStateChanged_Lambda([this, Row](ECheckBoxState S) { if (S == ECheckBoxState::Checked) Selected.Add(Row->SectionIndex); else Selected.Remove(Row->SectionIndex); }) ]
                + SHorizontalBox::Slot().AutoWidth()
                [ SNew(SBox).WidthOverride(120)
                    [ SNew(STextBlock).Text_Lambda([Row] { return FText::FromString(FString::Printf(TEXT("%s\nNext: %s%s"), *Row->Name.ToString(), *Row->Next.ToString(), Row->bLoop ? TEXT(" (loop path)") : TEXT(""))); }) ] ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4,0).VAlign(VAlign_Center)
                [ SNew(SButton)
                    .Text_Lambda([this, Row] { auto* P = GetPreview(); return P && P->GetLoopingSection() == Row->Name ? LOCTEXT("Looping", "Looping") : LOCTEXT("Loop", "Loop"); })
                    .ButtonColorAndOpacity_Lambda([this, Row] { auto* P = GetPreview(); return P && P->GetLoopingSection() == Row->Name ? FLinearColor(0.15f,0.45f,0.8f) : FLinearColor::White; })
                    .ToolTipText(LOCTEXT("LoopHelp", "Toggle preview looping for only this section. Authored section links are preserved."))
                    .OnClicked_Lambda([this, Row] { ToggleSectionLoop(Row->SectionIndex); return FReply::Handled(); }) ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4,0).VAlign(VAlign_Center)
                [ SNew(SButton).Text(LOCTEXT("Jump", "Jump to section"))
                    .ToolTipText(LOCTEXT("JumpHelp", "Start preview playback at this section and exit section-only looping."))
                    .OnClicked_Lambda([this, Row] { EnsurePreview(); if (auto* P = GetPreview()) P->PreviewSection(Row->SectionIndex, false); return FReply::Handled(); }) ]
                + SHorizontalBox::Slot().FillWidth(1)[ SNew(SSpacer) ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                [ SNew(STextBlock).Text_Lambda([Row] { return FText::FromString(FString::Printf(TEXT("Original: %.3f frames\nSource: %.3f fps"), Row->OriginalFrames, Row->FramesPerSecond)); }) ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(LOCTEXT("Target", "Target frames")) ]
                    + SVerticalBox::Slot().AutoHeight()[ SNew(SBox).WidthOverride(100)
                        [ SNew(SNumericEntryBox<double>).MinValue(1).AllowSpin(false)
                            .IsEnabled_Lambda([Row] { return Row->Error.IsEmpty(); })
                            .Value_Lambda([Row] { return Row->TargetFrames; })
                            .OnValueChanged_Lambda([this, Row](double V) {
                                if (!LiveTransaction) LiveTransaction = MakeShared<FScopedTransaction>(LOCTEXT("LiveEdit", "Edit section target frames"));
                                SetTarget(Row->SectionIndex, V);
                            })
                            .OnValueCommitted_Lambda([this, Row](double V, ETextCommit::Type) { SetTarget(Row->SectionIndex, V); LiveTransaction.Reset(); }) ] ] ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                [ SNew(STextBlock).Text_Lambda([Row] { return FText::FromString(FString::Printf(TEXT("%.3fx speed | %.4f seconds\nTimeline-order start: %.4fs"), Row->Speed, Row->TargetSeconds, Row->TimelineOrderStartSeconds)); })
                    .ToolTipText(LOCTEXT("ElapsedHelp", "Elapsed start assumes each preceding section plays once in timeline order. Jumps and loops change actual arrival time.")) ]
                + SHorizontalBox::Slot().AutoWidth()
                [ SNew(SButton).Text(LOCTEXT("Reset", "Reset section")).OnClicked_Lambda([this, Row] { Reset(Row->SectionIndex); return FReply::Handled(); }) ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(1.f,0.45f,0.1f))
                .Text_Lambda([Row] { return FText::FromString(Row->Error); }) ]
        ];
    }
}

UMontageRetimingSection* SMontageRetiming::GetOrAddSection(UAnimMontage& Montage, int32 Index)
{
    auto* Data = MontageRetiming::FindSection(Montage, Index);
    if (!Data)
    {
        Data = NewObject<UMontageRetimingSection>(&Montage, NAME_None, RF_Transactional);
        Data->Identity = FGuid::NewGuid();
        Montage.CompositeSections[Index].MetaData.Add(Data);
    }
    Data->Modify();
    return Data;
}

void SMontageRetiming::Edit(TFunctionRef<void(UAnimMontage&)> Change, const FText& Description, bool bValidate)
{
    auto* Montage = GetMontage();
    if (!Montage) return;
    const auto Before = MontageRetiming::Describe(*Montage);
    if (bValidate)
    {
        FString Errors;
        for (const auto& Row : MontageRetiming::Describe(*Montage))
            if (!Row.Error.IsEmpty()) Errors += Row.Name.ToString() + TEXT(": ") + Row.Error + TEXT("\n");
        if (!Errors.IsEmpty()) { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Errors)); return; }
    }
    TUniquePtr<FScopedTransaction> Transaction;
    if (!LiveTransaction) Transaction = MakeUnique<FScopedTransaction>(Description);
    auto* Preview = GetPreview();
    bool bUnusedGuard = false;
    TGuardValue<bool> EditingGuard(Preview ? Preview->bEditingRetiming : bUnusedGuard, true);
    Montage->Modify();
    Change(*Montage);
    Montage->MarkPackageDirty();
    Refresh();
    // All target-changing actions (including batch/reset) share preview restart
    // behavior. Prefer an affected looping section; otherwise preview the first
    // changed section in timeline order. Restart at most once per action.
    int32 RestartIndex = INDEX_NONE;
    for (const auto& Row : MontageRetiming::Describe(*Montage))
    {
        const auto* Old = Before.FindByPredicate([&Row](const auto& R) { return R.SectionIndex == Row.SectionIndex; });
        if (Old && !FMath::IsNearlyEqual(Old->TargetSeconds, Row.TargetSeconds))
        {
            if (RestartIndex == INDEX_NONE) RestartIndex = Row.SectionIndex;
            if (Preview && Preview->GetLoopingSection() == Row.Name) { RestartIndex = Row.SectionIndex; break; }
        }
    }
    if (RestartIndex != INDEX_NONE) PreviewSection(RestartIndex);
}

void SMontageRetiming::SetTarget(int32 Index, double Frames)
{
    if (!FMath::IsFinite(Frames) || Frames < 1) return;
    auto* Montage = GetMontage();
    if (!Montage) return;
    const auto Before = MontageRetiming::Describe(*Montage);
    const auto* Previous = Before.FindByPredicate([Index](const auto& R) { return R.SectionIndex == Index; });
    if (!Previous || !Previous->Error.IsEmpty() || FMath::IsNearlyEqual(Previous->TargetFrames, FMath::RoundToDouble(Frames))) return;
    Edit([Index, Frames](UAnimMontage& M) {
        for (const auto& Row : MontageRetiming::Describe(M)) if (Row.SectionIndex == Index)
        {
            auto* D = GetOrAddSection(M, Index);
            D->TargetSeconds = FMath::RoundToDouble(Frames) / Row.FramesPerSecond;
            D->bOverride = true;
        }
    }, LOCTEXT("SetTarget", "Set section target frames"));
}

void SMontageRetiming::Reset(int32 Index)
{
    Edit([Index](UAnimMontage& M) {
        ResetSection(M, Index);
    }, LOCTEXT("ResetTransaction", "Reset section timing"), false);
}

void SMontageRetiming::ResetSection(UAnimMontage& M, int32 Index)
{
    if (!M.CompositeSections.IsValidIndex(Index)) return;
    // Detach instead of mutating: copied sections may accidentally share metadata objects.
    M.CompositeSections[Index].MetaData.RemoveAll([](const auto& D) { return D && D->template IsA<UMontageRetimingSection>(); });
    GetOrAddSection(M, Index);
}

void SMontageRetiming::Batch(bool bScale)
{
    if (Selected.IsEmpty() || !FMath::IsFinite(BatchFrames) || !FMath::IsFinite(BatchPercent) || BatchFrames < 1 || BatchPercent <= 0) return;
    Edit([this, bScale](UAnimMontage& M) {
        for (const auto& Row : MontageRetiming::Describe(M)) if (Selected.Contains(Row.SectionIndex))
        {
            auto* D = GetOrAddSection(M, Row.SectionIndex);
            const double Frames = bScale ? Row.TargetFrames * BatchPercent / 100.0 : BatchFrames;
            D->TargetSeconds = FMath::Max(1.0, FMath::RoundToDouble(Frames)) / Row.FramesPerSecond;
            D->bOverride = true;
        }
    }, LOCTEXT("BatchTransaction", "Batch edit montage timings"));
}

void SMontageRetiming::EnsurePreview()
{
    const auto Pinned = Editor.Pin();
    auto* Montage = GetMontage();
    if (!Pinned || !Montage) return;
    auto* Mesh = Pinned->GetPersonaToolkit()->GetPreviewMeshComponent();
    if (Mesh) UMontageRetimingPreviewInstance::Install(*Mesh);
}

UMontageRetimingPreviewInstance* SMontageRetiming::GetPreview() const
{
    const auto Pinned = Editor.Pin();
    auto* Mesh = Pinned ? Pinned->GetPersonaToolkit()->GetPreviewMeshComponent() : nullptr;
    return Mesh && Mesh->IsPreviewOn() ? Cast<UMontageRetimingPreviewInstance>(Mesh->PreviewInstance) : nullptr;
}

void SMontageRetiming::PreviewSection(int32 Index)
{
    EnsurePreview();
    auto* M = GetMontage();
    auto* P = GetPreview();
    if (M && P && M->CompositeSections.IsValidIndex(Index))
        P->PreviewSection(Index, P->GetLoopingSection() == M->GetSectionName(Index));
}

void SMontageRetiming::ToggleSectionLoop(int32 Index)
{
    EnsurePreview();
    auto* M = GetMontage();
    auto* P = GetPreview();
    if (!M || !P || !M->CompositeSections.IsValidIndex(Index)) return;
    if (P->GetLoopingSection() == M->GetSectionName(Index)) P->ClearSectionLoop(true);
    else P->PreviewSection(Index, true);
}

#undef LOCTEXT_NAMESPACE
