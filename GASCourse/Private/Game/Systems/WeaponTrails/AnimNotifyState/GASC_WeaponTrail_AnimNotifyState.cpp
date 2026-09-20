// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/WeaponTrails/AnimNotifyState/GASC_WeaponTrail_AnimNotifyState.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Curves/CurveLinearColor.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Systems/WeaponTrails/GASC_WeaponTrails_DataAsset.h"

DEFINE_LOG_CATEGORY(LogGASCourseWeaponTrail);

UGASC_WeaponTrail_AnimNotifyState::UGASC_WeaponTrail_AnimNotifyState(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LocationOffset = FVector::ZeroVector;
	RotationOffset = FRotator::ZeroRotator;
	bDestroyAtEnd = false;
}

void UGASC_WeaponTrail_AnimNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(
		MeshComp,
		Animation,
		TotalDuration,
		EventReference);

	if (!IsValid(MeshComp))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT("Invalid skeletal mesh component in %s."),
			*GetPathNameSafe(this));

		return;
	}

	if (!IsValid(WeaponTrailData))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT("Invalid Weapon Trail data in %s. Please fix."),
			*GetPathNameSafe(this));

		return;
	}

	UNiagaraComponent* SpawnedComponent = GetSpawnedEffect(MeshComp);

	if (!IsValid(SpawnedComponent))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT("Failed to spawn Weapon Trail Niagara component in %s."),
			*GetPathNameSafe(this));

		return;
	}

	TArray<TWeakObjectPtr<UNiagaraComponent>>& MeshTrails =
		ActiveWeaponTrails.FindOrAdd(MeshComp);

	// Remove stale references left by components that were destroyed externally.
	MeshTrails.RemoveAll(
		[](const TWeakObjectPtr<UNiagaraComponent>& Component)
		{
			return !Component.IsValid();
		});

	MeshTrails.Add(SpawnedComponent);

	SetWeaponTrailMaterialInterface(SpawnedComponent);
	SetWeaponTrailColorArrayAtTime(SpawnedComponent, 0.0f);
	SetWeaponTrailLifeTime(
		SpawnedComponent,
		WeaponTrailData->TrailLifeTime);

	SetWeaponTrailRibbonWidth(
		SpawnedComponent,
		MeshComp->GetOwner(),
		WeaponTrailData->TrailWidth);
}

void UGASC_WeaponTrail_AnimNotifyState::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(
		MeshComp,
		Animation,
		FrameDeltaTime,
		EventReference);

	if (!IsValid(MeshComp) || !IsValid(WeaponTrailData))
	{
		return;
	}

	TArray<TWeakObjectPtr<UNiagaraComponent>>* MeshTrails =
		ActiveWeaponTrails.Find(MeshComp);

	if (!MeshTrails)
	{
		return;
	}

	MeshTrails->RemoveAll(
		[](const TWeakObjectPtr<UNiagaraComponent>& Component)
		{
			return !Component.IsValid();
		});

	if (MeshTrails->IsEmpty())
	{
		ActiveWeaponTrails.Remove(MeshComp);
		return;
	}

	// The most recently spawned trail belongs to the current notify execution.
	UNiagaraComponent* NiagaraComponent = MeshTrails->Last().Get();

	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	const float CurrentTimeRatio =
		UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(
			EventReference);

	SetWeaponTrailColorArrayAtTime(
		NiagaraComponent,
		CurrentTimeRatio);
}

void UGASC_WeaponTrail_AnimNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	DestroyWeaponTrailVFX(MeshComp);

	Super::NotifyEnd(
		MeshComp,
		Animation,
		EventReference);
}

UNiagaraComponent* UGASC_WeaponTrail_AnimNotifyState::GetSpawnedEffect(
	UMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp) || !IsValid(WeaponTrailData))
	{
		return nullptr;
	}

	FVector TrailLocation = LocationOffset;
	FRotator TrailRotation = RotationOffset;
	UMeshComponent* AttachComponent = MeshComp;

	if (bTransformByWeaponSocket)
	{
		if (UGASC_CharacterWeapon_Base* PrimaryWeapon =
			GetCharacterWeaponFromInventory(MeshComp->GetOwner()))
		{
			TrailLocation +=
				PrimaryWeapon->GetWeaponTrailMidPoint(WeaponIndex);

			TrailRotation =
				PrimaryWeapon->GetWeaponTrailRotation(WeaponIndex);

			if (UMeshComponent* WeaponMesh =
				PrimaryWeapon->GetPrimaryWeaponMeshComponent(WeaponIndex))
			{
				AttachComponent = WeaponMesh;
			}
		}
	}

	if (!IsValid(AttachComponent))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT("Invalid trail attachment component in %s."),
			*GetPathNameSafe(this));

		return nullptr;
	}

	UNiagaraSystem* WeaponTrailSystem = WeaponTrailData->WeaponTrailNiagaraSystem;

	if (!IsValid(WeaponTrailSystem))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT("Invalid Weapon Trail Niagara System in %s."),
			*GetPathNameSafe(WeaponTrailData));

		return nullptr;
	}

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		WeaponTrailSystem,
		AttachComponent,
		SocketName,
		TrailLocation,
		TrailRotation,
		EAttachLocation::KeepWorldPosition,
		false,                 // bAutoDestroy: managed explicitly by notify
		true,                  // bAutoActivate
		ENCPoolMethod::None,   // avoid AutoRelease/manual-destroy conflict
		true                   // bPreCullCheck
	);
}

void UGASC_WeaponTrail_AnimNotifyState::SetWeaponTrailMaterialInterface(
	UNiagaraComponent* InWeaponTrailNiagaraComponent) const
{
	if (!IsValid(InWeaponTrailNiagaraComponent) ||
		!IsValid(WeaponTrailData))
	{
		return;
	}

	UMaterialInterface* TrailMaterial =
		WeaponTrailData->WeaponTrailMaterialInterface;

	if (!IsValid(TrailMaterial))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT(
				"Invalid Weapon Trail Material Interface in data %s, notify %s."),
			*GetPathNameSafe(WeaponTrailData),
			*GetPathNameSafe(this));

		return;
	}

	InWeaponTrailNiagaraComponent->SetVariableMaterial(
		TEXT("TrailMaterial"),
		TrailMaterial);
}

void UGASC_WeaponTrail_AnimNotifyState::SetWeaponTrailColorArrayAtTime(
	UNiagaraComponent* InWeaponTrailNiagaraComponent,
	float InTime) const
{
	if (!IsValid(InWeaponTrailNiagaraComponent) ||
		!IsValid(WeaponTrailData))
	{
		return;
	}

	UCurveLinearColor* ColorCurve = WeaponTrailData->ColorCurve;

	if (!IsValid(ColorCurve))
	{
		UE_LOG(
			LogGASCourseWeaponTrail,
			Warning,
			TEXT(
				"Invalid Weapon Trail Color Curve in data %s, notify %s."),
			*GetPathNameSafe(WeaponTrailData),
			*GetPathNameSafe(this));

		return;
	}

	const float ClampedTime = FMath::Clamp(InTime, 0.0f, 1.0f);
	const FLinearColor ColorData =
		ColorCurve->GetLinearColorValue(ClampedTime);

	const TArray<FLinearColor> ArrayData = {ColorData};

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
		InWeaponTrailNiagaraComponent,
		TEXT("Color Selection Array_Color"),
		ArrayData);
}

void UGASC_WeaponTrail_AnimNotifyState::SetWeaponTrailLifeTime(
	UNiagaraComponent* InWeaponTrailNiagaraComponent,
	float InLifeTime) const
{
	if (!IsValid(InWeaponTrailNiagaraComponent))
	{
		return;
	}

	InWeaponTrailNiagaraComponent->SetVariableFloat(
		TEXT("LifeTime"),
		FMath::Max(0.0f, InLifeTime));
}

void UGASC_WeaponTrail_AnimNotifyState::SetWeaponTrailRibbonWidth(
	UNiagaraComponent* InWeaponTrailNiagaraComponent,
	AActor* OwnerActor,
	float InRibbonWidth) const
{
	if (!IsValid(InWeaponTrailNiagaraComponent))
	{
		return;
	}

	float FinalRibbonWidth = InRibbonWidth;

	if (bTransformByWeaponSocket)
	{
		if (UGASC_CharacterWeapon_Base* PrimaryWeapon =
			GetCharacterWeaponFromInventory(OwnerActor))
		{
			FinalRibbonWidth =
				PrimaryWeapon->GetWeaponTrailWidth(WeaponIndex);
		}
	}

	InWeaponTrailNiagaraComponent->SetVariableFloat(
		TEXT("TrailWidth"),
		FMath::Max(0.0f, FinalRibbonWidth));
}

UGASC_CharacterWeapon_Base*
UGASC_WeaponTrail_AnimNotifyState::GetCharacterWeaponFromInventory(
	AActor* OwningActor) const
{
	AGASCoursePlayerCharacter* PlayerCharacter =
		Cast<AGASCoursePlayerCharacter>(OwningActor);

	if (!IsValid(PlayerCharacter))
	{
		return nullptr;
	}

	UGASC_WeaponInventoryComponent* InventoryComponent =
		PlayerCharacter->FindComponentByClass<
			UGASC_WeaponInventoryComponent>();

	if (!IsValid(InventoryComponent))
	{
		return nullptr;
	}

	return InventoryComponent->GetPrimaryWeaponObject();
}

void UGASC_WeaponTrail_AnimNotifyState::DestroyWeaponTrailVFX(
	USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	TArray<TWeakObjectPtr<UNiagaraComponent>>* MeshTrails =
		ActiveWeaponTrails.Find(MeshComp);

	if (!MeshTrails)
	{
		return;
	}

	MeshTrails->RemoveAll(
		[](const TWeakObjectPtr<UNiagaraComponent>& Component)
		{
			return !Component.IsValid();
		});

	if (MeshTrails->IsEmpty())
	{
		ActiveWeaponTrails.Remove(MeshComp);
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> TrailReference =
		MeshTrails->Pop(EAllowShrinking::No);

	if (UNiagaraComponent* NiagaraComponent = TrailReference.Get())
	{
		if (bDestroyAtEnd)
		{
			NiagaraComponent->DeactivateImmediate();
			NiagaraComponent->DestroyComponent();
		}
		else
		{
			// Allows existing ribbon particles to finish their configured lifetime.
			NiagaraComponent->Deactivate();
		}
	}

	if (MeshTrails->IsEmpty())
	{
		ActiveWeaponTrails.Remove(MeshComp);
	}
}
