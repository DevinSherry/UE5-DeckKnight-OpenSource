// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUD/Healing/GASC_UI_Healing.h"
#include "NiagaraUIComponent.h"
#include "Game/Systems/Damage/Statics/GASC_ResourcePipelineStatics.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Kismet/KismetMathLibrary.h"

void UGASC_UI_Healing::NativeConstruct()
{
	Super::NativeConstruct();

	AActor* OwningActor = GetOwningPlayerPawn();
	if (!OwningActor)
	{
		return;
	}

	if (UWorld* World = OwningActor->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			// Old Damage Pipeline Implementation
			// Bind native received event
			//FOnHealingReceivedNative NativeDelegate;
			//NativeDelegate.BindUObject(this, &UGASC_UI_Healing::OnHealingReceived_Event);
			//Subsys->RegisterNativeHealingReceivedListener(this, MoveTemp(NativeDelegate));

			// Unified attribute-keyed registration: listen for CurrentHealth "Received" modifications.
			FOnResourceModifiedNative NativeDelegate;
			NativeDelegate.BindUObject(this, &UGASC_UI_Healing::OnHealingReceived_Event);
			Subsys->RegisterResourceListener(this, UGASCourseHealthAttributeSet::GetCurrentHealthAttribute(),
				EResourceModificationEventDirection::Received, MoveTemp(NativeDelegate));
		}
	}
}

void UGASC_UI_Healing::NativeDestruct()
{
	Super::NativeDestruct();
	
	if (AActor* OwningActor = GetOwningPlayerPawn())
	{
		if (UWorld* World = OwningActor->GetWorld())
		{
			if (UGASC_ResourcePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
			{
				// Old Damage Pipeline Implementation
				//Subsys->UnregisterNativeHealingListener(this);

				// Unified: clear this widget's attribute-keyed registrations.
				Subsys->UnregisterAllForListener(this);
			}
		}
	}

	// Cleanup Niagara
	if (HealingNiagaraSystem)
	{
		HealingNiagaraSystem->DeactivateSystem();
	}
}

void UGASC_UI_Healing::OnHealingReceived_Event(const FResourceModificationContext& HealingContext)
{
	// CurrentHealth is shared by damage & healing; only react to healing modifications here.
	if (HealingContext.DamageType != DamageType_Healing)
	{
		return;
	}

	// Copy context for safety (in case struct is reused)
	FResourceModificationContext CapturedContext = HealingContext;

	AsyncTask(ENamedThreads::GameThread, [this, CapturedContext]()
	{
		// Widget may be gone already
		if (!IsValid(this))
		{
			return;
		}

		// Niagara widget may have been removed or GC’d
		if (!HealingNiagaraSystem)
		{
			return;
		}

		HealingNiagaraSystem->ActivateSystem(true);
			
			if (AGASCourseCharacter* PlayerCharacter = Cast<AGASCourseCharacter>(GetOwningPlayerPawn()))
			{
				float HealingDelta = CapturedContext.DeltaValue;

				// 2) Map health delta → niagara spawn count
				float MappedValue = FMath::GetMappedRangeValueClamped(
					FVector2D(1.0f, PlayerCharacter->GetMaxHealth()),   
					FVector2D(1.0f, MaxNumberofNiagaraSpawn),             
					HealingDelta);
				
				if (UNiagaraComponent* NiagaraComp = HealingNiagaraSystem->GetNiagaraComponent())
				{
					NiagaraComp->SetVariableInt(NiagaraSpawnCount, MappedValue);
				}
			}
	});
}
