// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Animation/Notify/GASC_Locomotion_FootPlant_Notify.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "KismetTraceUtils.h"

UGASC_Locomotion_FootPlant_Notify::UGASC_Locomotion_FootPlant_Notify()
{
	
}

void UGASC_Locomotion_FootPlant_Notify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp->DoesSocketExist(FootPlantNotifyName))
	{
		UE_LOGFMT(LogTemp, Warning, "{0} socket does not exist on {1}", FootPlantNotifyName, MeshComp->GetName());
		return;
	}

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			if (!FootPlantGameplayCueTag.IsValid())
			{
				return;
			}
			FVector StartLocation = MeshComp->GetSocketLocation(FootPlantNotifyName);
			FVector TraceDirection = Owner->GetActorUpVector();
			FVector EndLocation = StartLocation + (TraceDirection * -TraceDistance);
			FCollisionQueryParams TraceParams(FName(TEXT("TargetTraceChannel")), false, Owner);
			TraceParams.AddIgnoredActor(Owner);
			TraceParams.bReturnPhysicalMaterial = true;

			FTraceDelegate Delegate = FTraceDelegate::CreateUObject(this, &UGASC_Locomotion_FootPlant_Notify::TraceFootPlant, Owner);
			World->AsyncLineTraceByProfile(EAsyncTraceType::Single, StartLocation, EndLocation, FName(TEXT("TargetTraceChannel")), TraceParams, &Delegate);
		}
	}
}

FString UGASC_Locomotion_FootPlant_Notify::GetNotifyName_Implementation() const
{
	if (FootPlantGameplayCueTag.IsValid())
	{
		return FootPlantGameplayCueTag.ToString();
	}
	return Super::GetNotifyName_Implementation();
}

void UGASC_Locomotion_FootPlant_Notify::TraceFootPlant(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum, AActor* InOwner) const
{
	bool bHasBlockingHit = false;
	FHitResult FoundHitResult;
	for (const FHitResult& HitResult : InTraceDatum.OutHits)
	{
		if (HitResult.bBlockingHit)
		{
			bHasBlockingHit = true;
			FoundHitResult = HitResult;
			break;
		}
	}
	if (!bHasBlockingHit)
	{
		return;
	}
#if !UE_BUILD_SHIPPING
	if (bDebug)
	{
		DrawDebugLineTraceMulti(InOwner->GetWorld(), InTraceDatum.Start, InTraceDatum.End, DrawDebugTraceType, bHasBlockingHit, InTraceDatum.OutHits,
			FColor::Green, FColor::Red, DebugDrawTime);
	}
#endif
	
	
	FGameplayCueParameters GameplayCueParams = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(FoundHitResult);
	UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(InOwner, FootPlantGameplayCueTag, GameplayCueParams);
}
