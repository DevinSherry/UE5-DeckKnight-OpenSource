// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Interface.h"
#include "IGASCDebugPanel.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIGASCDebugPanel : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GASCOURSE_API IIGASCDebugPanel
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual const char* GetDebugPanelName() const = 0;
	virtual void DrawDebugPanel(bool & bOpen) = 0;
	/** Release transient world visuals when the hub closes a panel. */
	virtual void OnDebugPanelClosed() {}

	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) = 0;

	/**
	 * Return true when this panel should force itself open, for panels driven by something outside
	 * the hub - a debug flag on a gameplay object, a console variable.
	 *
	 * Edge-triggered by contract: the hub consumes the request, so return true only on the
	 * transition into wanting to be open, never continuously. That way the user can still close the
	 * panel afterwards without it reappearing the next frame.
	 *
	 * Defaults to false, so panels that are only ever opened from the hub need not implement it.
	 */
	virtual bool ConsumeAutoOpenRequest() { return false; }
	
	TArray<TWeakObjectPtr<APawn>> CachedPawns;
};
