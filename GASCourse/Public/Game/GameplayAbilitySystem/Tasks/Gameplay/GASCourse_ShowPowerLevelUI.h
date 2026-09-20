// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Abilities/Tasks/AbilityTask.h"
#include "Components/WidgetComponent.h"
#include "GASCourse_ShowPowerLevelUI.generated.h"

class UEnhancedPlayerInput;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGasCourse_ShowPowerLevelUI, UUserWidget*, PowerLevelWidget);

/**
 * @class UGASCourse_ShowPowerLevelUI
 * @brief A class responsible for handling and displaying the Power Level UI in the game.
 *
 * This class manages the Power Level user interface elements and coordinates updates
 * to reflect the current state of the player's power level in the game. It serves
 * as a connection between the gameplay logic and the UI framework, ensuring that
 * the displayed information is accurate and responsive.
 *
 * The class includes functionality for initializing the UI, updating the power level
 * values based on gameplay changes, and managing visibility or other UI-related transitions.
 * It is intended to be used in conjunction with gameplay abilities or systems
 * affecting the player's power level.
 *
 * Responsibilities of the class include:
 * - Initializing the power level UI during gameplay.
 * - Listening for power level changes or relevant events.
 * - Updating UI elements to reflect updated power level values.
 * - Managing the visibility and lifecycle of the Power Level UI elements.
 */
UCLASS()
class GASCOURSE_API UGASCourse_ShowPowerLevelUI : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FGasCourse_ShowPowerLevelUI OnPowerLevelUICreated;
	
protected:
	virtual void Activate() override;
	
public:
	virtual void OnDestroy(bool AbilityEnded) override;
	
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGASCourse_ShowPowerLevelUI* WaitPowerLevelSelectionWithUI(UGameplayAbility* OwningAbility, AActor* TargetActor, TSubclassOf<UUserWidget> PowerLevelUIWidgetClass, FVector2D WidgetDrawSize, FTransform RelativeWidgetTransform, EWidgetSpace WidgetSpace = EWidgetSpace::Screen, bool bAttachWidgetToTargetActor = true);
	
private:
	
	UFUNCTION()
	bool InstantiatePowerLevelUIWidget(const APlayerController* PC);

private:
	
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;
	
	UPROPERTY()
	TWeakObjectPtr<APlayerController> TargetPlayerController;
	
	UPROPERTY()
	TSubclassOf<UUserWidget> PowerLevelUIWidgetClass;
	
	UPROPERTY()
	TWeakObjectPtr<UUserWidget> WidgetInstance;
	
	UPROPERTY()
	EWidgetSpace WidgetSpace = EWidgetSpace::Screen;
	
	UPROPERTY()
	FVector2D WidgetDrawSize;
	
	UPROPERTY()
	FTransform RelativeWidgetTransform;
	
	UPROPERTY()
	bool bAttachWidgetToTargetActor = true;
	
	UPROPERTY()
	TWeakObjectPtr<UWidgetComponent> PowerLevelUIWidgetComponent;
	
};
