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
};
