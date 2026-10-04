#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MontageRetimingAnimInstance.generated.h"

/** Ordinary Montage_Play calls honor opted-in assets in Editor, Development and Shipping. */
UCLASS(Transient, Blueprintable)
class MONTAGERETIMING_API UMontageRetimingAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void Montage_Advance(float DeltaSeconds) override;
};

namespace MontageRetiming
{
    /** Shared by runtime and Persona; NativeAdvance must call the superclass implementation. */
    MONTAGERETIMING_API void Advance(UAnimInstance& Instance, float DeltaSeconds, TFunctionRef<void(float)> NativeAdvance);
}
