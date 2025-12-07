// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUD/Healing/GASC_UI_Healing.h"
#include "NiagaraUIComponent.h"
#include "Game/Systems/Damage/Statics/GASC_DamagePipelineStatics.h"
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
		if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			// Bind native received event
			FOnHealingReceivedNative NativeDelegate;
			NativeDelegate.BindUObject(this, &UGASC_UI_Healing::OnHealingReceived_Event);

			Subsys->RegisterNativeHealingReceivedListener(this, MoveTemp(NativeDelegate));
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
			if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
			{
				// Remove native listener
				Subsys->UnregisterNativeHealingListener(this);
			}
		}
	}

	// Cleanup Niagara
	if (HealingNiagaraSystem)
	{
		HealingNiagaraSystem->DeactivateSystem();
	}
}

void UGASC_UI_Healing::OnHealingReceived_Event(const FDamageModificationContext& HealingContext)
{
	// Copy context for safety (in case struct is reused)
	FDamageModificationContext CapturedContext = HealingContext;

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
