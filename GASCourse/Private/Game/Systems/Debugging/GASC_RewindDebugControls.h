#pragma once
#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
// Shared presentation, with per-panel settings content owned by the caller.
void GASC_DrawRewindWorldControls(UWorld* World, FString& Error);
bool GASC_BeginRewindSettings();
#endif
