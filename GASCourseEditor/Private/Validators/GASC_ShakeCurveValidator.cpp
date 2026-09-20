// Fill out your copyright notice in the Description page of Project Settings.


#include "Validators/GASC_ShakeCurveValidator.h"

#include "Abilities/GameplayAbility.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_BaseAsyncTask.h"
#include "Misc/DataValidation.h"
#include "Game/GameplayAbilitySystem/Tasks/Gameplay/GASCourse_ShakeCharacter.h"

#define LOCTEXT_NAMESPACE "GASC_ShakeCurveValidator"

namespace GASC_ShakeCurveValidatorNames
{
	/** Both shake factory functions name their curve parameter this. */
	static const FName ShakeCurvePinName(TEXT("ShakeCurve"));
}

bool UGASC_ShakeCurveValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& InContext) const
{
	// Only Gameplay Ability Blueprints can host an ability task node, so everything else is skipped
	// outright rather than paying a graph walk. Mirrors the graph-compatibility test the engine's
	// own ability task node applies.
	const UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);

	return Blueprint
		&& Blueprint->GeneratedClass
		&& Blueprint->GeneratedClass->IsChildOf(UGameplayAbility::StaticClass());
}

EDataValidationResult UGASC_ShakeCurveValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData,
	UObject* InAsset, FDataValidationContext& Context)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	int32 MissingCurveCount = 0;

	for (const UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_BaseAsyncTask* AsyncTaskNode = Cast<UK2Node_BaseAsyncTask>(Node);
			if (!AsyncTaskNode)
			{
				continue;
			}

			// Identified by the factory function's owning class. ProxyClass would be the obvious
			// thing to read, but it is protected on UK2Node_BaseAsyncTask; GetFactoryFunction is
			// public and exported.
			const UFunction* FactoryFunction = AsyncTaskNode->GetFactoryFunction();
			if (!FactoryFunction || FactoryFunction->GetOwnerClass() != UGASCourse_ShakeCharacter::StaticClass())
			{
				continue;
			}

			// The deprecated ShakeTargetCharacter overload is owned by the same class but has no
			// curve pin, so it falls out here rather than producing a false positive.
			const UEdGraphPin* CurvePin = AsyncTaskNode->FindPin(GASC_ShakeCurveValidatorNames::ShakeCurvePinName, EGPD_Input);
			if (!CurvePin)
			{
				continue;
			}

			if (CurvePin->DefaultObject == nullptr && CurvePin->LinkedTo.Num() == 0)
			{
				++MissingCurveCount;

				AssetWarning(InAsset, FText::Format(
					LOCTEXT("ShakeNodeMissingCurve", "'{0}' in graph '{1}' has no shake curve assigned. A valid curve is required - the shake will fail at runtime and do nothing."),
					AsyncTaskNode->GetNodeTitle(ENodeTitleType::ListView),
					FText::FromName(Graph->GetFName())));
			}
		}
	}

	if (MissingCurveCount == 0)
	{
		AssetPasses(InAsset);
	}

	// Warnings deliberately do not invalidate the asset: a missing curve is an authoring mistake
	// worth surfacing, not something that should block a save or fail CI. Swap to AssetFails and
	// EDataValidationResult::Invalid if it should.
	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
