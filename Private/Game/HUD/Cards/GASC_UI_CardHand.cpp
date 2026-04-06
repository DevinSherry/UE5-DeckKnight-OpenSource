// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUD/Cards/GASC_UI_CardHand.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Game/HUD/Cards/GASC_UI_CardTemplate.h"
#include "Components/CanvasPanelSlot.h"
#include "Game/Character/Player/GASCoursePlayerState.h"

void UGASC_UI_CardHand::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateCardPositions(InDeltaTime);
}

void UGASC_UI_CardHand::NativeConstruct()
{
	if (AGASCoursePlayerState* PS = GetOwningPlayer()->GetPlayerState<AGASCoursePlayerState>())
	{
		AbilitySystemComponent = PS->GetAbilitySystemComponent();
		DeckManager = PS->GetDeckManagerComponent();
	}
	
	Super::NativeConstruct();
}

void UGASC_UI_CardHand::UpdateCardPositions(float DeltaTime)
{
	SCOPED_NAMED_EVENT(UpdateCardPositions, FColor::Green);
	if (CardsInHand.IsEmpty())
	{
		return;
	}
	
	int32 NumberOfCards = CardsInHand.Num();
	
	for (UGASC_UI_CardTemplate* CurrentCard : CardsInHand)
	{
		uint32 CardIndex = CardsInHand.Find(CurrentCard);
		
		FWidgetTransform TargetTransform = CalculateCardTransform_Native(CurrentCard, CardIndex, NumberOfCards);
		
		if (SelectedCardIndex == CardIndex)
		{
			TargetTransform = CalculateSelectedCardTransform_Native(CurrentCard, TargetTransform);
		}

		CurrentCard->SetRenderTransform(InterpolateCardTransform_Native(CurrentCard->GetRenderTransform(), TargetTransform, DeltaTime));
	}
}

FWidgetTransform UGASC_UI_CardHand::CalculateSelectedCardTransform_Native(UGASC_UI_CardTemplate*  SelectedCard, FWidgetTransform TargetTransform)
{
	SCOPED_NAMED_EVENT(CalculateSelectedCardTransform_Native, FColor::Red);
	float InTargetTranslationX = TargetTransform.Translation.X;
	float InTargetTranslationY = TargetTransform.Translation.Y;
	float InTargetAngle = TargetTransform.Angle;
	
	float OutTargetTranslationX = (InTargetTranslationX + ((SelectedCard->GetDesiredSize().Y * CardScale) * SelectedShowXAmount * (FMath::Sin(FMath::DegreesToRadians(InTargetAngle)))));
	float OutTargetTranslationY = (InTargetTranslationY + ((FMath::Cos(FMath::DegreesToRadians(InTargetAngle)) * (SelectedShowYAmount * -1.0f) * (SelectedCard->GetDesiredSize().Y * CardScale))));
	FVector2D OutTargetTranslation = FVector2D(OutTargetTranslationX, OutTargetTranslationY);
	float OutCardScale = CardScale * SelectedCardScale;
	FVector2D OutTargetScale = FVector2D(OutCardScale, OutCardScale);
	
	FVector2D OutTargetShear = bStraightSelectedCard ? FVector2D::ZeroVector : TargetTransform.Shear;
	float OutTargetAngle = bStraightSelectedCard ? 0.0f : TargetTransform.Angle;
	
	return FWidgetTransform(OutTargetTranslation, OutTargetScale, OutTargetShear, OutTargetAngle);
}

FWidgetTransform UGASC_UI_CardHand::CalculateCardTransform_Native(UGASC_UI_CardTemplate* CurrentCard, int32 CurrentCardIndex,
	int32 TotalNumberOfCards)
{
	SCOPED_NAMED_EVENT(CalculateCardTransform_Native, FColor::Blue);
	float OutTranslationX = CalculateCardXTranslation_Native(CurrentCard, CurrentCardIndex, TotalNumberOfCards);
	float OutTranslationY = CalculateCardYTranslation_Native(CurrentCard, CurrentCardIndex, TotalNumberOfCards);
	float OutCardAngle = CalculateCardAngle_Native(CurrentCardIndex, TotalNumberOfCards);
	
	FVector2D OutCardTranslation = FVector2D(OutTranslationX, OutTranslationY);
	FVector2D OutCardScale = FVector2D(CardScale, CardScale);
	FVector2D OutCardShear = FVector2D(0.0f, 0.0f);
	
	return FWidgetTransform(OutCardTranslation, OutCardScale, OutCardShear, OutCardAngle);
}

FWidgetTransform UGASC_UI_CardHand::InterpolateCardTransform_Native(FWidgetTransform CurrentTransform, FWidgetTransform TargetTransform,
	float DeltaTime)
{
	FVector2D CurrentCardTranslation = CurrentTransform.Translation;
	FVector2D TargetCardTranslation = TargetTransform.Translation;
	FVector2D InterpolatedCardTranslation = FMath::Vector2DInterpTo(CurrentCardTranslation, TargetCardTranslation, DeltaTime, AnimationSpeed);
	
	FVector2D CurrentCardScale = CurrentTransform.Scale;
	FVector2D TargetCardScale = TargetTransform.Scale;
	FVector2D InterpolatedCardScale = FMath::Vector2DInterpTo(CurrentCardScale, TargetCardScale, DeltaTime, AnimationSpeed);
	
	//We don't want any card shear
	FVector2D CurrentCardShear = CurrentTransform.Shear;
	FVector2D TargetCardShear = TargetTransform.Shear;
	FVector2D InterpolatedCardShear =  FMath::Vector2DInterpTo(CurrentCardShear, TargetCardShear, DeltaTime, AnimationSpeed);
	
	float CurrentCardAngle = CurrentTransform.Angle;
	float TargetCardAngle = TargetTransform.Angle;
	float InterpolatedCardAngle = FMath::FInterpTo(CurrentCardAngle, TargetCardAngle, DeltaTime, AnimationSpeed);
	
	return FWidgetTransform(InterpolatedCardTranslation, InterpolatedCardScale, InterpolatedCardShear, InterpolatedCardAngle);
}

float UGASC_UI_CardHand::CalculateYOffset_Native() const
{
	return GetCachedGeometry().GetLocalSize().Y + ExtraYOffset;
}

float UGASC_UI_CardHand::CalculateCardXTranslation_Native(UGASC_UI_CardTemplate* CurrentCard, int32 CurrentCardIndex,
                                                          int32 TotalNumberOfCards) const
{
	 if (!CurrentCard || TotalNumberOfCards <= 0)
    {
        return 0.0;
    }

    const double HandWidth   = static_cast<double>(GetCachedGeometry().GetLocalSize().X);
    const double CardWidth   = static_cast<double>(CurrentCard->GetDesiredSize().X) * static_cast<double>(CardScale);
    const int32  TotalMinus1 = TotalNumberOfCards - 1;

    // ----- Selection-based extra spacing (only valid when selection exists and isn't last card)
    const double SelectionDelta = static_cast<double>(SelectedShowXAmount) - static_cast<double>(MinimumCardSpacing);
    const double SelectedExtraX = CardWidth * SelectionDelta;

    const bool bHasValidSelectionSpacing =
        (SelectedCardIndex >= 0) && (SelectedCardIndex < TotalMinus1);

    const double SelectedExtraXOrZero = bHasValidSelectionSpacing ? SelectedExtraX : 0.0;

    // Default “usable” width inside the hand after reserving selection spacing (BP logic)
    const double DefaultCardWidth = HandWidth - SelectedExtraXOrZero;

    // ----- AvailableWidth clamp
    const double AvailableByHand    = DefaultCardWidth * static_cast<double>(HandWidthPercentage);
    const double AvailableBySpacing = CardWidth * static_cast<double>(MaximumCardSpacing) * static_cast<double>(FMath::Max(TotalMinus1, 0));
    const double AvailableWidth     = FMath::Min(AvailableByHand, AvailableBySpacing);

    // Center the strip
    const double StartingPoint = (AvailableWidth - DefaultCardWidth) * 0.5;

    // ----- Per-card spacing values
    const double MinCardWidth = CardWidth * static_cast<double>(MinimumCardSpacing);
    const int32  MaxCardsDisplayed =
        (MinCardWidth > 0.0) ? FMath::FloorToInt(AvailableWidth / MinCardWidth) : 0;

    // ----- Selection “push” to the right for cards after selected index
    const int32 SafeSelectedIndex = (SelectedCardIndex >= 0) ? SelectedCardIndex : MAX_int32;
    const double AfterSelectedOffset = (CurrentCardIndex > SafeSelectedIndex) ? SelectedExtraX : 0.0;

    // ----- Stacking mode (BP: TotalNumberOfCards > MaxCardsDisplayed && bDoStackCardsOnSides)
    const bool bStackOnSides = (TotalNumberOfCards > MaxCardsDisplayed) && bDoStackCardsOnSides;

    double BaseX = 0.0;

    if (bStackOnSides)
    {
        // When nothing is selected, BP uses either leftmost or rightmost as the "selection anchor"
        const int32 FallbackIndex = bKeepStackOnLeft ? TotalMinus1 : 0;
        const int32 AnchorIndex   = (SelectedCardIndex == -1) ? FallbackIndex : SelectedCardIndex;

        const int32 Half = MaxCardsDisplayed / 2;
        const int32 Rem  = MaxCardsDisplayed % 2;

        // Window bounds around AnchorIndex
        const int32 WindowLeftCandidate  = AnchorIndex - Half;
        const int32 WindowRightCandidate = AnchorIndex + Half + Rem;

        // If right bound exceeds total, shift window left (ShiftLeft <= 0)
        const int32 ShiftLeft = FMath::Min(TotalMinus1 - WindowRightCandidate, 0);

        const int32 WindowStart = FMath::Max(WindowLeftCandidate + ShiftLeft, 0);
        const int32 WindowEnd   = FMath::Min(WindowRightCandidate, TotalMinus1);

        // Clamp cards outside window to the "stack" positions
        if (CurrentCardIndex < WindowStart)
        {
            BaseX = StartingPoint; // left stack
        }
        else if (CurrentCardIndex > WindowEnd)
        {
            BaseX = StartingPoint + (static_cast<double>(MaxCardsDisplayed + 1) * MinCardWidth); // right stack
        }
        else
        {
            const int32 OffsetIndex = CurrentCardIndex - WindowStart;
            BaseX = StartingPoint + (static_cast<double>(OffsetIndex) * MinCardWidth);
        }
    }
    else
    {
        // Normal spread across AvailableWidth
        if (TotalMinus1 > 0)
        {
            const double T = static_cast<double>(CurrentCardIndex) / static_cast<double>(TotalMinus1);
            BaseX = StartingPoint + AvailableWidth * T;
        }
        else
        {
            // Single card case: keep at starting point (safe)
            BaseX = StartingPoint;
        }
    }

	return MinCardWidth * CurrentCardIndex;
   // return static_cast<float>(BaseX + AfterSelectedOffset);
}

float UGASC_UI_CardHand::CalculateCardYTranslation_Native(TWeakObjectPtr<UGASC_UI_CardTemplate> CurrentCard, int32 CurrentCardIndex,
	int32 TotalNumberOfCards) const
{
	float CardDesiredSizeY = CurrentCard->GetDesiredSize().Y;
	
	float A = CurrentCardIndex - ((TotalNumberOfCards - 1)/ 2.0f);
	A *= A;
	
	float B = (CardDesiredSizeY * CardScale * CardArcAmount) / (TotalNumberOfCards * TotalNumberOfCards);
	
	float C = A * B;
	
	return C + CalculateYOffset_Native();
}

float UGASC_UI_CardHand::CalculateCardAngle_Native(int32 CurrentCardIndex, int32 TotalNumberOfCards) const
{
	if (TotalNumberOfCards == 1)
	{
		return 0.0f;
	}
	
	if (TotalNumberOfCards == 2)
	{
		switch (CurrentCardIndex)
		{
			case 0:
				return -(CardAngle)/ 3.0f;
			case 1:
				return 0.0f;
			default:
				return 0.0f;
		}
	}
	
	return (CurrentCardIndex * (CardAngle * 2.0f) / (TotalNumberOfCards - 1) + (CardAngle * -1.0f));
}

void UGASC_UI_CardHand::AddCardToHand()
{
	SCOPED_NAMED_EVENT(AddCardToHand, FColor::Blue);
	if (!CardTemplateClass)
	{
		UE_LOGFMT(LogTemp, Error, "CardTemplateClass is not set in UGASC_UI_CardHand");
		return;
	}
	if (UGASC_UI_CardTemplate* NewCard = CreateWidget<UGASC_UI_CardTemplate>(GetOwningPlayer(), CardTemplateClass))
	{
		if (!CardHandCanvas->HasChild(NewCard))
		{
			UCanvasPanelSlot* CardSlot = CardHandCanvas->AddChildToCanvas(NewCard);
			CardSlot->SetAutoSize(true);
			CardSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		}
		NewCard->AddToViewport();
		uint32 index = CardsInHand.Add(NewCard);
		UWidgetLayoutLibrary::SlotAsCanvasSlot(NewCard)->SetZOrder(index); 
		AddCardToHand_Event();
	}
}

void UGASC_UI_CardHand::AddCardToHand_WithData(UCardDataAsset* CardData)
{
	SCOPED_NAMED_EVENT(AddCardToHand, FColor::Blue);
	if (!CardTemplateClass)
	{
		UE_LOGFMT(LogTemp, Error, "CardTemplateClass is not set in UGASC_UI_CardHand");
		return;
	}
	if (UGASC_UI_CardTemplate* NewCard = CreateWidget<UGASC_UI_CardTemplate>(GetOwningPlayer(), CardTemplateClass))
	{
		if (!CardHandCanvas->HasChild(NewCard))
		{
			UCanvasPanelSlot* CardSlot = CardHandCanvas->AddChildToCanvas(NewCard);
			CardSlot->SetAutoSize(true);
			CardSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		}
		CardsInHand.Add(NewCard);
		uint32 index = CardsInHand.Num();
		NewCard->CardData = CardData;
		NewCard->InstantiateCardData();
		if (CardsInHand.Num() == 1)
		{
			UWidgetLayoutLibrary::SlotAsCanvasSlot(NewCard)->SetZOrder(100);
		}
		else
		{
			UWidgetLayoutLibrary::SlotAsCanvasSlot(NewCard)->SetZOrder(CardsInHand.Num() + 1); 
		}
		
		AddCardToHand_Event();
	}
}

void UGASC_UI_CardHand::AddCardInstanceToHand(FCardInstance CardInstanceData)
{
	SCOPED_NAMED_EVENT(AddCardToHand, FColor::Blue);
	if (!CardTemplateClass)
	{
		UE_LOGFMT(LogTemp, Error, "CardTemplateClass is not set in UGASC_UI_CardHand");
		return;
	}
	if (UGASC_UI_CardTemplate* NewCard = CreateWidget<UGASC_UI_CardTemplate>(GetOwningPlayer(), CardTemplateClass))
	{
		if (!CardHandCanvas->HasChild(NewCard))
		{
			UCanvasPanelSlot* CardSlot = CardHandCanvas->AddChildToCanvas(NewCard);
			CardSlot->SetAutoSize(true);
			CardSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		}
		CardsInHand.Add(NewCard);
		uint32 index = CardsInHand.Num();
		NewCard->CardInstance = CardInstanceData;
		NewCard->InstantiateCardData();
		if (CardsInHand.Num() == 1)
		{
			UWidgetLayoutLibrary::SlotAsCanvasSlot(NewCard)->SetZOrder(100);
		}
		else
		{
			UWidgetLayoutLibrary::SlotAsCanvasSlot(NewCard)->SetZOrder(CardsInHand.Num() + 1); 
		}
		
		AddCardToHand_Event();
	}
}

void UGASC_UI_CardHand::RemoveCardFromHand()
{
	if (CardsInHand.Num() == 0)
	{
		return;
	}
	UGASC_UI_CardTemplate* CardToRemove = CardsInHand.Pop();
	CardHandCanvas->RemoveChild(CardToRemove);
}

int32 UGASC_UI_CardHand::GetNumberOfCardsInHand() const
{
	return CardsInHand.Num();
}
