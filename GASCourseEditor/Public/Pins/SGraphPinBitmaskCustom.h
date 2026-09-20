// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KismetPins/SGraphPinNum.h"

class SGraphPinBitmaskCustom : public SGraphPinNum<int32>
{
public:
	SLATE_BEGIN_ARGS(SGraphPinBitmaskCustom) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
};
