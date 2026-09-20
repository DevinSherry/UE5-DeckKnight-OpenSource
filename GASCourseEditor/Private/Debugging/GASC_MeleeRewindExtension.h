#pragma once

#include "CoreMinimal.h"
#include "IRewindDebuggerExtension.h"

class UGASC_MeleeTrace_Subsystem;

/** Connects the current PIE session's bounded melee history to the Rewind playhead. */
class FGASC_MeleeRewindExtension : public IRewindDebuggerExtension
{
public:
	virtual ~FGASC_MeleeRewindExtension() = default;
	virtual FString GetName() override { return TEXT("GASCourse Melee Traces"); }
	virtual void RecordingStarted(IRewindDebugger* Debugger) override;
	virtual void RecordingStopped(IRewindDebugger* Debugger) override;
	virtual void Update(float DeltaTime, IRewindDebugger* Debugger) override;
	virtual void Clear(IRewindDebugger* Debugger) override;
private:
	struct FCapture
	{
		TWeakObjectPtr<UGASC_MeleeTrace_Subsystem> Subsystem;
		bool bWasRecording = false;
	};
	TArray<FCapture> Captures;
};
