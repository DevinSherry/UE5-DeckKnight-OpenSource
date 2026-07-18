// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "MVVMViewModelBase.h"
#include "Game/HUD/ViewModels/Health/GASC_UVM_Health.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "GASC_HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthViewModelInstantiated, UGASC_UVM_Health*, HealthViewModel);

/**
 * This class represents a health component for an actor in the game.
 * It provides functionality for managing the current and maximum health values of the actor.
 * The health component also supports replication to ensure consistent gameplay across the network.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class GASCOURSE_API UGASC_HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Default constructor for the UGASC_HealthComponent class.
	 * Sets up the component to be initialized when the game starts and to be ticked every frame.
	 * It also enables replication for the component.
	 */
	UGASC_HealthComponent();
	
	virtual void BeginPlay() override;

	/**
	 * Initializes the view model for the UGASC_HealthComponent.
	 *
	 * The view model is responsible for managing the health-related properties and functions of the UGASC_HealthComponent.
	 * This method creates a UGASC_UVM_Health instance and adds it to the global view model collection.
	 * It then sets the HealthViewModel property of the UGASC_HealthComponent to the created instance.
	 *
	 * Prerequisites:
	 * - The UGASC_HealthComponent must be attached to an actor.
	 * - The owning game instance must have a valid UMVVMGameSubsystem instance.
	 *
	 * @see UGASC_HealthComponent
	 * @see UMVVMGameSubsystem
	 * @see UMVVMViewModelCollectionObject
	 * @see UGASC_UVM_Health
	 * @see FMVVMViewModelContext
	 * @see CharacterHealthViewModelContextClass
	 * @see CharacterHealthContextName
	 * @see HealthViewModel
	 */
	
protected:
	
	UFUNCTION()
	void InitializeViewModel();

	/**
	 * @brief CurrentHealth is a float variable that represents the current health of the actor.
	 * It is replicated using the "OnRep_CurrentHealth" function.
	 */
	UPROPERTY(BlueprintReadWrite)
	float CurrentHealth = 0.0f;

	/**
	 * @brief The maximum health of the actor.
	 *
	 * This variable represents the maximum health value that an actor can have.
	 * It is used in conjunction with the current health variable to determine the health status of the actor.
	 * The value of this variable should be set according to the specific requirements of the game or application.
	 *
	 * @details
	 * - The variable is of type `float`.
	 * - It is marked with the `ReplicatedUsing` attribute, indicating that changes to its value will be replicated to all clients.
	 * - The default value of the variable is `0.0f`.
	 *
	 * @note
	 * - Modifying the value of this variable directly may have unintended side effects.
	 *   It is recommended to use appropriate functions or methods to update the health value.
	 * - This variable can be accessed and modified during runtime, as needed.
	 * - The variable should be synchronized across all instances of the actor in a networked environment to ensure consistent gameplay.
	 *   The `OnRep_MaxHealth` function will be called when the value of this variable is updated on the server.
	 *   Therefore, any logic related to synchronizing the value on clients should be implemented in that function.
	 *
	 * @see OnRep_MaxHealth
	 *
	 * @warning This variable should not be initialized or modified directly in external code.
	 *          Use appropriate functions or methods within the associated class to manage the health value.
	 */
	UPROPERTY(BlueprintReadWrite)
	float MaxHealth = 0.0f;

	/**
	 * Initializes the health attributes for the server.
	 * This method is called on the server when the health component is being initialized.
	 * It retrieves the health attribute values from the owning character's ability system component and sets the local CurrentHealth and MaxHealth variables accordingly.
	 * It also triggers the OnRep_CurrentHealth and OnRep_MaxHealth methods to ensure proper replication.
	 *
	 * @note This method should be called on the server only.
	 */
	UFUNCTION()
	void InitializeHealthAttributes();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnHealthViewModelInstantiated OnHealthViewModelInstantiated;

	UFUNCTION(BlueprintNativeEvent)
	void HealthViewModelInstantiated(UGASC_UVM_Health* InstantiatedViewModel); 

public:

	/**
	 * CharacterHealthViewModelContextClass is a property that holds the subclass of UMVVMViewModelBase
	 * used to represent the character's health in the health view model.
	 *
	 * This property is editable in the editor and can be accessed by blueprint scripts.
	 *
	 * @see UGASC_HealthComponent::RegisterHealthComponent
	 *
	 * @category View Models
	 * @subcategory Health
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Component|View Model")
	TSubclassOf<UMVVMViewModelBase> CharacterHealthViewModelContextClass;

	/**
	 * CharacterHealthContextName is a variable that holds the name of the context used to represent the character's health in the health view model.
	 *
	 * This variable is editable in the editor and can be accessed by blueprint scripts.
	 *
	 * @see UGASC_HealthComponent::RegisterToHealthViewModel_Client
	 * @see UGASC_HealthComponent::RegisterToHealthViewModel
	 *
	 * @category View Models
	 * @subcategory Health
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Component|View Model")
	FName CharacterHealthContextName;

	/**
	 * The HealthAttributeSet variable is a subclass of UGASCourseHealthAttributeSet.
	 * This variable is exposed to the editor and can be accessed by blueprint scripts.
	 * It represents the attribute set for the health component.
	 *
	 * @see UGASC_HealthComponent
	 * @see UGASC_HealthComponent::OnHealthViewModelRegistered_Implementation
	 * @see UGASC_HealthComponent::MonitorHealthChanges_Server
	 *
	 * @category Attribute
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Component|Gameplay AttributeSet")
	TSubclassOf<UGASCourseHealthAttributeSet> HealthAttributeSet;

	/**
	 * @brief The HealthViewModel variable represents the health view model for an actor.
	 *
	 * This variable is of type UGASC_UVM_Health and is marked as BlueprintReadOnly, which means it can only be read from Blueprint.
	 *
	 * The HealthViewModel is responsible for managing the current and maximum health values of the actor.
	 * It provides functionality for updating and retrieving the health values.
	 *
	 * Example usage:
	 * @code
	 * if (HealthViewModel != nullptr) {
	 *     float currentHealth = HealthViewModel->GetCurrentHealth();
	 *     float maxHealth = HealthViewModel->GetMaxHealth();
	 *     // Perform necessary operations with currentHealth and maxHealth
	 * }
	 * @endcode
	 *
	 * @see UGASC_UVM_Health
	 */
	UPROPERTY(BlueprintReadOnly)
	UGASC_UVM_Health* HealthViewModel;
};
