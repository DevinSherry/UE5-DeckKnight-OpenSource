#pragma once

#include "AnimPreviewInstance.h"
#include "MontageRetimingPreviewInstance.generated.h"

UCLASS(Transient)
class UMontageRetimingPreviewInstance : public UAnimPreviewInstance
{
    GENERATED_BODY()
public:
    virtual void Montage_Advance(float DeltaSeconds) override;
    static void Install(class UDebugSkelMeshComponent& Mesh);
    /** Preview-only routing; never changes the montage's authored section links. */
    void PreviewSection(int32 SectionIndex, bool bLoop);
    void ClearSectionLoop(bool bRestorePreviewLoop = false);
    FName GetLoopingSection() const { return LoopingSection; }
    void HandleExternalEdit(UObject* Object);
    bool bEditingRetiming = false;
private:
    friend class FRetimingSectionPreviewTest;
    FName LoopingSection;
    TWeakObjectPtr<class UAnimMontage> LoopMontage;
    int32 LoopInstanceID = INDEX_NONE;
    float ExpectedPosition = 0.f;
    bool bPreviousPreviewLooping = false;
};
