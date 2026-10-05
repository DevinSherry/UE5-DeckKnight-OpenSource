#include "MontageRetimingPreviewInstance.h"
#include "MontageRetimingAnimInstance.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"

void UMontageRetimingPreviewInstance::Montage_Advance(float DeltaSeconds)
{
    if (!LoopingSection.IsNone())
    {
        auto* Montage = Cast<UAnimMontage>(GetCurrentAsset());
        auto* Instance = Montage ? GetActiveInstanceForMontage(Montage) : nullptr;
        // Scrubbing/transport controls change position or replace the preview instance
        // outside this advancement path. Release our routing before advancing again.
        if (!Montage || Montage != LoopMontage || Montage->GetSectionIndex(LoopingSection) == INDEX_NONE
            || !Instance || Instance->GetInstanceID() != LoopInstanceID || !IsPlaying()
            || !FMath::IsNearlyEqual(Instance->GetPosition(), ExpectedPosition, UE_SMALL_NUMBER)) ClearSectionLoop();
        else Montage_SetNextSection(LoopingSection, LoopingSection, Montage);
    }
    // Editor previews hold their final pose instead of blending to reference pose.
    // This flag belongs to the transient playback instance, not the saved asset.
    for (auto* Instance : MontageInstances)
        if (Instance && Instance->Montage == GetCurrentAsset()) Instance->bEnableAutoBlendOut = false;
    MontageRetiming::Advance(*this, DeltaSeconds, [this](float Step) { Super::Montage_Advance(Step); });
    if (!LoopingSection.IsNone())
        if (auto* Instance = GetActiveInstanceForMontage(LoopMontage.Get())) ExpectedPosition = Instance->GetPosition();
}

void UMontageRetimingPreviewInstance::PreviewSection(int32 SectionIndex, bool bLoop)
{
    auto* Montage = Cast<UAnimMontage>(GetCurrentAsset());
    if (!Montage || !Montage->CompositeSections.IsValidIndex(SectionIndex)) return;
    const bool bHadSectionLoop = !LoopingSection.IsNone();
    if (bLoop && !bHadSectionLoop) bPreviousPreviewLooping = IsLooping();
    const bool bNormalLooping = bHadSectionLoop ? bPreviousPreviewLooping : IsLooping();
    LoopingSection = bLoop ? Montage->GetSectionName(SectionIndex) : NAME_None;
    SetLooping(!bLoop && bNormalLooping);
    MontagePreview_SetReverse(false);
    MontagePreview_PreviewNormal(SectionIndex, true);
    // Restarting replaces the montage instance. Publish its weights/evaluation
    // immediately, just as the native single-node RestartMontage path does.
    // Otherwise the preview proxy can keep the stopped instance's zero weights
    // and evaluate the reference pose despite a running montage playhead.
    GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>().ResetWeightInfo();
    UpdateMontageWeightForTimeSkip(Montage->BlendIn.GetBlendTime());
    if (bLoop || !bNormalLooping) MontagePreview_ResetSectionsOrder();
    if (auto* Instance = GetActiveInstanceForMontage(Montage)) Instance->bEnableAutoBlendOut = false;
    if (bLoop)
    {
        // Native normal preview loops the entire linked chain. Restore its links,
        // then loop only this section on the transient montage instance.
        MontagePreview_ResetSectionsOrder();
        Montage_SetNextSection(LoopingSection, LoopingSection, Montage);
        MontagePreviewStartSectionIdx = SectionIndex;
        LoopMontage = Montage;
        if (auto* Instance = GetActiveInstanceForMontage(Montage))
        {
            LoopInstanceID = Instance->GetInstanceID();
            ExpectedPosition = Instance->GetPosition();
        }
    }
}

void UMontageRetimingPreviewInstance::ClearSectionLoop(bool bRestorePreviewLoop)
{
    if (LoopingSection.IsNone()) return;
    LoopingSection = NAME_None;
    LoopMontage.Reset();
    LoopInstanceID = INDEX_NONE;
    // Explicitly releasing the row loop returns to Persona's repeating preview.
    // Do not depend on a cached proxy flag: scrubbing/transport can leave it false
    // even though the user expects the normal full-chain preview to repeat.
    SetLooping(bRestorePreviewLoop);
    // Restore authored links exactly, including explicit section loops.
    MontagePreview_ResetSectionsOrder();
    if (IsLooping()) MontagePreview_SetLoopNormal(true);
    if (bRestorePreviewLoop) MontagePreview_SetPlaying(true);
    if (auto* Instance = GetActiveMontageInstance()) Instance->bEnableAutoBlendOut = false;
}

void UMontageRetimingPreviewInstance::HandleExternalEdit(UObject* Object)
{
    auto* Montage = Cast<UAnimMontage>(GetCurrentAsset());
    if (bEditingRetiming || LoopingSection.IsNone() || !Montage || !Object) return;
    if (Object == Montage || Object->IsIn(Montage)) { ClearSectionLoop(); return; }
    for (const auto& Slot : Montage->SlotAnimTracks)
        for (const auto& Segment : Slot.AnimTrack.AnimSegments)
            if (UAnimSequenceBase* Source = Segment.GetAnimReference())
                if (Object == Source || Object->IsIn(Source)) { ClearSectionLoop(); return; }
}

void UMontageRetimingPreviewInstance::Install(UDebugSkelMeshComponent& Mesh)
{
    if (!Mesh.PreviewInstance || !Mesh.IsPreviewOn() || Mesh.PreviewInstance->IsA<UMontageRetimingPreviewInstance>()) return;
    auto* Montage = Cast<UAnimMontage>(Mesh.PreviewInstance->GetCurrentAsset());
    if (!Montage) return;
    const float Position = Mesh.PreviewInstance->GetCurrentTime();
    const float PlayRate = Mesh.PreviewInstance->GetPlayRate();
    const bool bPlaying = Mesh.PreviewInstance->IsPlaying();
    const bool bLooping = Mesh.PreviewInstance->IsLooping();
    Mesh.EnablePreview(false, Montage);
    Mesh.PreviewInstance = NewObject<UMontageRetimingPreviewInstance>(&Mesh, NAME_None, RF_Transient | RF_Transactional);
    Mesh.PreviewInstance->InitializeAnimation();
    Mesh.EnablePreview(true, Montage);
    Mesh.PreviewInstance->SetLooping(bLooping);
    Mesh.PreviewInstance->SetPlayRate(PlayRate);
    Mesh.PreviewInstance->SetPosition(Position, false);
    Mesh.PreviewInstance->SetPlaying(bPlaying);
}
