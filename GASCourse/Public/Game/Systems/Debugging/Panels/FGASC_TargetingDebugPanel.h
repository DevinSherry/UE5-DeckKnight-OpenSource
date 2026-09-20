#pragma once
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

class FGASC_TargetingDebugRecorder;
class UGameInstance;

class GASCOURSE_API FGASC_TargetingDebugPanel : public IIGASCDebugPanel
{
public:
	explicit FGASC_TargetingDebugPanel(UGameInstance* GameInstance);
	virtual ~FGASC_TargetingDebugPanel() override;
	virtual const char* GetDebugPanelName() const override { return "Targeting History"; }
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void OnDebugPanelClosed() override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override {}
private:
	void SetGamePaused(bool bPaused);
	bool bPanelWasOpen = false;
	FString PauseError;
	TUniquePtr<FGASC_TargetingDebugRecorder> Recorder;
};
