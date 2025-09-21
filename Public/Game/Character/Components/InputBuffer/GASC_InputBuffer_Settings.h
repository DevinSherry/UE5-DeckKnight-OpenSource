// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InputAction.h"
#include "Engine/DeveloperSettings.h"
#include "GASC_InputBuffer_Settings.generated.h"

/**
 * UGASC_InputBuffer_Settings
 *
 * Class designed to manage configuration settings for input buffering in the GAS Course project.
 * The class inherits from UDeveloperSettings, allowing these settings to be exposed and configurable
 * through the Unreal Engine project settings under the appropriate section.
 *
 * Configuration parameters:
 * - Default configuration file: Game.ini.
 * - Display name in project settings: "GASCourse Input Buffer Settings".
 *
 * Properties:
 * - BufferedInputActions: A list of input actions which are treated as buffered actions. These are soft object references
 *   that help to ensure non-intrusive loading of assets.
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName="GASCourse Input Buffer Settings"))
class GASCOURSE_API UGASC_InputBuffer_Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * BufferedInputActions
	 *
	 * Represents a collection of input actions that are designated as buffered actions
	 * for the GAS Course Input Buffer system. These input actions are stored as soft
	 * object references to manage asset loading efficiently without causing runtime blockage.
	 *
	 * Configuration properties:
	 * - Category: Input Buffer Actions.
	 * - Directory Path: /Game/GASCourse/Game/Character/Input/Actions/
	 * - Allows configuration in the Game.ini file.
	 *
	 * Accessibility:
	 * - Configurable in both C++ and Blueprints.
	 * - Editable across various project editing contexts.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input Buffer Actions", meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	TArray<TSoftObjectPtr<UInputAction>> BufferedInputActions;

	/**
	 * MovementInputAction
	 *
	 * Represents a specific input action associated with character movement in the GAS Course Input Buffer system.
	 * This variable is stored as a soft object pointer to manage asset loading efficiently and prevent runtime delays.
	 *
	 * Configuration properties:
	 * - Category: Input Buffer Actions.
	 * - Directory Path: /Game/GASCourse/Game/Character/Input/Actions/.
	 * - Defined in the configuration file: Game.ini.
	 *
	 * Accessibility:
	 * - Configurable in both C++ and Blueprints.
	 * - Editable in applicable contexts through the Unreal Engine editor.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input Buffer Actions", meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	TSoftObjectPtr<UInputAction> MovementInputAction;
	
	UGASC_InputBuffer_Settings();
	
};
