// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Game/Systems/CardEnergy/GASCourseCardEnergyExecution.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"
#include "Game/Systems/Damage/Debug/DamagePipelineDebugSubsystem.h"

UGASCourseHealthAttributeSet::UGASCourseHealthAttributeSet()
{

}

void UGASCourseHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if(Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(CurrentHealth, MaxHealth, NewValue, GetCurrentHealthAttribute());
	}
	
	if(Attribute == GetAllDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	
	if(Attribute == GetElementalDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}

	if(Attribute == GetPhysicalDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}

	if(Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	
	if(Attribute == GetCriticalDamageMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}
	
	if(Attribute == GetDamageResistanceMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	
	if(Attribute == GetDamageMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
}

void UGASCourseHealthAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	
	if(Attribute == GetDamageResistanceMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	
	if(Attribute == GetDamageMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
}

void UGASCourseHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                                       float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
}

void UGASCourseHealthAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UGASCourseHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectSpec& Spec                     = Data.EffectSpec;
	const FGameplayEffectContextHandle& ContextHandle   = Spec.GetContext();
	const FGameplayTagContainer& DynamicTags            = Spec.DynamicGrantedTags;

	// ---------------------------------------------------------------------
	// Resolve target info once
	// ---------------------------------------------------------------------
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AGASCourseCharacter* TargetCharacter = nullptr;
	UGASCourseAbilitySystemComponent* TargetASC = nullptr;

	if (Data.Target.AbilityActorInfo.IsValid())
	{
		const FGameplayAbilityActorInfo* TargetInfo = Data.Target.AbilityActorInfo.Get();
		TargetActor      = TargetInfo->AvatarActor.Get();
		TargetController = TargetInfo->PlayerController.Get();
		TargetCharacter  = Cast<AGASCourseCharacter>(TargetActor);

		if (TargetCharacter)
		{
			TargetASC = Cast<UGASCourseAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent());
		}
	}

	// ---------------------------------------------------------------------
	// Resolve source info once
	// ---------------------------------------------------------------------
	AActor* SourceActor = ContextHandle.IsValid() ? ContextHandle.GetInstigator() : nullptr;
	AGASCourseCharacter* SourceCharacter = Cast<AGASCourseCharacter>(SourceActor);
	UGASCourseAbilitySystemComponent* SourceASC =
		ContextHandle.IsValid()
			? Cast<UGASCourseAbilitySystemComponent>(ContextHandle.GetInstigatorAbilitySystemComponent())
			: nullptr;

	// Resolve world once
	UWorld* World = nullptr;
	if (TargetActor)
	{
		World = TargetActor->GetWorld();
	}
	else if (SourceActor)
	{
		World = SourceActor->GetWorld();
	}

	// ---------------------------------------------------------------------
	// Precompute commonly used tag info
	// ---------------------------------------------------------------------
	const bool bIsCritical         = DynamicTags.HasTagExact(DamageType_Critical);
	const bool bIsDamageOverTime   = DynamicTags.HasTagExact(Data_DamageOverTime);
	const bool bDamageResisted     = DynamicTags.HasTagExact(DamageType_Resistance);

	FGameplayTag DamageTypeTag;
	for (const FGameplayTag& Tag : DynamicTags)
	{
		if (Tag.MatchesTag(DamageType_Root))
		{
			DamageTypeTag = Tag;
			break;
		}
	}

	// Convenience pointers to owned tag containers (no copying)
	const FGameplayTagContainer* SourceOwnedTags = SourceASC ? &SourceASC->GetOwnedGameplayTags() : nullptr;
	const FGameplayTagContainer* TargetOwnedTags = TargetASC ? &TargetASC->GetOwnedGameplayTags() : nullptr;

	// =====================================================================
	// DAMAGE BRANCH
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (!TargetCharacter || !TargetASC || !World)
		{
			goto ClampHealth;
		}
		
		if (LocalDamage <= 0.0f && !bDamageResisted)
		{
			goto ClampHealth;
		}

		const bool bWasAlive = TargetCharacter->IsCharacterAlive();
		const float OldHealth = CurrentHealth.GetCurrentValue();
		const float NewHealth = OldHealth - LocalDamage;

		SetCurrentHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
		
		// 1. Get original context
		FGameplayEffectContextHandle Original = Spec.GetEffectContext();

		// 2. Duplicate into a writable one
		FGameplayEffectContextHandle NewContext = Original.Duplicate();

		// 3. Modify your custom data
		FGASCourseGameplayEffectContext* MutableContext =
			static_cast<FGASCourseGameplayEffectContext*>(NewContext.Get());
			
		MutableContext->DamageLogEntry.FinalDamageValue = NewHealth >= GetMaxHealth() ? GetMaxHealth() - GetCurrentHealth() : LocalDamage;
		MutableContext->DamageLogEntry.bIsCriticalHit = bIsCritical;
		MutableContext->DamageLogEntry.bIsDamageEffect = true;
		MutableContext->DamageLogEntry.bIsOverTimeEffect = bIsDamageOverTime;
		MutableContext->DamageLogEntry.bDamageResisted = bDamageResisted;
			
		if (UDamagePipelineDebugSubsystem* Debug = GetWorld()->GetSubsystem<UDamagePipelineDebugSubsystem>())
		{
			MutableContext->DamageLogEntry.DamageID = Debug->GenerateDebugDamageUniqueID();
			Debug->LogDamageEvent(MutableContext->DamageLogEntry);
		}

		// ---------------- Damage pipeline event ----------------
		if (UGASC_DamagePipelineSubsystem* DPS = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			FDamageModificationContext ModContext;
			ModContext.bCriticalModification     = bIsCritical;
			ModContext.bModificationOverTime     = bIsDamageOverTime;
			ModContext.bDamageResisted           = bDamageResisted;
			ModContext.bDamageModificationKilled = (NewHealth <= 0.0f && bWasAlive);
			ModContext.DamagePipelineType        = Damage;
			ModContext.DeltaValue                = LocalDamage;
			ModContext.NewValue                  = NewHealth;
			ModContext.DamageType                = DamageTypeTag;

			FHitContext HitCtx;
			HitCtx.HitTarget      = TargetActor;
			HitCtx.HitInstigator  = SourceActor;
			HitCtx.HitTargetTagsContainer      = TargetOwnedTags;
			HitCtx.HitInstigatorTagsContainer  = SourceOwnedTags;
			if (ContextHandle.GetHitResult())
			{
				HitCtx.HitResult = *ContextHandle.GetHitResult();
			}
			HitCtx.HitTimeStamp            = World->GetTimeSeconds();
			HitCtx.OptionalSourceObject    = Cast<AActor>(ContextHandle.GetSourceObject());
			HitCtx.HitContextTagsContainer = &DynamicTags;

			ModContext.HitContext = HitCtx;

			// Delegates broadcast synchronously -> pointer to HitCtx is safe
			DPS->Internal_BroadcastDamageApplied(ModContext);
			DPS->Internal_BroadcastDamageReceived(ModContext);
		}

		// ---------------- Death handling ----------------
		if (NewHealth <= 0.0f && bWasAlive && TargetASC && SourceASC)
		{
			TargetCharacter->SetCharacterDead(true);

			FGameplayEventData OnDeathPayload;
			OnDeathPayload.EventTag       = Event_OnDeath;
			OnDeathPayload.Instigator     = ContextHandle.GetOriginalInstigator();
			OnDeathPayload.Target         = GetOwningActor();
			OnDeathPayload.ContextHandle  = ContextHandle;
			OnDeathPayload.EventMagnitude = LocalDamage;

			if (TargetOwnedTags)
			{
				OnDeathPayload.TargetTags = *TargetOwnedTags;
			}
			if (SourceOwnedTags)
			{
				OnDeathPayload.InstigatorTags = *SourceOwnedTags;
			}

			TargetASC->HandleGameplayEvent(Event_OnDeath, &OnDeathPayload);
			SourceASC->HandleGameplayEvent(Event_OnDeathDealt, &OnDeathPayload);

			if (AbilitySystemSettings)
			{
				TSubclassOf<UGameplayEffectExecutionCalculation> CardResourceExec =
					AbilitySystemSettings->CardResourceExecution;
				if (!CardResourceExec)
				{
					UE_LOG(LogTemp, Warning, TEXT("Health Calculation is not valid!"));
				}
			}
		}
	}

	// =====================================================================
	// PASSIVE HEALING BRANCH
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
	{
		const float LocalHeal = GetIncomingHealing();
		SetIncomingHealing(0.0f);
		if (CurrentHealth.GetCurrentValue() != MaxHealth.GetCurrentValue())
		{
			if (!TargetCharacter || !TargetASC || !World || LocalHeal <= 0.0f)
			{
				goto ClampHealth;
			}
			
			float NewHealth = GetCurrentHealth() + LocalHeal;
			float TrueHealthDelta = NewHealth >= GetMaxHealth() ? GetMaxHealth() - GetCurrentHealth() : LocalHeal;
			NewHealth = FMath::Clamp(NewHealth, 0.0f, GetMaxHealth());
			SetCurrentHealth(NewHealth);
			
			// 1. Get original context
			FGameplayEffectContextHandle Original = Spec.GetEffectContext();

			// 2. Duplicate into a writable one
			FGameplayEffectContextHandle NewContext = Original.Duplicate();

			// 3. Modify your custom data
			FGASCourseGameplayEffectContext* MutableContext =
				static_cast<FGASCourseGameplayEffectContext*>(NewContext.Get());
			
			MutableContext->DamageLogEntry.FinalDamageValue = TrueHealthDelta;
			MutableContext->DamageLogEntry.bIsCriticalHit = bIsCritical;
			MutableContext->DamageLogEntry.bIsDamageEffect = false;
			MutableContext->DamageLogEntry.bIsOverTimeEffect = bIsDamageOverTime;
			
			if (UDamagePipelineDebugSubsystem* Debug = GetWorld()->GetSubsystem<UDamagePipelineDebugSubsystem>())
			{
				MutableContext->DamageLogEntry.DamageID = Debug->GenerateDebugDamageUniqueID();
				Debug->LogDamageEvent(MutableContext->DamageLogEntry);
			}
			
			if (UGASC_DamagePipelineSubsystem* DPS = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
			{
				FDamageModificationContext ModContext;
				ModContext.bCriticalModification     = bIsCritical;
				ModContext.bModificationOverTime     = bIsDamageOverTime;
				ModContext.bDamageModificationKilled = false;
				ModContext.DamagePipelineType        = Healing;
				ModContext.DeltaValue                = TrueHealthDelta;
				ModContext.NewValue                  = NewHealth;
				ModContext.DamageType                = DamageTypeTag;

				FHitContext HitCtx;
				HitCtx.HitTarget      = TargetActor;
				HitCtx.HitInstigator  = SourceActor;
				HitCtx.HitTargetTagsContainer      = TargetOwnedTags;
				HitCtx.HitInstigatorTagsContainer  = SourceOwnedTags;
				if (ContextHandle.GetHitResult())
				{
					HitCtx.HitResult = *ContextHandle.GetHitResult();
				}
				HitCtx.HitTimeStamp            = World->GetTimeSeconds();
				HitCtx.OptionalSourceObject    = Cast<AActor>(ContextHandle.GetSourceObject());
				HitCtx.HitContextTagsContainer = &DynamicTags;

				ModContext.HitContext = HitCtx;

				DPS->Internal_ForwardOnHealingReceived(ModContext);
				DPS->Internal_ForwardOnHealingApplied(ModContext);

				// ---- Gameplay events for UI / logic ----
				if (SourceASC)
				{
					FGameplayEventData TargetHealedPayload;
					TargetHealedPayload.Instigator     = SourceASC->GetAvatarActor();
					TargetHealedPayload.Target         = TargetASC->GetAvatarActor();
					TargetHealedPayload.EventMagnitude = LocalHeal;
					TargetHealedPayload.ContextHandle  = ContextHandle;
					TargetHealedPayload.InstigatorTags = DynamicTags;
					TargetHealedPayload.InstigatorTags.AddTag(DamageType_Healing);

					// Use HandleGameplayEvent (sync) for better perf; swap to SendGameplayEventAsync if you truly need async BP.
					TargetASC->HandleGameplayEvent(Event_Gameplay_OnTargetHealed, &TargetHealedPayload);
					SourceASC->HandleGameplayEvent(Event_Gameplay_OnHealing, &TargetHealedPayload);
				}
			}
		}
	}

ClampHealth:
	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.0f, GetMaxHealth()));
	}
}

