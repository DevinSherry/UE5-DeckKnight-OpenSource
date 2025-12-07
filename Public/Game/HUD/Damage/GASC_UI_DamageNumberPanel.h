#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GASC_UI_DamageNumber.h"
#include "GASC_UI_DamageNumberPanel.generated.h"

/**
 * @class UGASC_UI_DamageNumberPanel
 * Displays floating damage numbers using world-space tracking.
 * Now uses world positions instead of local offsets, ensuring proper positioning
 * across all screen resolutions and DPI modes.
 */
UCLASS()
class GASCOURSE_API UGASC_UI_DamageNumberPanel : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	/** Panel into which damage number widgets are added */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Damage, meta=(BindWidget))
	class UCanvasPanel* DamageNumberPanel;

	/** Handles incoming damage events */
	UFUNCTION()
	void OnDamageApplied_Event(const FDamageModificationContext& DamageContext);
	
	UFUNCTION()
	void OnHealingReceived_Event(const FDamageModificationContext& HealingContext);

	UFUNCTION(BlueprintNativeEvent)
	void AddCriticalHitDamageText();

	UFUNCTION(BlueprintNativeEvent)
	void AddHitDamageText();

	UFUNCTION()
	void OnDamageNumberRemoved(UGASC_UI_DamageNumber* DamageNumber);

	UFUNCTION()
	void GenerateDamageNumberPool();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Damage)
	TSubclassOf<UGASC_UI_DamageNumber> DamageNumberClass;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Damage)
	FVector2D Position;

private:

	/** Indicates last damage event was critical */
	bool bIsCriticalDamage = false;

	FDamageModificationContext DamageModificationContext;

	/** WORLD-SPACE positions where damage numbers should appear */
	UPROPERTY()
	TMap<TWeakObjectPtr<UGASC_UI_DamageNumber>, FVector> DamageNumberWorldPositions;

	UPROPERTY()
	TArray<UGASC_UI_DamageNumber*> DamageNumberPool;

	UPROPERTY()
	UGASCourseDamageTypeUIData* DamageTypeUIData;

	UPROPERTY()
	APlayerController* OwningPlayerController;

	UPROPERTY(EditAnywhere, Category=DamageUI)
	int32 PoolSize = 100;

	int32 DamageNumberPoolIndex = 0;

	UGASC_UI_DamageNumber* GetPooledDamageNumber();

	void ReturnToDamageNumberPool(UGASC_UI_DamageNumber* DamageNumber);

	/** NEW — Computes the world position for a given damage number */
	FVector GetDamageNumberWorldPosition(const FHitContext& HitContext, const UGASC_UI_DamageNumber& DamageNumber) const;

	/** NEW — Converts world → screen → widget space (DPI-safe) */
	TOptional<FVector2D> GetDamagePositionOnScreen(const FVector& WorldLocation, const AActor* Target, const UGASC_UI_DamageNumber& DamageNumber);
};
