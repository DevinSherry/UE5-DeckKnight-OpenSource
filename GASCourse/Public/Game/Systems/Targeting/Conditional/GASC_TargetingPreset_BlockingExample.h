#pragma once
#include "TargetingSystem/TargetingPreset.h"
#include "GASC_TargetingPreset_BlockingExample.generated.h"
/** Create a Data Asset of this type for an editable example: nearby pawns, then conditional defensive target preferences. */
UCLASS(DisplayName="Targeting Example: Blocking Condition and Children")
class GASCOURSE_API UGASC_TargetingPreset_BlockingExample : public UTargetingPreset
{
 GENERATED_BODY()
public:
 UGASC_TargetingPreset_BlockingExample(const FObjectInitializer& ObjectInitializer);
};
