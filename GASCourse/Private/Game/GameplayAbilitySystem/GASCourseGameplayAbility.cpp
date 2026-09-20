// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "ObjectEditorUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"
#include "Game/GameplayAbilitySystem/GameplayAbilities/GASC_AbilityParamsObject.h"
#include "Game/GameplayAbilitySystem/Tasks/DamagePipeline/GASC_OnHitEventTask.h"
#include "Game/GameplayAbilitySystem/Tasks/DamagePipeline/GASC_OnDamageEventTask.h"
#include "StructUtils/PropertyBag.h"

DEFINE_LOG_CATEGORY(LOG_GASC_GameplayAbility);

namespace GASCourseAbilityComponents
{
	/** Category component variables are authored under, so they group together in the details panel. */
	static const FName ComponentsCategory(TEXT("Components"));
}


UGASCourseGameplayAbility::UGASCourseGameplayAbility(const FObjectInitializer& ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EGASCourseAbilityActivationPolicy::OnInputTriggered;
	AbilityType = EGASCourseAbilityType::Instant;
	AbilitySlotType = EGASCourseAbilitySlotType::EmptySlot;

	bAutoCommitAbilityOnActivate = true;
}

UGASCourseAbilitySystemComponent* UGASCourseGameplayAbility::GetGASCourseAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UGASCourseAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AGASCoursePlayerController* UGASCourseGameplayAbility::GetGASCoursePlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AGASCoursePlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

AController* UGASCourseGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (const APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;
}

AGASCoursePlayerCharacter* UGASCourseGameplayAbility::GetGASCouresPlayerCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AGASCoursePlayerCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

void UGASCourseGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec) const
{
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
	{
		return;
	}
	const bool bIsPredicting = (Spec.GetPrimaryInstance()->GetCurrentActivationInfo().ActivationMode == EGameplayAbilityActivationMode::Predicting);
	
	// Try to activate if activation policy is on spawn.
	if (ActorInfo && !Spec.IsActive() && !bIsPredicting && (ActivationPolicy == EGASCourseAbilityActivationPolicy::OnSpawn))
	{
		FGameplayTagContainer SourceTags;
		UAbilitySystemComponent* InstigatorASC = GetGrantedByEffectContext().IsValid() ? GetGrantedByEffectContext().GetOriginalInstigatorAbilitySystemComponent() 
		: ActorInfo->AbilitySystemComponent.Get();
		if (InstigatorASC)
		{
			InstigatorASC->GetOwnedGameplayTags(SourceTags);
		}
		
		FGameplayTagContainer TargetTags;
		FGameplayTagContainer RelevantActivationTags;
		GetAbilitySystemComponentFromActorInfo()->GetOwnedGameplayTags(TargetTags);
		
		if(DoesAbilitySatisfyTagRequirements(*GetAbilitySystemComponentFromActorInfo(), &SourceTags, &TargetTags, &RelevantActivationTags))
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

			// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
			if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
			{
				const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
				const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

				const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
				const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

				if (bClientShouldActivate || bServerShouldActivate)
				{
					ASC->TryActivateAbility(Spec.Handle);
				}
			}
		}
	}
}

bool UGASCourseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                                   const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	//Check Components
	return ValidateComponents(ActorInfo);
}

void UGASCourseGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UGASCourseGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	K2_OnAbilityAdded();
	TryActivateAbilityOnSpawn(ActorInfo, Spec);
	
	// GAS routes this through the class default object whenever the spec has no primary instance
	// yet, so the copy below no-ops in that case rather than rewriting class defaults; PreActivate
	// picks up the slack. See ApplyAbilityParamsFromSourceObject.
	ApplyAbilityParamsFromSourceObject(Spec);

	if (GetAbilitySlot() == EGASCourseAbilitySlotType::EmptySlot)
	{
		return;
	}
	ConfigureAbilityFromGrantedSpec(Spec);
	
	FGameplayEventData OnAbilityGranted;
	OnAbilityGranted.EventTag = Event_Gameplay_OnAbilityGranted;
	OnAbilityGranted.Instigator = GetAvatarActorFromActorInfo();
	OnAbilityGranted.Target = GetAvatarActorFromActorInfo();
	OnAbilityGranted.OptionalObject = this;
	GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(Event_Gameplay_OnAbilityGranted, &OnAbilityGranted);
}

void UGASCourseGameplayAbility::OnRemoveAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	UGASCourseAbilitySystemComponent* ASC = GetGASCourseAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		Super::OnRemoveAbility(ActorInfo, Spec);
		return;
	}

	if (IsActive())
	{
		if (UGameplayAbility* Instance = Spec.GetPrimaryInstance())
		{
			EndAbility(Spec.Handle, ActorInfo, Instance->GetCurrentActivationInfo(), true, false);
		}
	}

	OnAbilityClearedDelegate.Broadcast(this);
	K2_OnAbilityRemoved();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UGASCourseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if(bAutoCommitAbilityOnActivate)
	{
		CommitAbility(Handle, ActorInfo, ActivationInfo);
	}
	if (AbilityType == EGASCourseAbilityType::Duration && bAutoApplyDurationEffect)
	{
		if(!ApplyDurationEffect())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
	}
	
	if (bListenForOnHitAppliedEvent)
	{
		//Wait for On Hit Applied Task Event
		UGASC_OnHitEventTask* WaitForOnHitAppliedEvent = UGASC_OnHitEventTask::WaitOnHitEvent(this, GetAvatarActorFromActorInfo(), EHitEventType::OnHitApplied);
		WaitForOnHitAppliedEvent->OnHitDelegate.AddDynamic(this, &UGASCourseGameplayAbility::OnHitApplied);
		WaitForOnHitAppliedEvent->ReadyForActivation();
	}

	if (bListenForOnHitReceivedEvent)
	{
		//Wait for On Hit Received Task Event
		UGASC_OnHitEventTask* WaitForOnHitReceivedEvent = UGASC_OnHitEventTask::WaitOnHitEvent(this, GetAvatarActorFromActorInfo(), EHitEventType::OnHitReceived);
		WaitForOnHitReceivedEvent->OnHitDelegate.AddDynamic(this, &UGASCourseGameplayAbility::OnHitReceived);
		WaitForOnHitReceivedEvent->ReadyForActivation();
	}
	
	if (bListenForOnDamageAppliedEvent)
	{
		UGASC_OnDamageEventTask* WaitForOnDamageAppliedEvent = UGASC_OnDamageEventTask::WaitOnDamageEvent(this, GetAvatarActorFromActorInfo(), EOnDamageEventType::OnDamageApplied);
		WaitForOnDamageAppliedEvent->OnDamageEvent.AddDynamic(this, &UGASCourseGameplayAbility::OnDamageApplied);
		WaitForOnDamageAppliedEvent->ReadyForActivation();
	}
	
	CachedInputDirection = GetInputDirection();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGASCourseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UGASCourseAbilitySystemComponent* ASC = GetGASCourseAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}

	// If this is a duration ability and its duration effect is still active,
	// just end the current activation and leave the granted ability alone.
	if (AbilityType == EGASCourseAbilityType::Duration && DurationEffect)
	{
		if (DurationEffectHandle.IsValid() && ASC->GetActiveGameplayEffect(DurationEffectHandle) != nullptr)
		{
			Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
			return;
		}
	}

	// Stack logic should use Handle, not class lookup.
	if (FGrantedCardAbilityConfig* Config = ASC->CardAbilityConfigHandles.Find(Handle))
	{
		if (Config->AbilityStackCount > 1)
		{
			--Config->AbilityStackCount;
			ASC->OnActiveCardAbilityStackCountChanged.Broadcast(Config->AbilityStackCount, GetClass());

			Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
			return;
		}

		// Last stack: remove config and clear granted ability.
		ASC->CardAbilityConfigHandles.Remove(Handle);
		ASC->ClearAbility(Handle);

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}

	// No config found, just end normally.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGASCourseGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (UGameplayEffect* CostGE = GetCostGameplayEffect())
	{
		UAbilitySystemComponent* AbilitySystemComponent = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ensure(AbilitySystemComponent))
		{
			if (!AbilitySystemComponent->CanApplyAttributeModifiers(CostGE, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo)))
			{
				FGameplayTagContainer FailureReason = FGameplayTagContainer::EmptyContainer;
				FailureReason.AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);

				if (FailureReason.IsValid())
				{
					FGameplayTag EventTag = Event_AbilityActivation_CantAffordCost;
					FGameplayEventData Payload;
					Payload.Instigator = ActorInfo->AvatarActor.Get(); //GetAvatarActorFromActorInfo();
					Payload.OptionalObject = this;
					Payload.EventTag = AbilityActivationFail_CantAffordCost;
					Payload.Target = ActorInfo->AvatarActor.Get();//GetAvatarActorFromActorInfo();
					AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);

					//Haptic feedback for failure.
					InvokeAbilityFailHapticFeedback();
					const UGameplayAbility* BaseAbility = Cast<UGameplayAbility>(this);
					AbilitySystemComponent->NotifyAbilityFailed(Handle, const_cast<UGameplayAbility*>(BaseAbility), FailureReason);
				}
				
				return false;
			}
		}
	}
	return true;
}

void UGASCourseGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	if (!AbilitySystemGlobals.ShouldIgnoreCosts())
	{
		Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	}
}

FGameplayEffectContextHandle UGASCourseGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Super::MakeEffectContext(Handle, ActorInfo);
}

void UGASCourseGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec& Spec,
	FGameplayAbilitySpec* AbilitySpec) const
{
	Super::ApplyAbilityTagsToGameplayEffectSpec(Spec, AbilitySpec);
}

bool UGASCourseGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Specialized version to handle death exclusion and AbilityTags expansion via ASC

	bool bBlocked = false;
	bool bMissing = false;

	const UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;

	// Check if any of this ability's tags are currently blocked
	if (AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags()))
	{
		bBlocked = true;
	}

	const UGASCourseAbilitySystemComponent* GASCourseASC = Cast<UGASCourseAbilitySystemComponent>(&AbilitySystemComponent);
	static FGameplayTagContainer AllRequiredTags;
	static FGameplayTagContainer AllBlockedTags;

	AllRequiredTags = ActivationRequiredTags;
	AllBlockedTags = ActivationBlockedTags;

	// Expand our ability tags to add additional required/blocked tags
	if (GASCourseASC)
	{
		GASCourseASC->GetAdditionalActivationTagRequirements(GetAssetTags(), AllRequiredTags, AllBlockedTags);
	}

	// Check to see the required/blocked tags for this ability
	if (AllBlockedTags.Num() || AllRequiredTags.Num())
	{
		static FGameplayTagContainer AbilitySystemComponentTags;
		
		AbilitySystemComponentTags.Reset();
		AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);
		
		if (AbilitySystemComponentTags.HasAny(AllBlockedTags))
		{
			if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(Status_Death))
			{
				// If player is dead and was rejected due to blocking tags, give that feedback
			}

			bBlocked = true;
		}

		if (!AbilitySystemComponentTags.HasAll(AllRequiredTags))
		{
			bMissing = true;
		}
	}

	if (SourceTags != nullptr)
	{
		if (SourceBlockedTags.Num() || SourceRequiredTags.Num())
		{
			if (SourceTags->HasAny(SourceBlockedTags))
			{
				bBlocked = true;
			}

			if (!SourceTags->HasAll(SourceRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (TargetTags != nullptr)
	{
		if (TargetBlockedTags.Num() || TargetRequiredTags.Num())
		{
			if (TargetTags->HasAny(TargetBlockedTags))
			{
				bBlocked = true;
			}

			if (!TargetTags->HasAll(TargetRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (bBlocked)
	{
		if (OptionalRelevantTags && BlockedTag.IsValid())
		{
			OptionalRelevantTags->AddTag(BlockedTag);
		}
		return false;
	}
	if (bMissing)
	{
		if (OptionalRelevantTags && MissingTag.IsValid())
		{
			OptionalRelevantTags->AddTag(MissingTag);
		}
		return false;
	}

	return true;
}

bool UGASCourseGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	OUT FGameplayTagContainer* OptionalRelevantTags)
{
	bool bCanCommit = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
	OnAbilityCommitDelegate.Broadcast(bCanCommit);
	return bCanCommit;
}

void UGASCourseGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	if (!AbilitySystemGlobals.ShouldIgnoreCooldowns())
	{
		check(ActorInfo);
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

		if(ActorInfo->IsNetAuthority() || HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
		{
			if (const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect())
			{
				const FActiveGameplayEffectHandle CooldownActiveGEHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, GetAbilityLevel(Handle, ActorInfo));
				if(CooldownActiveGEHandle.WasSuccessfullyApplied())
				{
					if(UGASCourseAbilitySystemComponent* ASC = GetGASCourseAbilitySystemComponentFromActorInfo())
					{
						ASC->WaitForAbilityCooldownEnd(const_cast<UGASCourseGameplayAbility*>(this), CooldownActiveGEHandle);
					}
				}
			}
		}
	}
}

bool UGASCourseGameplayAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ensure(ActorInfo))
	{
		return true;
	}

	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (CooldownTags && !CooldownTags->IsEmpty())
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get())
		{
			if (AbilitySystemComponent->HasAnyMatchingGameplayTags(*CooldownTags))
			{
				if (OptionalRelevantTags)
				{
					const FGameplayTag& FailCooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
					if (FailCooldownTag.IsValid())
					{
						FGameplayTag EventTag;
						EventTag = Event_AbilityActivation_Fail;
						
						FGameplayEventData Payload;
						Payload.Instigator = GetAvatarActorFromActorInfo();
						Payload.OptionalObject = this;
						Payload.EventTag = AbilityActivationFail_OnCooldown;
						Payload.Target = GetAvatarActorFromActorInfo();
						
						OptionalRelevantTags->AddTag(FailCooldownTag);
						AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);

						//Haptic feedback for failure.
						InvokeAbilityFailHapticFeedback();
					}

					// Let the caller know which tags were blocking
					OptionalRelevantTags->AppendMatchingTags(AbilitySystemComponent->GetOwnedGameplayTags(), *CooldownTags);
				}

				return false;
			}
		}
	}
	return true;
}

void UGASCourseGameplayAbility::GetAbilityCooldownTags(FGameplayTagContainer& CooldownTags) const
{
	CooldownTags.Reset();
	if(const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect())
	{
		CooldownTags.AppendTags(CooldownGE->GetGrantedTags());
		
	}
}

void UGASCourseGameplayAbility::GetAbilityDurationTags(FGameplayTagContainer& DurationTags) const
{
	DurationTags.Reset();
	if (DurationEffect)
	{
		DurationTags.AppendTags(DurationEffect.GetDefaultObject()->GetGrantedTags());
	}
}

float UGASCourseGameplayAbility::GetGrantedbyEffectDuration() const
{
	check(CurrentActorInfo);
	if (CurrentActorInfo)
	{
		const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo_Ensured();
		const FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->FindActiveGameplayEffectHandle(GetCurrentAbilitySpecHandle());
		if (ActiveHandle.IsValid())
		{
			return AbilitySystemComponent->GetGameplayEffectDuration(ActiveHandle);
		}
	}
	return 0.0f;
}

void UGASCourseGameplayAbility::OnPawnAvatarSet()
{
}

void UGASCourseGameplayAbility::GetStackedAbilityDurationTags(FGameplayTagContainer& DurationTags) const
{
	if (!bHasAbilityStacks)
	{
		return;
	}
	if (!StackDurationEffect.Get())
	{
		return;
	}
	DurationTags.Reset();
	
	if (const UGameplayEffect* StackDurationGE = StackDurationEffect->GetDefaultObject<UGameplayEffect>())
	{
		DurationTags.AppendTags(StackDurationGE->GetGrantedTags());
	}
}

void UGASCourseGameplayAbility::ApplyAbilityParamsFromSourceObject(const FGameplayAbilitySpec& Spec)
{
	const UGASC_AbilityParamsObject* ParamsObject = Cast<UGASC_AbilityParamsObject>(Spec.SourceObject);
	if (!ParamsObject)
	{
		return;
	}

	// Never write into the class default object. Both callers can reach this on the CDO: GAS
	// dispatches OnGiveAbility through Spec.Ability whenever the spec has no primary instance, and
	// a NonInstanced ability has no instance to run on at all. Copying there would rewrite the
	// class defaults for every future grant and every later-spawned instance of the class, and
	// would outlive the PIE session that did it. This is the single guard for every caller.
	if (!IsInstantiated())
	{
		// IsInstantiated means "I am not the CDO", which is not the same question as whether the
		// class is instanced at all, so the two cases need telling apart before shouting about it.
		// An instanced policy reaching here is transient and self-correcting - OnGiveAbility always
		// routes through the CDO for InstancedPerExecution, and does so on a client whose replicated
		// per-actor instance has not arrived yet - and PreActivate applies the parameters to the real
		// instance either way. Only a policy with no instance at all can never be fixed.
		const EGameplayAbilityInstancingPolicy::Type Policy = GetInstancingPolicy();
		const bool bWillBeAppliedLater = Policy == EGameplayAbilityInstancingPolicy::InstancedPerActor
			|| Policy == EGameplayAbilityInstancingPolicy::InstancedPerExecution;

		if (bWillBeAppliedLater)
		{
			UE_LOG(LOG_GASC_GameplayAbility, Verbose,
				TEXT("'%s' was reached on its class default object, so its granted parameters were not applied here; ")
				TEXT("PreActivate will apply them to the live instance."),
				*GetClass()->GetName());
		}
		else
		{
			UE_LOG(LOG_GASC_GameplayAbility, Warning,
				TEXT("'%s' was granted with ability parameters attached but is never instanced, so they cannot be applied. ")
				TEXT("Set its Instancing Policy to Instanced Per Actor or Instanced Per Execution to use granted parameters."),
				*GetClass()->GetName());
		}
		return;
	}

	// The copy itself lives on the params object, so the per-property type checking and reporting is
	// shared by everything that grants an ability with parameters attached.
	ParamsObject->ApplyTo(this);
}

void UGASCourseGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);

	// Super::PreActivate is what sets CurrentSpecHandle and the current source object, so the spec
	// is only resolvable from here on. Re-applying is cheap and idempotent, and for an
	// InstancedPerExecution ability this is the only place the parameters ever get applied at all,
	// since each execution runs on a freshly constructed instance.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			ApplyAbilityParamsFromSourceObject(*Spec);
		}
	}
}

void UGASCourseGameplayAbility::ConfigureAbilityFromGrantedSpec(const FGameplayAbilitySpec& Spec)
{
	UGASCourseGameplayAbility* TemplateAbility = Cast<UGASCourseGameplayAbility>(Spec.Ability.Get());
	if (!TemplateAbility)
	{
		UE_LOGFMT(LogTemp, Warning, "Invalid ability provided in spec!");
		return;
	}
	if (UGASCourseAbilitySystemComponent* ASC = GetGASCourseAbilitySystemComponentFromActorInfo())
	{
		if (const FGrantedCardAbilityConfig* CardAbilityConfig = ASC->CardAbilityConfigHandles.Find(Spec.Handle))
		{
			AbilityType = CardAbilityConfig->AbilityType;
			if (AbilityType == EGASCourseAbilityType::Duration)
			{
				ConfigureAbilityDuration(*CardAbilityConfig);
			}
		}
	}
}

void UGASCourseGameplayAbility::ConfigureAbilityDuration(const FGrantedCardAbilityConfig& CardAbilityConfig)
{
	DurationEffect = CardAbilityConfig.DurationEffect;
	bAutoApplyDurationEffect = true;
	bAutoEndAbilityOnDurationEnd = true;
	bAutoCommitAbilityOnActivate = false;
	bAutoCommitCooldownOnDurationEnd = true;
}

bool UGASCourseGameplayAbility::ValidateComponents(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return ResolveComponentProperties(ActorInfo);
}

bool UGASCourseGameplayAbility::ResolveComponentProperties(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return false;
	}

	if (!bComponentPropertyNamesBuilt)
	{
		BuildComponentPropertyNames();
	}

	if (ComponentPropertyNames.IsEmpty())
	{
		return true;
	}

	// Search order: owning actor, avatar actor, then the owning controller.
	AActor* SearchActors[] =
	{
		ActorInfo->OwnerActor.Get(),
		ActorInfo->AvatarActor.Get(),
		ActorInfo->PlayerController.Get()
	};

	const UClass* AbilityClass = GetClass();
	bool bAllResolved = true;

	for (const FName PropertyName : ComponentPropertyNames)
	{
		const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(AbilityClass->FindPropertyByName(PropertyName));
		if (!ObjectProperty || !ObjectProperty->PropertyClass)
		{
			continue;
		}

		UActorComponent* FoundComponent = nullptr;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(SearchActors) && !FoundComponent; ++Index)
		{
			AActor* SearchActor = SearchActors[Index];
			if (!SearchActor)
			{
				continue;
			}

			// Owner and avatar are frequently the same actor - skip the duplicate search.
			bool bAlreadySearched = false;
			for (int32 Previous = 0; Previous < Index; ++Previous)
			{
				bAlreadySearched |= (SearchActors[Previous] == SearchActor);
			}

			if (!bAlreadySearched)
			{
				FoundComponent = SearchActor->FindComponentByClass(ObjectProperty->PropertyClass);
			}
		}

		if (!FoundComponent)
		{
			bAllResolved = false;
			UE_LOG(LOG_GASC_GameplayAbility, Warning,
				TEXT("%s: no %s found on the owner, avatar or controller for variable '%s'."),
				*GetName(), *ObjectProperty->PropertyClass->GetName(), *PropertyName.ToString());
			continue;
		}
		const UObject* ConstTargetObject = Cast<UObject>(this);
		if (UObject* TargetObject = const_cast<UObject*>(ConstTargetObject))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(TargetObject, FoundComponent);
		}
	}

	return bAllResolved;
}

void UGASCourseGameplayAbility::BuildComponentPropertyNames() const
{
	ComponentPropertyNames.Reset();
	bComponentPropertyNamesBuilt = true;

	// GetClass(), not StaticClass() - variables added in a Blueprint child live on the generated class.
	for (TFieldIterator<FObjectProperty> It(GetClass(), EFieldIterationFlags::IncludeSuper); It; ++It)
	{
		const FObjectProperty* ObjectProperty = *It;
		UClass* PropertyClass = ObjectProperty->PropertyClass;
		// The component test: an object reference whose pointed-to class derives from UActorComponent.
		if (!PropertyClass || !PropertyClass->GetSuperClass()->IsChildOf(UActorComponent::StaticClass())|| PropertyClass->IsChildOf(UPrimitiveComponent::StaticClass()))
		{
			continue;
		}

#if WITH_EDITOR
		// Category metadata is stripped from cooked builds, so it can only drive a warning, never the resolve.
		if (FObjectEditorUtils::GetCategoryFName(ObjectProperty) != GASCourseAbilityComponents::ComponentsCategory)
		{
			UE_LOG(LOG_GASC_GameplayAbility, Warning,
				TEXT("%s: component variable '%s' is outside the '%s' category but will still be auto-resolved."),
				*GetName(), *ObjectProperty->GetName(), *GASCourseAbilityComponents::ComponentsCategory.ToString());
		}
#endif

		ComponentPropertyNames.Add(ObjectProperty->GetFName());
	}
}

FVector UGASCourseGameplayAbility::GetInputDirection() const
{
	const AGASCoursePlayerCharacter* OwningPlayerCharacter = GetGASCouresPlayerCharacterFromActorInfo();
	if (!OwningPlayerCharacter)
	{
		return FVector::ZeroVector;
	}
	const UCharacterMovementComponent* CharacterMovementComponent = Cast<UCharacterMovementComponent>(GetActorInfo().MovementComponent);
	if (!CharacterMovementComponent)
	{
		return FVector::ZeroVector;
	}

	UGASC_InputBufferComponent* InputBufferComponent = Cast<UGASC_InputBufferComponent>(GetAvatarActorFromActorInfo()->GetComponentByClass<UGASC_InputBufferComponent>());
	if (!InputBufferComponent)
	{
		if (CharacterMovementComponent->GetLastInputVector().GetSafeNormal().IsNearlyZero())
		{
			return FVector::ZeroVector;
		}
	}

	FVector BufferedMovementInput = InputBufferComponent->GetCachedMovementInputVector(true);
	if (CharacterMovementComponent->GetLastInputVector().GetSafeNormal().IsNearlyZero())
	{
		if (BufferedMovementInput.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}
		return BufferedMovementInput.GetSafeNormal();
	}
	
	return CharacterMovementComponent->GetLastInputVector().GetSafeNormal();
}

void UGASCourseGameplayAbility::DurationEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo)
{
	// Clear stored handle
	if (DurationEffectRemovedDelegateHandle.IsValid())
	{
		DurationEffectRemovedDelegateHandle.Reset();
	}

	if(HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		if(bAutoCommitCooldownOnDurationEnd)
		{
			CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		}
		
		if(bAutoEndAbilityOnDurationEnd)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
}

bool UGASCourseGameplayAbility::HasStacksAvailable()
{
	if (UGASCourseAbilitySystemComponent* ASC = GetGASCourseAbilitySystemComponentFromActorInfo())
	{
		TArray<FGameplayAbilitySpecHandle> ActiveCardHandles;
		ASC->CardAbilityConfigHandles.GetKeys(ActiveCardHandles);
		for (const FGameplayAbilitySpecHandle Handle : ActiveCardHandles)
		{
			if (const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(Handle))
			{
				if (AbilitySpec->Ability && AbilitySpec->Ability->GetClass() == GetClass())
				{
					int32 StackCount = ASC->CardAbilityConfigHandles.Find(Handle)->AbilityStackCount;
					return StackCount > 1;
				}
			}
		}
	}
	
	return false;
}

bool UGASCourseGameplayAbility::ApplyDurationEffect()
{
	bool bSuccess = false;
	
	int32 DurationEffectCount = GetAbilitySystemComponentFromActorInfo()->GetGameplayEffectCount(DurationEffect, 
		nullptr);
	if (DurationEffectCount > 0)
	{
		return true;
	}

	if (DurationEffect && HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddInstigator(ASC->GetOwnerActor(), ASC->GetOwnerActor());
			DurationEffectHandle = ASC->ApplyGameplayEffectToSelf(DurationEffect.GetDefaultObject(), GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo), 
				ContextHandle);
			
			if(DurationEffectHandle.WasSuccessfullyApplied())
			{
				bSuccess = true;
				if (FOnActiveGameplayEffectRemoved_Info* RemoveDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(DurationEffectHandle))
				{
					DurationEffectRemovedDelegateHandle = RemoveDelegate->AddUObject(this, &UGASCourseGameplayAbility::DurationEffectRemoved);
				}
			}
			return bSuccess;
		}
	}
	
	return bSuccess;
}

void UGASCourseGameplayAbility::OnAbilityInputPressed(float InTimeWaited)
{
	if(HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		if(bAutoCommitCooldownOnDurationEnd || bAutoEndAbilityOnDurationEnd)
		{
			BP_RemoveGameplayEffectFromOwnerWithHandle(DurationEffectHandle);
		}
		else
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
}

void UGASCourseGameplayAbility::InvokeAbilityFailHapticFeedback() const
{
	if (const UGASC_AbilitySystemSettings* AbilitySystemSettings = GetDefault<UGASC_AbilitySystemSettings>())
	{
		if (AGASCoursePlayerController* PC = GetGASCoursePlayerControllerFromActorInfo())
		{
			if (PC->IsLocalPlayerController())
			{
				UForceFeedbackEffect* ForceFeedbackEffect = AbilitySystemSettings->HapticFeedback_AbilityActivationFail.Get();
				PC->ClientPlayForceFeedback(ForceFeedbackEffect);
			}
		}
	}
}

void UGASCourseGameplayAbility::OnHitApplied(const FHitContext& HitContext)
{
	if (HitContext.HitInstigator.Get() == GetAvatarActorFromActorInfo())
	{
		OnHitApplied_Event(HitContext);
	}
}

void UGASCourseGameplayAbility::OnHitReceived(const FHitContext& HitContext)
{
	if (HitContext.HitTarget.Get() == GetAvatarActorFromActorInfo())
	{
		OnHitReceived_Event(HitContext);
	}
}

void UGASCourseGameplayAbility::OnDamageApplied(const FResourceModificationContext& ResourceModificationContext)
{
	if (ResourceModificationContext.HitContext.HitInstigator == GetAvatarActorFromActorInfo())
	{
		OnDamageApplied_Event(ResourceModificationContext);
	}
}
