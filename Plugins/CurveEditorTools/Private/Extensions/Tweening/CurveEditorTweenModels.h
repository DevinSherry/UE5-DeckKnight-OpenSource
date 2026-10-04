// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Abstraction/TweenModelArray.h"

class FCurveEditor;

namespace UE::CurveEditorTools
{
/** Knows of the tween models used in curve editor. */
class FCurveEditorTweenModels : public TweeningUtilsEditor::FTweenModelArray
{
public:

	explicit FCurveEditorTweenModels(const TWeakPtr<FCurveEditor> InCurveEditor);
};
}

