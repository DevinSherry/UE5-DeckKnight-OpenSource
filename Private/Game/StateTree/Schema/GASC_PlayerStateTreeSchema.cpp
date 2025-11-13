// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Schema/GASC_PlayerStateTreeSchema.h"
#include "BrainComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeTypes.h"
#include "GameFramework/Pawn.h"

namespace UE::GameplayStateTree::Private
{
	static FLazyName Name_PlayerPawn = "PlayerPawn";
}

namespace UE::GameplayStateTree::Private
{
	static FLazyName Name_AbilitySystemComponent = "PlayerASC";
}

UGASC_PlayerStateTreeSchema::UGASC_PlayerStateTreeSchema(const FObjectInitializer& ObjectInitializer)
	//: PawnClass(AGASCoursePlayerCharacter::StaticClass())
{
	check(ContextDataDescs.Num() == 1 && ContextDataDescs[0].Struct == AActor::StaticClass());

	ContextActorClass = APlayerController::StaticClass();
	ContextDataDescs[0].Struct = ContextActorClass.Get();
	ContextDataDescs.Emplace(UE::GameplayStateTree::Private::Name_PlayerPawn, PawnClass.Get(), FGuid::NewGuid());
	ContextDataDescs.Emplace(UE::GameplayStateTree::Private::Name_AbilitySystemComponent, AbilitySystemComponent.Get(), FGuid::NewGuid());
}

void UGASC_PlayerStateTreeSchema::PostLoad()
{
	Super::PostLoad();
	if (ContextDataDescs.Num() > 1)
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
	
	return Super::SetContextRequirements(BrainComponent, Context, true);
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