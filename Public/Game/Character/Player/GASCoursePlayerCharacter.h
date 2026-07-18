// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Input/GASCourseInputConfig.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Tasks/Task.h"
#include "InputAction.h"
#include "Game/Data/Camera/GASCoursePlayerCameraSettings.h"
#include "Game/Input/GASC_InputAbilityChordedData.h"
#include "GASCoursePlayerCharacter.generated.h"

class UCameraComponent;
class UGASC_InputBufferComponent;
class USpringArmComponent;
/**
 * The AGASCoursePlayerCharacter class represents a player character
 * implementation for a game, utilizing the Gameplay Ability System (GAS).
 * This class serves as the core character that interacts with abilities,
 * attributes, and the game's interaction logic.
 */
UCLASS()
class GASCOURSE_API AGASCoursePlayerCharacter : public AGASCourseCharacter
{
	GENERATED_BODY()

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContextKBM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContextGamepad;
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASCourse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASC_InputBufferComponent> InputBufferComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASCoursePlayerCameraSettings> CameraSettingsData;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UGASCourseInputConfig> InputConfig;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UGASC_InputAbilityChordedData> AbilityChordData;

public:

	AGASCoursePlayerCharacter(const FObjectInitializer& ObjectInitializer);

	/** Returns CameraBoom subobject **/
	FORCEINLINE TObjectPtr<USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE TObjectPtr<UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	/** Returns InputBufferComponent subobject **/
	FORCEINLINE TObjectPtr<UGASC_InputBufferComponent> GetInputBufferComponent() const { return InputBufferComponent; }

	UFUNCTION(BlueprintPure, Category="GASCourse|Input")
	UGASCourseInputConfig* GetInputConfig() const
	{
		return InputConfig;
	}
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Animation")
	TSubclassOf<UAnimInstance> UnArmedAnimLayer;

	void UpdateCharacterAnimLayer(TSubclassOf<UAnimInstance> NewAnimLayer) const;

	/**
	 * Binds a set of ability input actions to a chorded input configuration.
	 *
	 * @param inputMappingName The name of the input mapping to apply.
	 * @param abilityBindings A collection of ability bindings to be associated with the input mapping.
	 * @param overrideExisting Indicates whether existing input mappings should be overridden.
	 */
	UFUNCTION()
	void ApplyAbilityChordedInputMapping(UGASCourseEnhancedInputComponent* EnhancedInputComponent);

	/**
	 * Removes a chorded input mapping for an ability, effectively disabling the linked ability input action.
	 *
	 * This function is used to unbind and remove input mappings associated with abilities
	 * that require specific chorded inputs. Removing such mappings ensures the input
	 * combination no longer triggers the ability.
	 *
	 * Typically utilized when deactivating or unbinding an ability from the input system.
	 */
	UFUNCTION()
	void RemoveAbilityChordedInputMapping();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Weapon")
	UMeshComponent* ActiveWeaponMeshComponent() const;
	
	UFUNCTION(BlueprintPure)
	FVector2D GetMovementInputVector() const
	{
		return MovementInputVector;
	}

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//Add GASCourseAbilitySystemComponent on PossessedBy
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1, bool bForce = false) override;
	
	virtual bool SimulateInputActionFromBuffer(FGameplayTag InputTag) override;

protected:
	void BindASCInput();

	virtual void Move(const FInputActionValue& Value) override;
	virtual void Look(const FInputActionValue& Value) override;

	
	UPROPERTY()
	FVector2D MovementInputVector;

	void Input_CameraZoom(const FInputActionInstance& InputActionInstance);
	void Input_RotateCameraAxis(const FInputActionValue& Value);
	
	void InitializeCamera();

	
private:
	
	bool bAbilityChordedBound = false;
};
