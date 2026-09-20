// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * @class FGASC_WaveManagerPanel
 * @brief This class implements a debug panel for managing waves in the GAS Course system.
 *
 * The FGASC_WaveManagerPanel class provides functionalities to display and control
 * debug information related to waves in the system. It is an implementation of
 * the IIGASCDebugPanel interface.
 */
class GASCOURSE_API FGASC_WaveManagerPanel : public IIGASCDebugPanel
{
public:
	FGASC_WaveManagerPanel();
	~FGASC_WaveManagerPanel();

	virtual const char* GetDebugPanelName() const override {return "Wave Manager Panel";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;

	void RequestAsyncEnemyLoad();

	static TSubclassOf<APawn> SelectedEnemyToSpawn;

private:
	
	TArray<TSoftClassPtr<APawn>> EnemyList;
	TArray<TSubclassOf<APawn>> EnemyListLoaded;
};
