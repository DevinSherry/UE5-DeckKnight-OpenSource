// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "NiagaraSystemWidget.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineTypes.h"
#include "GASC_UI_Healing.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FHealingReceivedSignature, const FDamageModificationContext&);

/**
 *
 */
UCLASS()
class GASCOURSE_API UGASC_UI_Healing : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Healing, meta=(BindWidget))
	class UCanvasPanel* HealingContainer;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Healing, meta=(BindWidget))
	UNiagaraSystemWidget* HealingNiagaraSystem;
	
	UPROPERTY(BlueprintreadOnly, EditAnywhere, Category=Healing)
	FName NiagaraSpawnCount = "Spawn Count";
	
	UPROPERTY(BlueprintreadOnly, EditAnywhere, Category=Healing)
	float MaxNumberofNiagaraSpawn = 20.0f;
	
	UFUNCTION()
	void OnHealingReceived_Event(const FDamageModificationContext& HealingContext);
};
