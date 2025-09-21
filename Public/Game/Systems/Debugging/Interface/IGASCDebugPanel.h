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

	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) = 0;
	
	TArray<TWeakObjectPtr<APawn>> CachedPawns;
};
