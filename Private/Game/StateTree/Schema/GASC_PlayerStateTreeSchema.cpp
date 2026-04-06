// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Schema/GASC_PlayerStateTreeSchema.h"
#include "BrainComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeTypes.h"
#include "GameFramework/Pawn.h"

namespace UE::GameplayStateTree::Private
{
	static FLazyName Name_PlayerPawn = "PlayerPawn";
	static const FGuid PlayerPawnGuid(0x12345678, 0x11111111, 0x22222222, 0x33333333);

	static FLazyName Name_AbilitySystemComponent = "PlayerASC";
	static const FGuid AbilitySystemGuid(0x87654321, 0x44444444, 0x55555555, 0x66666666);
}

UGASC_PlayerStateTreeSchema::UGASC_PlayerStateTreeSchema(const FObjectInitializer& ObjectInitializer)
{
	check(ContextDataDescs.Num() >= 1);

	ContextActorClass = APlayerController::StaticClass();
	ContextDataDescs[0].Struct = ContextActorClass.Get();

	if (ContextDataDescs.Num() < 2)
	{
		ContextDataDescs.Emplace(
			UE::GameplayStateTree::Private::Name_PlayerPawn,
			PawnClass.Get(),
			UE::GameplayStateTree::Private::PlayerPawnGuid);
	}
	else
	{
		ContextDataDescs[1].Name = UE::GameplayStateTree::Private::Name_PlayerPawn;
		ContextDataDescs[1].Struct = PawnClass.Get();
	}

	if (ContextDataDescs.Num() < 3)
	{
		ContextDataDescs.Emplace(
			UE::GameplayStateTree::Private::Name_AbilitySystemComponent,
			AbilitySystemComponent.Get(),
			UE::GameplayStateTree::Private::AbilitySystemGuid);
	}
	else
	{
		ContextDataDescs[2].Name = UE::GameplayStateTree::Private::Name_AbilitySystemComponent;
		ContextDataDescs[2].Struct = AbilitySystemComponent.Get();
	}
}

void UGASC_PlayerStateTreeSchema::PostLoad()
{
	Super::PostLoad();
	if (ContextDataDescs.Num() > 3)
	{
		ContextDataDescs[1].Struct = PawnClass.Get();
		ContextDataDescs[2].Struct = AbilitySystemComponent.Get();
	}
}

bool UGASC_PlayerStateTreeSchema::SetContextRequirements(UBrainComponent& BrainComponent, FStateTreeExecutionContext& Context, bool bLogErrors)
{
	if (!Context.IsValid())
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(BrainComponent.GetOwner());
	if (!PC)
		return false;
	AGASCoursePlayerCharacter* PlayerCharacter = PC ? Cast<AGASCoursePlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!PlayerCharacter)
	{
		return false;
	}
	UGASCourseAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
	
	const FName PlayerPawnName = UE::GameplayStateTree::Private::Name_PlayerPawn;
	const FName AbilitySystemComponentName = UE::GameplayStateTree::Private::Name_AbilitySystemComponent;
	
	//Context.SetContextDataByName(TEXT("Context Actor Class"), FStateTreeDataView(PC));
	Context.SetContextDataByName(PlayerPawnName, FStateTreeDataView(PlayerCharacter));
	Context.SetContextDataByName(AbilitySystemComponentName, FStateTreeDataView(ASC));
	
	return Super::SetContextRequirements(BrainComponent, Context, bLogErrors);
}

#if WITH_EDITOR

void UGASC_PlayerStateTreeSchema::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (const FProperty* Property = PropertyChangedEvent.Property)
	{
		if (Property->GetOwnerClass() == StaticClass()
			&& Property->GetFName() == GET_MEMBER_NAME_CHECKED(UGASC_PlayerStateTreeSchema, PawnClass))
		{
			ContextDataDescs[1].Struct = PawnClass.Get();
		}
		if (Property->GetOwnerClass() == StaticClass() && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UGASC_PlayerStateTreeSchema, AbilitySystemComponent))
		{
			ContextDataDescs[2].Struct = AbilitySystemComponent.Get();
		}
	}
}
#endif // WITH_EDITOR