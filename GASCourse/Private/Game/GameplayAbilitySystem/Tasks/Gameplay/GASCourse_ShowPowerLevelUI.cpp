// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/Gameplay/GASCourse_ShowPowerLevelUI.h"
#include "GameFramework/Character.h"

UGASCourse_ShowPowerLevelUI::UGASCourse_ShowPowerLevelUI(const FObjectInitializer& ObjectInitializer)
{
}

void UGASCourse_ShowPowerLevelUI::Activate()
{
	Super::Activate();
	
	AActor* Actor = TargetActor.Get();
	if (!IsValid(Actor))
	{
		EndTask();
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!IsValid(Character))
	{
		EndTask();
		return;
	}
	
	APlayerController* PC = Character->GetController<APlayerController>();
	if (!IsValid(PC))
	{
		EndTask();
		return;
	}
	
	if (!InstantiatePowerLevelUIWidget(PC))
	{
		EndTask();
		return;
	}
	
	OnPowerLevelUICreated.Broadcast(WidgetInstance.Get());
}

void UGASCourse_ShowPowerLevelUI::OnDestroy(bool AbilityEnded)
{
	if (UWidgetComponent* WidgetComponent = PowerLevelUIWidgetComponent.Get())
	{
		WidgetComponent->DestroyComponent();
	}
	
	if (UUserWidget* Widget = WidgetInstance.Get())
	{
		Widget->RemoveFromParent();
	}

	Super::OnDestroy(AbilityEnded);
}

UGASCourse_ShowPowerLevelUI* UGASCourse_ShowPowerLevelUI::WaitPowerLevelSelectionWithUI(UGameplayAbility* OwningAbility,
	AActor* TargetActor, TSubclassOf<UUserWidget> PowerLevelUIWidgetClass, FVector2D WidgetDrawSize,
	FTransform RelativeWidgetTransform, EWidgetSpace WidgetSpace, bool bAttachWidgetToTargetActor)
{
	UGASCourse_ShowPowerLevelUI* Task = NewAbilityTask<UGASCourse_ShowPowerLevelUI>(OwningAbility);
	
	Task->bAttachWidgetToTargetActor = bAttachWidgetToTargetActor;
	Task->WidgetSpace = WidgetSpace;
	Task->WidgetDrawSize = WidgetDrawSize;
	
	Task->TargetActor = TargetActor ? TargetActor : OwningAbility->GetAvatarActorFromActorInfo();

	Task->RelativeWidgetTransform = RelativeWidgetTransform;
	Task->bAttachWidgetToTargetActor = bAttachWidgetToTargetActor;
	Task->PowerLevelUIWidgetClass = PowerLevelUIWidgetClass;

	return Task;
}

bool UGASCourse_ShowPowerLevelUI::InstantiatePowerLevelUIWidget(const APlayerController* PC)
{
	if (!PowerLevelUIWidgetClass)
	{
		return false;
	}
	
	PowerLevelUIWidgetComponent = NewObject<UWidgetComponent>(PC->GetPawn());
	if (!IsValid(PowerLevelUIWidgetComponent.Get()))
	{
		return false;
	}
	
	WidgetInstance = CreateWidget<UUserWidget>(
	GetWorld(), PowerLevelUIWidgetClass);
	
	PowerLevelUIWidgetComponent->SetWidgetSpace(WidgetSpace);
	PowerLevelUIWidgetComponent->SetWidget(WidgetInstance.Get());
	PowerLevelUIWidgetComponent->SetDrawSize(WidgetDrawSize);
	
	if (bAttachWidgetToTargetActor)
	{
		PowerLevelUIWidgetComponent->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		PowerLevelUIWidgetComponent->SetRelativeTransform(RelativeWidgetTransform);
	}
	
	return true;
	
}
