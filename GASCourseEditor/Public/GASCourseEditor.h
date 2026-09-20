// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGASCourseEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<struct FGraphPanelPinFactory> BitmaskPinFactory;
	TSharedPtr<class FGASC_MeleeRewindExtension> MeleeRewindExtension;
};
