// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectComponent.h"
#include "GameplayTagContainer.h"
#include "SendGameplayEventEffectComponent.generated.h"

/**
 * @class USendGameplayEventEffectComponent
 * @brief A component that triggers gameplay events when certain effects occur.
 *
 * The USendGameplayEventEffectComponent is designed to facilitate the
 * triggering of gameplay events in response to specific game effects.
 * This is particularly useful for implementing reactive gameplay mechanics,
 * such as notifying the game system of status changes, item effects, or
 * ability activations.
 *
 * This component can be attached to relevant actor instances to enable event-based
 * communication between game systems.
 *
 * Key functionality includes:
 * - Listening for specific game states or changes.
 * - Dispatching gameplay events when the relevant conditions are met.
 * - Integration with the broader gameplay framework for event management.
 */
UCLASS()
class GASCOURSE_API USendGameplayEventEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()
	
public:
	
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const;

	/**
	 * @brief Specifies the gameplay tag associated with the target event to trigger.
	 *
	 * The `TargetEventTag` serves as an identifier for a specific gameplay event
	 * that this component is designed to handle or activate. It allows the system
	 * to link actions, effects, or communications to particular in-game occurrences.
	 *
	 * Key attributes:
	 * - **Editable Defaults Only**: The value can only be set in the default properties
	 *   and not during runtime.
	 * - **Category**: Organized under "Gameplay Effect Component" for clarity in editor workflows.
	 * - **Metadata**: Restricted to tags within the "Event.Gameplay" category, ensuring its
	 *   usage aligns with predefined event tagging conventions.
	 *
	 * This tag is critical for enabling targeted and systematic event-driven gameplay logic.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effect Component", meta=(Categories="Event.Gameplay"))
	FGameplayTag TargetEventTag;

	/**
	 * @brief Specifies the gameplay tag associated with the instigator event to trigger.
	 *
	 * The `InstigatorEventTag` identifies a specific gameplay event triggered by the instigating
	 * actor or component. It aids in associating actions, reactions, or effects with the entity
	 * responsible for initiating the event, enabling systemic and organized gameplay logic.
	 *
	 * Key attributes:
	 * - **Editable Defaults Only**: The tag can only be configured in the default properties and
	 *   is not modifiable at runtime.
	 * - **Category**: Categorized under "Gameplay Effect Component" to streamline organization and
	 *   usage within the editor.
	 * - **Metadata**: Limited to tags within the "Event.Gameplay" category, ensuring compliance
	 *   with established gameplay event tagging conventions.
	 *
	 * This tag is instrumental in linking gameplay effects with the instigator's context,
	 * facilitating detailed and structured event-driven mechanics.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effect Component", meta=(Categories="Event.Gameplay"))
	FGameplayTag InstigatorEventTag;
};
