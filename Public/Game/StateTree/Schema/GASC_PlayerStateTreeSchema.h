// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/StateTreeComponentSchema.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "GASC_PlayerStateTreeSchema.generated.h"

class APlayerController;

/**
 * @class UGASC_PlayerStateTreeSchema
 * @brief Defines a schema for StateTree that is specific to a player character and its associated components.
 *
 * This class extends the UStateTreeComponentSchema to provide predefined context descriptions that include
 * the player's pawn class and their ability system component. It serves as a base for associating and validating
 * the relationships between gameplay components like the player's character and ability system, within a StateTree execution.
 *
 * The schema ensures consistency in data requirements and context initialization for StateTree executions.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, Meta = (DisplayName = "StateTree Player Component", CommonSchema))
class GASCOURSE_API UGASC_PlayerStateTreeSchema : public UStateTreeComponentSchema
{
	GENERATED_BODY()
public:
	UGASC_PlayerStateTreeSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	UPROPERTY(EditAnywhere, Category = "Defaults", NoClear)
	TSubclassOf<AGASCoursePlayerCharacter> PawnClass = AGASCoursePlayerCharacter::StaticClass();

	UPROPERTY(EditAnywhere, Category = "Defaults", NoClear)
	TSubclassOf<UGASCourseAbilitySystemComponent> AbilitySystemComponent = UGASCourseAbilitySystemComponent::StaticClass();

	//~ Begin UStateTreeComponentSchema
public:
	virtual void PostLoad() override;
	static bool SetContextRequirements(UBrainComponent& BrainComponent, FStateTreeExecutionContext& Context, bool bLogErrors = false);

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	
	//~ End UStateTreeComponentSchema
	
};
