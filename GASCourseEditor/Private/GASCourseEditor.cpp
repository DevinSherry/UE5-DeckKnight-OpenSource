// Fill out your copyright notice in the Description page of Project Settings.

#include "GASCourseEditor.h"
#include "EdGraphUtilities.h"
#include "Pins/SGraphPinBitmaskCustom.h"
#include "Debugging/GASC_MeleeRewindExtension.h"
#include "Features/IModularFeatures.h"

struct FGASCourseBitmaskPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* InPin) const override
	{
		if (InPin && InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int && InPin->PinType.PinSubCategory == UEdGraphSchema_K2::PSC_Bitmask)
		{
			return SNew(SGraphPinBitmaskCustom, InPin);
		}
		return nullptr;
	}
};

void FGASCourseEditorModule::StartupModule()
{
	MeleeRewindExtension = MakeShared<FGASC_MeleeRewindExtension>();
	IModularFeatures::Get().RegisterModularFeature(IRewindDebuggerExtension::ModularFeatureName, MeleeRewindExtension.Get());
	BitmaskPinFactory = MakeShared<FGASCourseBitmaskPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(BitmaskPinFactory);
}

void FGASCourseEditorModule::ShutdownModule()
{
	if (MeleeRewindExtension.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(IRewindDebuggerExtension::ModularFeatureName, MeleeRewindExtension.Get());
		MeleeRewindExtension->Clear(nullptr);
		MeleeRewindExtension.Reset();
	}
	if (BitmaskPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(BitmaskPinFactory);
		BitmaskPinFactory.Reset();
	}
}

IMPLEMENT_MODULE(FGASCourseEditorModule, GASCourseEditor);
