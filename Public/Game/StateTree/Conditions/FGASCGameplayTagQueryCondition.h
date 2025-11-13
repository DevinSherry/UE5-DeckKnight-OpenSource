// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StateTreeConditionBase.h"
#include "FGASCGameplayTagQueryCondition.generated.h"

/**
 * Struct containing instance data required for a Gameplay Tag Query condition.
 */

USTRUCT()
struct FGASCGameplayTagQueryConditionInstanceData
{
	GENERATED_BODY()
	
	/** Actor to use owning tags as part of the query match condition. */
	UPROPERTY(EditAnywhere, Category = Condition, meta=(EditCondition = "!bUseAlternateTagContainer", EditConditionHides))
	AActor* ActorToCheck;
	
	/** Query to match against */
	UPROPERTY(EditAnywhere, Category = Condition)
	FGameplayTagQuery TagQuery;
	
	UPROPERTY(EditAnywhere, Category = Condition)
	bool bUseAlternateTagContainer = false;

	/** Container that needs to match the query. */
	UPROPERTY(EditAnywhere, Category = Condition,meta=(EditCondition = "bUseAlternateTagContainer",EditConditionHides))
	FGameplayTagContainer AlternativeTagContainer;

};

/**
 * Struct defining a state tree condition that evaluates a Gameplay Tag Query against a tag container.
 *
 * This condition checks if a specified tag container matches the provided Gameplay Tag Query. The result can be inverted
 * based on the `bInvert` property. It is used within the StateTree system to evaluate conditions during execution.
 *
 * Key Responsibilities:
 * - Links the condition during the StateTree linking process.
 * - Retrieves the appropriate tag container to evaluate against the query.
 * - Executes the condition test to determine whether the tag container matches the Gameplay Tag Query.
 * - Provides description, icon information, and validation support for editor usage.
 *
 * Key Properties:
 * - `bInvert`: Inverts the result of the condition when set to true.
 *
 * Used with:
 * - Instance data of type `FGASCGameplayTagQueryConditionInstanceData`.
 */

USTRUCT(DisplayName="GAS Course Does Container Match Tag Query", Category="GASCourse| Gameplay Tags")
struct FGASCGameplayTagQueryCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGASCGameplayTagQueryConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	FGameplayTagContainer GetTagContainer(const FInstanceDataType& InstanceData) const;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Tag");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::DarkBlue;
	}

	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;

#endif

	UPROPERTY(EditAnywhere, Category = Condition)
	bool bInvert = false;
};