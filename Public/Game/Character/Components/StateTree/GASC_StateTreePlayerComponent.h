// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/StateTreeComponent.h"
#include "GASC_StateTreePlayerComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class GASCOURSE_API UGASC_StateTreePlayerComponent : public UStateTreeComponent
{
	GENERATED_BODY()

public:
	//~ Begin UStateTreeComponent
	virtual TSubclassOf<UStateTreeSchema> GetSchema() const override;
	virtual bool SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors = false) override;
	
};
