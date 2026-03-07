#include "Game/Card/BaseCardGameplayAbilitySet.h"
#include "Engine/AssetManager.h"
#include "Game/BlueprintLibraries/GameplayAbilitySystem/GASCourseASCBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Ability/GASC_AbilityDurationEffect.h"

void FGASCourseCardAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FGASCourseCardAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FGASCourseCardAbilitySet_GrantedHandles::AddAttributeSet(UGASCourseAttributeSet* Set)
{
	if (Set)
	{
		GrantedAttributeSets.Add(Set);
	}
}

void FGASCourseCardAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	if (!ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		if (Set)
		{
			ASC->RemoveSpawnedAttribute(Set);
		}
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

UBaseCardGameplayAbilitySet::UBaseCardGameplayAbilitySet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UBaseCardGameplayAbilitySet::ActivateCard(UAbilitySystemComponent* ASC, int32 CardLevel, bool bAutoLoadThenActivate )
{
	if (!ASC)
	{
		return false;
	}

	// Authority rule (match your TakeFromAbilitySystem)
	if (!ASC->IsOwnerActorAuthoritative())
	{
		return false;
	}
	
	//Store reference to passed in card level for use throughout class.
	CardLevel_Internal = CardLevel;
	
	FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles = new FGASCourseAbilitySet_GrantedHandles();

	if (bCardDataLoaded)
	{
		GiveToAbilitySystem(ASC, CardLevel_Internal, OutGrantedHandles, nullptr);
		return true;
	}

	if (!bAutoLoadThenActivate)
	{
		// Caller chose to not auto-load; safe no-op
		return false;
	}

	// Queue activation, then start load
	FPendingActivation Pending;
	Pending.ASC = ASC;
	Pending.Handles = OutGrantedHandles;
	Pending.SourceObject = nullptr;
	PendingActivations.Add(Pending);

	LoadCardDataAsync();
	return false; // not activated yet, will activate after load completes
}

void UBaseCardGameplayAbilitySet::LoadCardDataAsync()
{
	if (bCardDataLoaded)
	{
		OnCardDataLoaded.Broadcast(true);
		return;
	}

	if (bCardDataLoading)
	{
		return; // already in-flight
	}

	StartAsyncLoad_Internal();
}

void UBaseCardGameplayAbilitySet::OverrideCardLevel(float InCardLevelOverride)
{
	CardLevel_Internal = FMath::Clamp(InCardLevelOverride, 0.0f, 5.0f);
}

void UBaseCardGameplayAbilitySet::StartAsyncLoad_Internal()
{
	bCardDataLoading = true;

	LoadedAbilities.Reset();
	LoadedGameplayEffects.Reset();

	TArray<FSoftObjectPath> PathsToLoad;
	PathsToLoad.Reserve(GrantedGameplayAbilities.Num() + GrantedGameplayEffects.Num());

	// Collect ability paths
	for (int32 i = 0; i < GrantedGameplayAbilities.Num(); ++i)
	{
		const auto& Entry = GrantedGameplayAbilities[i];
		if (!Entry.GameplayAbility.IsNull())
		{
			PathsToLoad.AddUnique(Entry.GameplayAbility.ToSoftObjectPath());
		}
	}

	// Collect effect paths
	for (int32 i = 0; i < GrantedGameplayEffects.Num(); ++i)
	{
		const auto& Entry = GrantedGameplayEffects[i];
		if (!Entry.GameplayEffect.IsNull())
		{
			PathsToLoad.AddUnique(Entry.GameplayEffect.ToSoftObjectPath());
		}
	}

	// Nothing to load -> build caches synchronously and complete
	if (PathsToLoad.IsEmpty())
	{
		BuildLoadedCaches_FromSoftRefs();
		OnAsyncLoadComplete_Internal(true);
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	CardDataLoadHandle = Streamable.RequestAsyncLoad(
		PathsToLoad,
		FStreamableDelegate::CreateWeakLambda(this, [this]()
		{
			// If this asset is gone, weak lambda won't execute
			BuildLoadedCaches_FromSoftRefs();
			OnAsyncLoadComplete_Internal(true);
		}),
		FStreamableManager::AsyncLoadHighPriority
	);

	// If RequestAsyncLoad failed to return a handle, fail gracefully
	if (!CardDataLoadHandle.IsValid())
	{
		BuildLoadedCaches_FromSoftRefs();
		OnAsyncLoadComplete_Internal(false);
	}
}

void UBaseCardGameplayAbilitySet::BuildLoadedCaches_FromSoftRefs()
{
	// Abilities (preserve source index)
	for (int32 i = 0; i < GrantedGameplayAbilities.Num(); ++i)
	{
		const auto& Entry = GrantedGameplayAbilities[i];
		if (Entry.GameplayAbility.IsNull())
		{
			continue;
		}

		UClass* Loaded = Entry.GameplayAbility.Get();
		if (TSubclassOf<UGASCourseGameplayAbility> Typed = Loaded)
		{
			FLoadedAbility LA;
			LA.SourceIndex = i;
			LA.GameplayAbilityClass = Typed;
			LoadedAbilities.Add(LA);
		}
	}

	// Effects (preserve source index)
	for (int32 i = 0; i < GrantedGameplayEffects.Num(); ++i)
	{
		const auto& Entry = GrantedGameplayEffects[i];
		if (Entry.GameplayEffect.IsNull())
		{
			continue;
		}

		UClass* Loaded = Entry.GameplayEffect.Get();
		TSubclassOf<UGameplayEffect> Typed = Loaded;

		if (Typed)
		{
			FLoadedGameplayEffect LE;
			LE.SourceIndex = i;
			LE.GameplayEffectClass = Typed;
			LoadedGameplayEffects.Add(LE);
		}
	}
}

void UBaseCardGameplayAbilitySet::OnAsyncLoadComplete_Internal(bool bSuccess)
{
	bCardDataLoading = false;
	bCardDataLoaded = bSuccess;

	if (CardDataLoadHandle.IsValid())
	{
		CardDataLoadHandle.Reset();
	}

	OnCardDataLoaded.Broadcast(bSuccess);

	if (bSuccess)
	{
		FlushPendingActivations();
	}
	else
	{
		// Clear pending activations on failure to avoid stale pointers
		PendingActivations.Reset();
	}
}

void UBaseCardGameplayAbilitySet::FlushPendingActivations()
{
	// Activate any queued requests
	for (int32 i = PendingActivations.Num() - 1; i >= 0; --i)
	{
		FPendingActivation& P = PendingActivations[i];

		UAbilitySystemComponent* ASC = P.ASC.Get();
		UObject* SourceObj = P.SourceObject.Get();

		if (!ASC || !ASC->IsOwnerActorAuthoritative())
		{
			PendingActivations.RemoveAtSwap(i);
			continue;
		}

		GiveToAbilitySystem(ASC, CardLevel_Internal, P.Handles, SourceObj);
		PendingActivations.RemoveAtSwap(i);
	}
}

void UBaseCardGameplayAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, int32 CardLevel,
                                                     FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles,
                                                     UObject* SourceObject) const
{
	check(ASC);

	// Prevent “activate while loading” crashes / partial grants
	if (!bCardDataLoaded)
	{
		return;
	}

	if (!ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// Attribute sets (sync, no soft refs here)
	for (const FGASCourseCardAbilitySet_AttributeSet& SetToGrant : GrantedAttributes)
	{
		if (!IsValid(SetToGrant.AttributeSet))
		{
			continue;
		}

		UGASCourseAttributeSet* NewSet = NewObject<UGASCourseAttributeSet>(ASC->GetOwner(), SetToGrant.AttributeSet);
		ASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}

	// Gameplay abilities (use preserved source index, DO NOT mutate CDO)
	for (const FLoadedAbility& Loaded : LoadedAbilities)
	{
		if (!GrantedGameplayAbilities.IsValidIndex(Loaded.SourceIndex))
		{
			continue;
		}

		const FGASCourseCardAbilitySet_GameplayAbility& AbilityEntry = GrantedGameplayAbilities[Loaded.SourceIndex];
		const TSubclassOf<UGASCourseGameplayAbility> AbilityClass = Loaded.GameplayAbilityClass;

		if (!AbilityClass)
		{
			continue;
		}

		// Choose slot tag
		const FGameplayTag ActiveSlotTag = UGASCourseASCBlueprintLibrary::GetNextAvailableActiveAbilitySlot(ASC);

		FGameplayAbilitySpec Spec(AbilityClass);
		FGrantedCardAbilityConfig CardAbilityConfig;
		
		CardAbilityConfig.SlotType = AbilityEntry.AbilitySlotType;
		if (CardLevel >= CardDurationProperties.DurationEffectAppliedAtLevelMin && CardDurationProperties.DurationEffect)
		{
			CardAbilityConfig.DurationEffect = CardDurationProperties.DurationEffect;
			CardAbilityConfig.AbilityType = EGASCourseAbilityType::Duration;
		}
		Spec.Level = CardLevel;
		
		// Store slot tag in the spec (safe)
		Spec.GetDynamicSpecSourceTags().AddTag(ActiveSlotTag);
		if (UGASCourseAbilitySystemComponent* OwningASC = Cast<UGASCourseAbilitySystemComponent>(ASC))
		{
			OwningASC->CardAbilityConfigHandles.Add(Spec.Handle, CardAbilityConfig);
			
		}
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(Handle);
		}
	}

	// Gameplay effects
	for (const FLoadedGameplayEffect& Loaded : LoadedGameplayEffects)
	{
		const TSubclassOf<UGameplayEffect> EffectClass = Loaded.GameplayEffectClass;
		if (!EffectClass)
		{
			continue;
		}

		const UGameplayEffect* GECDO = EffectClass.GetDefaultObject();
		if (!IsValid(GECDO))
		{
			continue;
		}

		const FActiveGameplayEffectHandle Handle =
			ASC->ApplyGameplayEffectToSelf(GECDO, CardLevel, ASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(Handle);
		}
	}
}