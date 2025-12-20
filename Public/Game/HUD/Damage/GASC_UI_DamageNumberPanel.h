#pragma once

#include "Blueprint/UserWidget.h"
#include "GASC_UI_DamageNumber.h"
#include "GASC_UI_DamageNumberPanel.generated.h"

/**
 * Displays floating damage/healing numbers using a pooled widget system.
 * World-space positions are tracked and converted to widget space each tick
 * in a DPI-safe way.
 */
UCLASS()
class GASCOURSE_API UGASC_UI_DamageNumberPanel : public UUserWidget
{
	GENERATED_BODY()

public:

	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	//~End interface

	/** Canvas panel that owns all damage number widgets */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage", meta = (BindWidget))
	class UCanvasPanel* DamageNumberPanel;

	/** Maximum number of pooled widgets */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
	int32 PoolSize = 100;

	/** Damage number widget class */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
	TSubclassOf<UGASC_UI_DamageNumber> DamageNumberClass;

	/** Called by the damage pipeline subsystem */
	UFUNCTION()
	void OnDamageApplied_Event(const FDamageModificationContext& DamageContext);

	/** Called by the damage pipeline subsystem */
	UFUNCTION()
	void OnHealingReceived_Event(const FDamageModificationContext& HealingContext);

protected:

	/* ============================
	 *  Pooling
	 * ============================ */

	/** All widgets ever created */
	UPROPERTY()
	TArray<TObjectPtr<UGASC_UI_DamageNumber>> DamageNumberPool;

	/** Widgets currently free and ready for reuse */
	UPROPERTY()
	TArray<TObjectPtr<UGASC_UI_DamageNumber>> FreeDamageNumbers;

	/** Acquire a widget from the pool */
	UGASC_UI_DamageNumber* GetPooledDamageNumber();

	/** Return a widget back to the pool */
	UFUNCTION()
	void ReturnToDamageNumberPool(UGASC_UI_DamageNumber* DamageNumber);

	/** Pre-allocates the pool */
	void GenerateDamageNumberPool();

	/* ============================
	 *  Damage Display
	 * ============================ */

	/** Adds a normal hit damage/heal number using the provided context */
	void AddHitDamageTextFromContext(const FDamageModificationContext& Context);

	/** Adds a critical hit damage/heal number using the provided context */
	void AddCriticalHitDamageTextFromContext(const FDamageModificationContext& Context);

	/** Called by damage number widgets when their animation finishes */
	UFUNCTION()
	void OnDamageNumberRemoved(UGASC_UI_DamageNumber* DamageNumber);

	/* ============================
	 *  World → Screen Tracking
	 * ============================ */

	/** World-space positions for active damage numbers */
	UPROPERTY()
	TMap<TObjectPtr<UGASC_UI_DamageNumber>, FVector> DamageNumberWorldPositions;

	/** Computes initial world position for a damage number */
	FVector GetDamageNumberWorldPosition(
		const FHitContext& HitContext,
		const UGASC_UI_DamageNumber& DamageNumber) const;

	/** Converts world location to widget-local space (DPI safe) */
	TOptional<FVector2D> GetDamagePositionOnScreen(
		const FVector& WorldLocation,
		const AActor* Target,
		const UGASC_UI_DamageNumber& DamageNumber);

	/* ============================
	 *  Cached Data
	 * ============================ */

	UPROPERTY()
	class UGASCourseDamageTypeUIData* DamageTypeUIData;

	UPROPERTY()
	APlayerController* OwningPlayerController;
};
