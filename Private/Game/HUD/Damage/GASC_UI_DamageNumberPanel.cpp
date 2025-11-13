// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUD/Damage/GASC_UI_DamageNumberPanel.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UGASC_UI_DamageNumberPanel::NativeConstruct()
{
	Super::NativeConstruct();
	if (AGASCoursePlayerController* PC = Cast<AGASCoursePlayerController>(GetOwningPlayer()))
	{
		PC->OnDamageDealtDelegate.AddDynamic(this, &UGASC_UI_DamageNumberPanel::OnDamageDealt);
	}

	SCOPED_NAMED_EVENT(DamageNumberConstruct, FColor::Red);
	const UGASC_AbilitySystemSettings* Settings = UGASC_AbilitySystemSettings::Get();
	
	TSoftObjectPtr<UGASCourseDamageTypeUIData> AssetRef = Settings->DamageTypeUIData;

	if (!AssetRef.IsValid() && AssetRef.IsNull() == false)
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(AssetRef.ToSoftObjectPath(),
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
		// Already loaded
		DamageTypeUIData = AssetRef.Get();
		GenerateDamageNumberPool();
	}

	// Cache expensive lookups
	OwningPlayerController = GetOwningPlayer();
	if (!OwningPlayerController)
	{
		OwningPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
}

void UGASC_UI_DamageNumberPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);


    if (DamageNumbers.IsEmpty())
    {
        return;
    }
    
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
    const FGeometry& CachedGeometry = GetCachedGeometry();

    TArray<TWeakObjectPtr<UGASC_UI_DamageNumber>> KeysToRemove;

    for (auto& Pair : DamageNumbers)
    {
        TWeakObjectPtr<UGASC_UI_DamageNumber> WeakDamageNumber = Pair.Key;
        UGASC_UI_DamageNumber* Current = WeakDamageNumber.Get();

        // Remove invalid widgets or targets
        if (!IsValid(Current) || !IsValid(Current->DamageData.Target))
        {
            KeysToRemove.Add(WeakDamageNumber);
            continue;
        }

        FVector LocalOffset = Pair.Value;
        FVector WorldLocation = Current->DamageData.Target->GetActorTransform().TransformPosition(LocalOffset);

        if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Current))
        {
            FVector2D ScreenPos;
            if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(OwningPlayerController, WorldLocation, ScreenPos, true))
            {
                ScreenPos *= ViewportScale;  // Use cached scale
                ScreenPos -= Current->GetDesiredSize();
                
                FVector2D LocalWidgetPos;
                USlateBlueprintLibrary::ScreenToWidgetLocal(this, CachedGeometry, ScreenPos, LocalWidgetPos, false);
                
                CanvasSlot->SetPosition(LocalWidgetPos);
                Current->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                Current->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // Remove all invalid widgets after iterating
    for (TWeakObjectPtr<UGASC_UI_DamageNumber>& Key : KeysToRemove)
    {
        DamageNumbers.Remove(Key);
    }
}

TOptional<FVector2D> UGASC_UI_DamageNumberPanel::GetDamagePositionOnScreen(
	const FVector& InStoredLocalPosition,
	const FGameplayEventData& StoredPayload,
	const UGASC_UI_DamageNumber& DamageNumber)
{
	if (!IsValid(StoredPayload.Target))
		return TOptional<FVector2D>();

	FVector WorldLocation = StoredPayload.Target->GetActorTransform().TransformPosition(InStoredLocalPosition);

	FVector2D ScreenPos;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(OwningPlayerController, WorldLocation, ScreenPos, true))
	{
		ScreenPos *= UWidgetLayoutLibrary::GetViewportScale(this);

		FVector2D DesiredSize = DamageNumber.GetDesiredSize();
		ScreenPos -= DesiredSize;

		FVector2D LocalWidgetPos;
		USlateBlueprintLibrary::ScreenToWidgetLocal(this, GetCachedGeometry(), ScreenPos, LocalWidgetPos, false);
		return LocalWidgetPos;
	}

	return TOptional<FVector2D>();
}

UGASC_UI_DamageNumber* UGASC_UI_DamageNumberPanel::GetPooledDamageNumber()
{
	if (DamageNumberPoolIndex < DamageNumberPool.Num())
	{
		UGASC_UI_DamageNumber* DamageNumber = DamageNumberPool[DamageNumberPoolIndex];
		if (DamageNumber->GetVisibility() == ESlateVisibility::Collapsed)
		{
			DamageNumberPoolIndex++;
			if (!DamageNumber->OnDamageNumberRemovedDelegate.IsAlreadyBound(this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved))
			{
				DamageNumber->OnDamageNumberRemovedDelegate.AddDynamic(this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);
			}
			return DamageNumber;
		}

	}

	if (DamageNumberPool.Num() < PoolSize)
	{
		if (IsValid(DamageNumberClass))
		{
			UGASC_UI_DamageNumber* NewDamageNumber = CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);
			NewDamageNumber->SetVisibility(ESlateVisibility::Collapsed);
			DamageNumberPanel->AddChild(NewDamageNumber);
			DamageNumberPool.Add(NewDamageNumber);
			DamageNumberPoolIndex++;
			if (!NewDamageNumber->OnDamageNumberRemovedDelegate.IsAlreadyBound(this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved))
			{
				NewDamageNumber->OnDamageNumberRemovedDelegate.AddDynamic(this, &UGASC_UI_DamageNumberPanel::OnDamageNumberRemoved);
			}
		
			return NewDamageNumber;
		}
	}
	
	return nullptr;
}

void UGASC_UI_DamageNumberPanel::ReturnToDamageNumberPool(UGASC_UI_DamageNumber* DamageNumber)
{
	if (!DamageNumber)
	{
		return;
	}

	DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
	DamageNumber->OnDamageNumberRemovedDelegate.RemoveAll(this);
	DamageNumber->DamageData = FGameplayEventData();
	DamageNumbers.Remove(DamageNumber);
    
	// Move returned widget to front of available pool
	int32 Index = DamageNumberPool.Find(DamageNumber);
	if (Index != INDEX_NONE && Index >= DamageNumberPoolIndex)
	{
		DamageNumberPool.Swap(Index, DamageNumberPoolIndex - 1);
		DamageNumberPoolIndex--;
	}
}

void UGASC_UI_DamageNumberPanel::OnDamageDealt(const FGameplayEventData& DamagePayloadData)
{
	// Capture necessary data by value to avoid race conditions
	FGameplayEventData CapturedData = DamagePayloadData;
	bool bIsCritical = CapturedData.InstigatorTags.HasTagExact(DamageType_Critical);
	
	AsyncTask(ENamedThreads::GameThread, [this, CapturedData, bIsCritical]()
	{
		// Check if widget is still valid
		if (!IsValid(this))
		{
			return;
		}
		
		DamageData = CapturedData;
		AddHitDamageText();
		
		if (bIsCritical)
		{
			bIsCriticalDamage = true;
			AddCriticalHitDamageText();
		}
	});
}

void UGASC_UI_DamageNumberPanel::AddCriticalHitDamageText_Implementation()
{
	if (!DamageNumberClass || !DamageNumberClass->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumberClass is invalid, returning"));
		return;
	}
	UGASC_UI_DamageNumber* DamageNumber = GetPooledDamageNumber();
	if (!DamageNumber) return;

	if (!DamageData.Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageData.Target is null"));
		return;
	}

	DamageNumber->bIsCriticalHit = true;
	DamageNumber->DamageData = DamageData;
	
	if (DamageNumber->DamageText)
	{
		DamageNumber->SetCriticalHitText();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageText is null"));
	}

	DamageNumber->SetVisibility(ESlateVisibility::Visible);
	DamageNumber->ForceLayoutPrepass();

	FVector LocalOffset = GetDamageNumberPosition(DamageData, *DamageNumber);
	DamageNumbers.Add(DamageNumber, LocalOffset);
}

void UGASC_UI_DamageNumberPanel::AddHitDamageText_Implementation()
{
	if (!DamageNumberClass || !DamageNumberClass->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumberClass is invalid, returning"));
		return;
	}
	UGASC_UI_DamageNumber* DamageNumber = GetPooledDamageNumber();
	if (!DamageNumber) return;

	if (!DamageData.Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageData.Target is null"));
		return;
	}

	DamageNumber->bIsCriticalHit = false;
	DamageNumber->DamageData = DamageData;

	if (DamageNumber->DamageText)
	{
		DamageNumber->SetDamageTextValue();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageText is null"));
	}

	DamageNumber->SetVisibility(ESlateVisibility::Visible);
	DamageNumber->ForceLayoutPrepass();

	FVector LocalOffset = GetDamageNumberPosition(DamageData, *DamageNumber);
	DamageNumbers.Add(DamageNumber, LocalOffset);
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
			auto* NewDamageNumber = CreateWidget<UGASC_UI_DamageNumber>(GetWorld(), DamageNumberClass);
			NewDamageNumber->SetVisibility(ESlateVisibility::Collapsed);
			NewDamageNumber->DamageTypeUIData = DamageTypeUIData;
			DamageNumberPanel->AddChild(NewDamageNumber);
			DamageNumberPool.Add(NewDamageNumber);
		}

		if (DamageNumberPool.Num() < PoolSize)
		{
			GenerateDamageNumberPool(); // schedule next
		}
	});

	DamageNumberPanel->InvalidateLayoutAndVolatility();

}

FVector UGASC_UI_DamageNumberPanel::GetDamageNumberPosition(const FGameplayEventData& InPayload, const UGASC_UI_DamageNumber& DamageNumber) const
{
	const AActor* Target = InPayload.Target;
	if (!Target) return FVector::ZeroVector;

	FVector WorldPosition = FVector::ZeroVector;

	FGameplayAbilityTargetDataHandle InTargetData = InPayload.TargetData;
	if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(InTargetData, 0))
	{
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(InTargetData, 0);
		WorldPosition = HitResult.ImpactPoint;
	}
	else
	{
		WorldPosition = Target->GetActorLocation();
	}

	// Convert world position to local relative to the actor
	FVector LocalOffset = Target->GetActorTransform().InverseTransformPosition(WorldPosition);
	// Optional: slight vertical boost for critical hits
	if (DamageNumber.bIsCriticalHit)
	{
		LocalOffset.Z = 130.f; // pushes critical hits slightly above the impact
		LocalOffset.X = 0.f;
		LocalOffset.Y = 0.f;
		return LocalOffset;
	}

	// Randomized XY offsets
	float MaxOffsetXY = 20.f; // critical hits spread more
	float RandomX = FMath::FRandRange(-MaxOffsetXY, MaxOffsetXY);
	float RandomY = FMath::FRandRange(-MaxOffsetXY, MaxOffsetXY);

	LocalOffset.X += RandomX;
	LocalOffset.Y += RandomY;

	return LocalOffset;
}
