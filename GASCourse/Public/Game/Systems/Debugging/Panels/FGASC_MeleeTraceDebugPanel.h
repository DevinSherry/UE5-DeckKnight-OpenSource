#pragma once
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

class UWorld;

class GASCOURSE_API FGASC_MeleeTraceDebugPanel : public IIGASCDebugPanel
{
public:
	explicit FGASC_MeleeTraceDebugPanel(UWorld* InWorld) : World(InWorld) {}
	virtual const char* GetDebugPanelName() const override { return "Melee Traces"; }
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void OnDebugPanelClosed() override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override { CachedPawns = MoveTemp(Pawns); }
private:
	TWeakObjectPtr<UWorld> World;
	bool bPlayHistory = false;
	bool bPanelWasOpen = false;
	FString ControlError;
	float PlaybackSpeed = 1.f;
};
