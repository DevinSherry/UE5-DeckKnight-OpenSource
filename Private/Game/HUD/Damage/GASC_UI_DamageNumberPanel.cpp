#include "Game/HUD/Damage/GASC_UI_DamageNumberPanel.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/Systems/Damage/Data/GASCourseDamageTypeUIData.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Game/DeveloperSettings/UGASC_AbilitySystemSettings.h"

void UGASC_UI_DamageNumberPanel::NativeConstruct()
{
	Super::NativeConstruct();

	OwningPlayerController = GetOwningPlayer();
	if (!OwningPlayerController)
	{
		OwningPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	// Register damage listeners
	if (UWorld* World = GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			FOnDamageAppliedNative DamageDelegate;
			DamageDelegate.BindUObject(this, &UGASC_UI_DamageNumberPanel::OnDamageApplied_Event);
			Subsys->RegisterNativeDamageAppliedListener(this, MoveTemp(DamageDelegate));

			FOnHealingReceivedNative HealingDelegate;
			HealingDelegate.BindUObject(this, &UGASC_UI_DamageNumberPanel::OnHealingReceived_Event);
			Subsys->RegisterNativeHealingReceivedListener(this, MoveTemp(HealingDelegate));
		}
	}

	// Load damage UI data
	const UGASC_AbilitySystemSettings* Settings = UGASC_AbilitySystemSettings::Get();
	if (!Settings)
		return;

	const TSoftObjectPtr<UGASCourseDamageTypeUIData> AssetRef = Settings->DamageTypeUIData;

	if (AssetRef.IsValid())
	{
		DamageTypeUIData = AssetRef.Get();
		GenerateDamageNumberPool();
	}
	else if (!AssetRef.IsNull())
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(
			AssetRef.ToSoftObjectPath(),
			FStreamableDelegate::CreateWeakLambda(this, [this, AssetRef]()
			{
				if (UGASCourseDamageTypeUIData* Loaded = AssetRef.Get())
				{
					DamageTypeUIData = Loaded;
					GenerateDamageNumberPool();
				}
			})
		);
	}
}

void UGASC_UI_DamageNumberPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			Subsys->UnregisterNativeDamageListener(this);
		}
	}

	DamageNumberWorldPositions.Empty();
	FreeDamageNumbers.Empty();
	DamageNumberPool.Empty();

	Super::NativeDestruct();
}

void UGASC_UI_DamageNumberPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DamageNumberWorldPositions.IsEmpty())
		return;

	TArray<UGASC_UI_DamageNumber*> ToRemove;

	for (const auto& Pair : DamageNumberWorldPositions)
	{
		UGASC_UI_DamageNumber* DamageNumber = Pair.Key;
		if (!IsValid(DamageNumber))
		{
			ToRemove.Add(DamageNumber);
			continue;
		}

		const FDamageModificationContext& Context = DamageNumber->DamageModContext;
		const AActor* Target = Context.HitContext.HitTarget.Get();
		if (!IsValid(Target))
		{
			DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
			ToRemove.Add(DamageNumber);
			continue;
		}

		const FVector WorldLocation = Pair.Value;
		TOptional<FVector2D> LocalPos =
			GetDamagePositionOnScreen(WorldLocation, Target, *DamageNumber);

		if (!LocalPos.IsSet())
		{
			DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* DamageNumberSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(DamageNumber))
		{
			DamageNumberSlot->SetPosition(LocalPos.GetValue());
			DamageNumber->SetVisibility(ESlateVisibility::Visible);
		}
	}

	for (UGASC_UI_DamageNumber* W : ToRemove)
	{
		DamageNumberWorldPositions.Remove(W);
	}
}

/* ============================
 *  Pooling
 * ============================ */

void UGASC_UI_DamageNumberPanel::GenerateDamageNumberPool()
{
	if (!IsValid(DamageNumberClass) || !IsValid(DamageNumberPanel))
		return;

	DamageNumberPool.Reserve(PoolSize);
	FreeDamageNumbers.Reserve(PoolSize);

	for (int32 i = DamageNumberPool.Num(); i < PoolSize; ++i)
	{
		UGASC_UI_DamageNumber* NewWidget =
			CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);

		if (!IsValid(NewWidget))
			break;

		NewWidget->SetVisibility(ESlateVisibility::Collapsed);
		NewWidget->DamageTypeUIData = DamageTypeUIData;

		DamageNumberPanel->AddChild(NewWidget);

		NewWidget->OnDamageNumberRemovedDelegate.AddDynamic(
			this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);

		DamageNumberPool.Add(NewWidget);
		FreeDamageNumbers.Add(NewWidget);
	}
}

UGASC_UI_DamageNumber* UGASC_UI_DamageNumberPanel::GetPooledDamageNumber()
{
	// 1) Reuse from free list first
	while (FreeDamageNumbers.Num() > 0)
	{
		UGASC_UI_DamageNumber* W = FreeDamageNumbers.Pop(EAllowShrinking::No);
		if (IsValid(W))
		{
			W->DamageTypeUIData = DamageTypeUIData;
			return W;
		}
	}

	// 2) Pool exhausted → create a new widget (overflow)
	if (!IsValid(DamageNumberClass) || !IsValid(DamageNumberPanel))
		return nullptr;

	UGASC_UI_DamageNumber* NewWidget =
		CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);

	if (!IsValid(NewWidget))
		return nullptr;

	NewWidget->SetVisibility(ESlateVisibility::Collapsed);
	NewWidget->DamageTypeUIData = DamageTypeUIData;

	DamageNumberPanel->AddChild(NewWidget);

	NewWidget->OnDamageNumberRemovedDelegate.AddDynamic(
		this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);

	// Important: track it like any other pooled widget
	DamageNumberPool.Add(NewWidget);

	return NewWidget;
}


void UGASC_UI_DamageNumberPanel::ReturnToDamageNumberPool(UGASC_UI_DamageNumber* DamageNumber)
{
	if (!IsValid(DamageNumber))
		return;

	DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
	DamageNumber->DamageModContext = FDamageModificationContext();

	DamageNumberWorldPositions.Remove(DamageNumber);
	FreeDamageNumbers.Add(DamageNumber);
}

void UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved(UGASC_UI_DamageNumber* DamageNumber)
{
	ReturnToDamageNumberPool(DamageNumber);
}

/* ============================
 *  Damage Events
 * ============================ */

void UGASC_UI_DamageNumberPanel::OnDamageApplied_Event(
	const FDamageModificationContext& DamageContext)
{
	if (!DamageContext.HitContext.HitTarget.IsValid())
		return;

	const FDamageModificationContext Captured = DamageContext;

	AsyncTask(ENamedThreads::GameThread, [this, Captured]()
	{
		if (!IsValid(this))
			return;

		AddHitDamageTextFromContext(Captured);

		if (Captured.bCriticalModification)
		{
			AddCriticalHitDamageTextFromContext(Captured);
		}
	});
}

void UGASC_UI_DamageNumberPanel::OnHealingReceived_Event(
	const FDamageModificationContext& HealingContext)
{
	if (!HealingContext.HitContext.HitTarget.IsValid())
		return;

	const FDamageModificationContext Captured = HealingContext;

	AsyncTask(ENamedThreads::GameThread, [this, Captured]()
	{
		if (!IsValid(this))
			return;

		AddHitDamageTextFromContext(Captured);

		if (Captured.bCriticalModification)
		{
			AddCriticalHitDamageTextFromContext(Captured);
		}
	});
}

/* ============================
 *  Damage Creation
 * ============================ */

void UGASC_UI_DamageNumberPanel::AddHitDamageTextFromContext(
	const FDamageModificationContext& Context)
{
	if (!Context.HitContext.HitTarget.IsValid())
		return;

	//TODO check for resistance as well
	if (FMath::RoundToInt(Context.DeltaValue) == 0 && !Context.bDamageResisted)
		return;

	UGASC_UI_DamageNumber* W = GetPooledDamageNumber();
	if (!IsValid(W))
		return;

	W->bIsCriticalHit = false;
	W->DamageModContext = Context;

	W->SetVisibility(ESlateVisibility::Visible);
	W->ForceLayoutPrepass();

	if (W->DamageText)
	{
		W->SetDamageTextValue();
	}

	const FVector WorldPos =
		GetDamageNumberWorldPosition(Context.HitContext, *W);

	DamageNumberWorldPositions.Add(W, WorldPos);
}

void UGASC_UI_DamageNumberPanel::AddCriticalHitDamageTextFromContext(
	const FDamageModificationContext& Context)
{
	if (!Context.HitContext.HitTarget.IsValid())
		return;

	UGASC_UI_DamageNumber* W = GetPooledDamageNumber();
	if (!IsValid(W))
		return;

	W->bIsCriticalHit = true;
	W->DamageModContext = Context;

	W->SetVisibility(ESlateVisibility::Visible);
	W->ForceLayoutPrepass();

	if (W->DamageText)
	{
		W->SetCriticalHitText();
	}

	const FVector WorldPos =
		GetDamageNumberWorldPosition(Context.HitContext, *W);

	DamageNumberWorldPositions.Add(W, WorldPos);
}

/* ============================
 *  World → Screen
 * ============================ */

FVector UGASC_UI_DamageNumberPanel::GetDamageNumberWorldPosition(
	const FHitContext& HitContext,
	const UGASC_UI_DamageNumber& DamageNumber) const
{
	const AActor* Target = HitContext.HitTarget.Get();
	if (!IsValid(Target))
		return FVector::ZeroVector;

	const FHitResult& HR = HitContext.HitResult;

	FVector WorldPos = HR.bBlockingHit
	? FVector(HR.ImpactPoint)
	: Target->GetActorLocation();

	if (DamageNumber.bIsCriticalHit)
	{
		return Target->GetActorLocation() + FVector(0.f, 0.f, 130.f);
	}

	const float Spread = 20.f;
	WorldPos.X += FMath::FRandRange(-Spread, Spread);
	WorldPos.Y += FMath::FRandRange(-Spread, Spread);
	WorldPos.Z += FMath::FRandRange(Spread, Spread * 3.f);

	return WorldPos;
}

TOptional<FVector2D> UGASC_UI_DamageNumberPanel::GetDamagePositionOnScreen(
	const FVector& WorldLocation,const AActor* Target, const UGASC_UI_DamageNumber& DamageNumber)
{
	FVector StoredTargetLocation = FVector::ZeroVector;
	FVector2D OutDamageScreenPosition = FVector2D::ZeroVector;
	if (!DamageNumber.IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumber is invalid, returning 0,0"));
		return OutDamageScreenPosition;
	}
	if (const AActor* StoredTarget = Target)
	{
		StoredTargetLocation = StoredTarget->GetActorLocation();	
	}
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		float WorldXPosition = StoredTargetLocation.X - WorldLocation.X;
		float WorldYPosition = StoredTargetLocation.Y - WorldLocation.Y;
		FVector WorldPosition = FVector(WorldXPosition, WorldYPosition, 0.0f) + WorldLocation;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldPosition, OutDamageScreenPosition, true))
		{
			const FGeometry CachedWidgetGeometry = GetCachedGeometry();
			FVector2D CachedDamageNumberDesiredSize = DamageNumber.GetDesiredSize();
			if (DamageNumber.bIsCriticalHit)
			{
				CachedDamageNumberDesiredSize = CachedDamageNumberDesiredSize / 2.0f;
			}

			OutDamageScreenPosition -= CachedDamageNumberDesiredSize;
			OutDamageScreenPosition *= UWidgetLayoutLibrary::GetViewportScale(this);
			FVector2D FinalDamageOutPosition = FVector2D::ZeroVector;
			USlateBlueprintLibrary::ScreenToWidgetLocal(this, CachedWidgetGeometry, OutDamageScreenPosition, FinalDamageOutPosition, false);
			return FinalDamageOutPosition;
		}
	}
	
	return OutDamageScreenPosition;
}
