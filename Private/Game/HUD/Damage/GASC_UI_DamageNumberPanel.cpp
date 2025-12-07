#include "Game/HUD/Damage/GASC_UI_DamageNumberPanel.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/Systems/Damage/Data/GASCourseDamageTypeUIData.h"

void UGASC_UI_DamageNumberPanel::NativeConstruct()
{
	Super::NativeConstruct();

	AActor* OwningActor = GetOwningPlayerPawn();
	if (!OwningActor)
		return;

	if (UWorld* World = OwningActor->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			FOnDamageAppliedNative NativeDelegate;
			NativeDelegate.BindUObject(this, &UGASC_UI_DamageNumberPanel::OnDamageApplied_Event);
			Subsys->RegisterNativeDamageAppliedListener(this, MoveTemp(NativeDelegate));
			
			FOnHealingReceivedNative HealingDelegate;
			HealingDelegate.BindUObject(this, &UGASC_UI_DamageNumberPanel::OnHealingReceived_Event);
			Subsys->RegisterNativeHealingReceivedListener(this, MoveTemp(HealingDelegate));
		}
	}

	// Load DamageTypeUIData
	const UGASC_AbilitySystemSettings* Settings = UGASC_AbilitySystemSettings::Get();
	TSoftObjectPtr<UGASCourseDamageTypeUIData> AssetRef = Settings->DamageTypeUIData;

	if (!AssetRef.IsValid() && !AssetRef.IsNull())
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(
			AssetRef.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda([AssetRef, this]()
			{
				if (UGASCourseDamageTypeUIData* LoadedAsset = AssetRef.Get())
				{
					DamageTypeUIData = LoadedAsset;
					GenerateDamageNumberPool();
				}
			}));
	}
	else if (AssetRef.IsValid())
	{
		DamageTypeUIData = AssetRef.Get();
		GenerateDamageNumberPool();
	}

	OwningPlayerController = GetOwningPlayer();
	if (!OwningPlayerController)
		OwningPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

void UGASC_UI_DamageNumberPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DamageNumberWorldPositions.IsEmpty())
		return;

	const FGeometry& CachedGeometry = GetCachedGeometry();
	TArray<TWeakObjectPtr<UGASC_UI_DamageNumber>> KeysToRemove;

	for (auto& Pair : DamageNumberWorldPositions)
	{
		TWeakObjectPtr<UGASC_UI_DamageNumber> WeakDamageNumber = Pair.Key;
		UGASC_UI_DamageNumber* Current = WeakDamageNumber.Get();
		if (!IsValid(Current) ||
			!IsValid(Current->DamageModContext.HitContext.HitTarget.Get()))
		{
			KeysToRemove.Add(WeakDamageNumber);
			continue;
		}

		FVector WorldLocation = Pair.Value;

		TOptional<FVector2D> MaybePos = GetDamagePositionOnScreen(WorldLocation, Current->DamageModContext.HitContext.HitTarget.Get(), *Current);
		if (!MaybePos.IsSet())
		{
			Current->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Current))
		{
			CanvasSlot->SetPosition(MaybePos.GetValue());
			Current->SetVisibility(ESlateVisibility::Visible);
		}
	}

	for (auto& Key : KeysToRemove)
	{
		DamageNumberWorldPositions.Remove(Key);
	}
}

void UGASC_UI_DamageNumberPanel::NativeDestruct()
{
	Super::NativeDestruct();

	if (AActor* OwningActor = GetOwningPlayerPawn())
	{
		if (UWorld* World = OwningActor->GetWorld())
		{
			if (UGASC_DamagePipelineSubsystem* Subsys = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
			{
				Subsys->UnregisterNativeDamageListener(this);
			}
		}
	}
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

	UE_LOG(LogTemp, Warning, TEXT("Failed damage number position calculation, returning 0,0"));
	return OutDamageScreenPosition;
}

UGASC_UI_DamageNumber* UGASC_UI_DamageNumberPanel::GetPooledDamageNumber()
{
	if (DamageNumberPoolIndex < DamageNumberPool.Num())
	{
		UGASC_UI_DamageNumber* DamageNumber = DamageNumberPool[DamageNumberPoolIndex];
		if (DamageNumber && DamageNumber->GetVisibility() == ESlateVisibility::Collapsed)
		{
			DamageNumberPoolIndex++;
			DamageNumber->DamageTypeUIData = DamageTypeUIData;

			if (!DamageNumber->OnDamageNumberRemovedDelegate.IsAlreadyBound(
				this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved))
			{
				DamageNumber->OnDamageNumberRemovedDelegate.AddDynamic(
					this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);
			}
			return DamageNumber;
		}
	}

	// Create new widget if needed
	if (DamageNumberPool.Num() < PoolSize && IsValid(DamageNumberClass))
	{
		UGASC_UI_DamageNumber* NewDamageNumber =
			CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);

		NewDamageNumber->SetVisibility(ESlateVisibility::Collapsed);
		NewDamageNumber->DamageTypeUIData = DamageTypeUIData;

		DamageNumberPanel->AddChild(NewDamageNumber);
		DamageNumberPool.Add(NewDamageNumber);
		DamageNumberPoolIndex++;

		NewDamageNumber->OnDamageNumberRemovedDelegate.AddDynamic(
			this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);

		return NewDamageNumber;
	}

	return nullptr;
}

void UGASC_UI_DamageNumberPanel::ReturnToDamageNumberPool(UGASC_UI_DamageNumber* DamageNumber)
{
	if (!DamageNumber)
		return;

	DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
	DamageNumber->OnDamageNumberRemovedDelegate.RemoveAll(this);
	DamageNumberPool.Remove(DamageNumber);
	DamageNumberWorldPositions.Remove(DamageNumber);
	DamageNumber->DamageModContext = FDamageModificationContext();

	int32 Index = DamageNumberPool.Find(DamageNumber);
	if (Index != INDEX_NONE && Index >= DamageNumberPoolIndex)
	{
		DamageNumberPool.Swap(Index, DamageNumberPoolIndex - 1);
		DamageNumberPoolIndex--;
	}
}

void UGASC_UI_DamageNumberPanel::OnDamageApplied_Event(
	const FDamageModificationContext& DamageContext)
{
	if (!DamageContext.HitContext.HitTarget.IsValid())
		return;

	FDamageModificationContext CapturedContext = DamageContext;

	AsyncTask(ENamedThreads::GameThread, [this, CapturedContext]()
	{
		if (!IsValid(this))
			return;

		if (!CapturedContext.HitContext.HitTarget.IsValid())
			return;

		DamageModificationContext = CapturedContext;

		AddHitDamageText();

		if (CapturedContext.bCriticalModification)
		{
			bIsCriticalDamage = true;
			AddCriticalHitDamageText();
		}
	});
}

void UGASC_UI_DamageNumberPanel::OnHealingReceived_Event(const FDamageModificationContext& HealingContext)
{
	if (!HealingContext.HitContext.HitTarget.IsValid())
		return;

	FDamageModificationContext CapturedContext = HealingContext;

	AsyncTask(ENamedThreads::GameThread, [this, CapturedContext]()
	{
		if (!IsValid(this))
			return;

		if (!CapturedContext.HitContext.HitTarget.IsValid())
			return;

		DamageModificationContext = CapturedContext;

		AddHitDamageText();

		if (CapturedContext.bCriticalModification)
		{
			bIsCriticalDamage = true;
			AddCriticalHitDamageText();
		}
	});
}

void UGASC_UI_DamageNumberPanel::AddCriticalHitDamageText_Implementation()
{
	if (!DamageNumberClass || !DamageNumberClass->IsValidLowLevel())
		return;

	UGASC_UI_DamageNumber* DamageNumber = GetPooledDamageNumber();
	if (!DamageNumber ||
		!DamageModificationContext.HitContext.HitTarget.IsValid())
		return;

	DamageNumber->bIsCriticalHit = true;
	DamageNumber->DamageModContext = DamageModificationContext;

	if (DamageNumber->DamageText)
		DamageNumber->SetCriticalHitText();

	DamageNumber->SetVisibility(ESlateVisibility::Visible);
	DamageNumber->ForceLayoutPrepass();

	FVector WorldPos =
		GetDamageNumberWorldPosition(DamageModificationContext.HitContext, *DamageNumber);

	DamageNumberWorldPositions.Add(DamageNumber, WorldPos);
}

void UGASC_UI_DamageNumberPanel::AddHitDamageText_Implementation()
{
	if (!DamageNumberClass || !DamageNumberClass->IsValidLowLevel())
		return;

	if (FMath::RoundToInt(DamageModificationContext.DeltaValue) == 0)
	{
		return;
	}
	UGASC_UI_DamageNumber* DamageNumber = GetPooledDamageNumber();
	if (!DamageNumber ||
		!DamageModificationContext.HitContext.HitTarget.IsValid())
		return;

	DamageNumber->bIsCriticalHit = false;
	DamageNumber->DamageModContext = DamageModificationContext;

	if (DamageNumber->DamageText)
		DamageNumber->SetDamageTextValue();

	DamageNumber->SetVisibility(ESlateVisibility::Visible);
	DamageNumber->ForceLayoutPrepass();

	FVector WorldPos =
		GetDamageNumberWorldPosition(DamageModificationContext.HitContext, *DamageNumber);

	DamageNumberWorldPositions.Add(DamageNumber, WorldPos);
}

void UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved(UGASC_UI_DamageNumber* DamageNumber)
{
	ReturnToDamageNumberPool(DamageNumber);
}

void UGASC_UI_DamageNumberPanel::GenerateDamageNumberPool()
{
	DamageNumberPool.Reserve(PoolSize);

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		if (DamageNumberPool.Num() < PoolSize && IsValid(DamageNumberClass))
		{
			UGASC_UI_DamageNumber* NewDamageNumber =
				CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);

			NewDamageNumber->SetVisibility(ESlateVisibility::Collapsed);
			NewDamageNumber->DamageTypeUIData = DamageTypeUIData;

			DamageNumberPanel->AddChild(NewDamageNumber);
			DamageNumberPool.Add(NewDamageNumber);
		}

		if (DamageNumberPool.Num() < PoolSize)
		{
			GenerateDamageNumberPool();
		}
	});

	DamageNumberPanel->InvalidateLayoutAndVolatility();
}

FVector UGASC_UI_DamageNumberPanel::GetDamageNumberWorldPosition(
	const FHitContext& HitContext,
	const UGASC_UI_DamageNumber& DamageNumber) const
{
	const AActor* Target = HitContext.HitTarget.Get();
	if (!IsValid(Target))
		return FVector::ZeroVector;

	FHitResult HR = HitContext.HitResult;
	FVector WorldPos = HR.bBlockingHit
		? HR.ImpactPoint
		: (Target->GetActorLocation() + FVector(0, 0, 0.f));
	
	if (DamageNumber.bIsCriticalHit)
	{
		FVector2D DamageNumberUISize = DamageNumber.GetDesiredSize();
		return Target->GetActorLocation() + FVector(DamageNumberUISize.X, DamageNumberUISize.Y, 130.0f);
	}
	
	float Spread = 20.f;
	WorldPos.X += FMath::FRandRange(-Spread, Spread);
	WorldPos.Y += FMath::FRandRange(-Spread, Spread);
	WorldPos.Z += FMath::FRandRange(Spread, Spread*3.0f);

	return WorldPos;
}
