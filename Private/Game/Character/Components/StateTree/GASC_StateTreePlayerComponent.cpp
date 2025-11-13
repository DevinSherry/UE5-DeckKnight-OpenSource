// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/StateTree/GASC_StateTreePlayerComponent.h"
#include "StateTreeExecutionContext.h"
#include "Game/StateTree/Schema/GASC_PlayerStateTreeSchema.h"

TSubclassOf<UStateTreeSchema> UGASC_StateTreePlayerComponent::GetSchema() const
{
	return UGASC_PlayerStateTreeSchema::StaticClass();
}

bool UGASC_StateTreePlayerComponent::SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors)
{
	Context.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateUObject(this, &UGASC_StateTreePlayerComponent::CollectExternalData));
	return UGASC_PlayerStateTreeSchema::SetContextRequirements(*this, Context, bLogErrors);
}
