#include "MontageRetimingPreviewInstance.h"
#include "MontageRetimingAnimInstance.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/AnimMontage.h"

void UMontageRetimingPreviewInstance::Montage_Advance(float DeltaSeconds)
{
    MontageRetiming::Advance(*this, DeltaSeconds, [this](float Step) { Super::Montage_Advance(Step); });
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
