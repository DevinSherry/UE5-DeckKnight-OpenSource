// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "DeckManagerGameInstanceSubsystem.generated.h"

/**
 * UDeckManagerGameInstanceSubsystem is a subsystem designed to manage deck-related functionality
 * within the game instance. It inherits from UGameInstanceSubsystem, leveraging its lifecycle
 * and attachment to the game instance for centralized deck management.
 *
 * This class can include functionality for creating, managing, and retrieving decks used in gameplay,
 * making it a core component of the game's card or deck-based systems.
 *
 * Features and responsibilities of this subsystem include:
 * - Managing the lifecycle and state of player or system-generated decks across the game session.
 * - Providing centralized access to deck-related operations to other subsystems or game layers.
 * - Efficiently persisting or resetting deck data within the scope initialized by the game instance.
 *
 * Can be extended to provide additional utilities specific to complex deck systems or custom game requirements.
 */
UCLASS()
class GASCOURSE_API UDeckManagerGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	
	
	
};
