// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "GASC_ShakeCurveValidator.generated.h"

/**
 * Flags Shake Target Character nodes that were dropped into a Gameplay Ability Blueprint without a
 * curve assigned. The task cannot do anything without one, so this catches it on asset save or
 * during a Validate Assets pass rather than leaving it to be discovered at runtime.
 *
 * This lives in Data Validation rather than being a compile-time node warning because
 * UK2Node_LatentAbilityCall - the node ability tasks use - is declared without an export macro and
 * so cannot be subclassed from another module.
 *
 * Only a static check: a curve supplied through a variable that happens to be null at runtime is
 * invisible here. The task's own Activate() guard and OnShakeFailed remain the backstop for that.
 */
UCLASS()
class UGASC_ShakeCurveValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:

	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
};
