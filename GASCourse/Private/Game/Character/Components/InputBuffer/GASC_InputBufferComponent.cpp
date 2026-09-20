// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "GASCourse/GASCourseCharacter.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY(LOG_GASC_InputBufferComponent);

UGASC_InputBufferComponent::UGASC_InputBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UGASC_InputBufferComponent::InitializeComponent()
{
	Super::InitializeComponent();
	InputBufferComponentName = GetPathNameSafe(this);
	bBindingsRegistered = false;
}

void UGASC_InputBufferComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGASC_InputBufferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	RemoveBindings();
}

void UGASC_InputBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Clear the one-frame latch
	StateTreeQueue.bOpenedThisFrame = false;
}


bool UGASC_InputBufferComponent::QueueStateTreeEventOncePerActionPerFrame(const UInputAction* Action, const FGameplayTag& Tag, ETriggerEvent TriggerEvent, const FInputActionValue& InputActionValue, bool bWasBufferedAction)
{
	if (!Action || !Tag.IsValid())
		return false;
	
	FGASC_STInputEventPayload PendingInputEventPayload;

	const uint64 Frame = GFrameCounter;
	
	FGameplayTag InputCategoryTag = FindCategoryTagForInputAction(Action);
	bool bIsInputCategoryBlockingInput	= IsInputBlockedForCategory(InputCategoryTag);
	bool bIsInputCategoryBufferOpen		= IsInputBufferOpenForCategory(InputCategoryTag);
	
	if (bIsInputCategoryBlockingInput && !bIsInputCategoryBufferOpen && TriggerEvent == ETriggerEvent::Triggered)
	{
		UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Input attempted to be buffered via state tree but %s category buffer is not open and is blocked."), 
			*Action->GetName(), *InputCategoryTag.ToString());
		return false;
	}
	
	// Once per action per frame
	const TObjectPtr<const UInputAction> Key(Action);
	uint64& LastFrame = StateTreeQueue.LastAcceptedFrameByAction.FindOrAdd(Key);
	if (LastFrame == Frame)
		return false;
	LastFrame = Frame;

	// Optional: only allow one queued event per frame total
	if (StateTreeQueue.LastQueuedFrame == Frame)
		return false;
	
	FVector MovementInputDirection = CachedMovementInputVector;
	if (OwningCharacter)
	{
		MovementInputDirection = OwningCharacter->GetLastMovementInputVector();
		CachedMovementInputVector = MovementInputDirection;
	}
	else
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose,
			TEXT("QueueStateTreeEventOncePerActionPerFrame: OwningCharacter null, using cached direction: %s"),
			*InputBufferComponentName);
	}
	
	PendingInputEventPayload.InputAction = const_cast<UInputAction*>(Action);
	PendingInputEventPayload.InputActionValue = InputActionValue;
	PendingInputEventPayload.bWasBufferedAction = bWasBufferedAction;
	PendingInputEventPayload.InputDirection = MovementInputDirection;
	
	UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Queued state tree input event: Input Action Value: %s, Was Buffered: %s, Cached Input Direction: %s"), 
		*Action->GetName(), *InputActionValue.ToString(), bWasBufferedAction ? TEXT("true") : TEXT("false"), *MovementInputDirection.ToString());

	StateTreeQueue.LastQueuedFrame = Frame;
	StateTreeQueue.bHasPending = true;
	StateTreeQueue.PendingTag = Tag;
	StateTreeQueue.InputEventPayload = PendingInputEventPayload;
	
	//InputBufferTimeoutFlush();
	
	return true;
}

bool UGASC_InputBufferComponent::ConsumeQueuedStateTreeEvent(FGameplayTag& OutTag, FGASC_STInputEventPayload& OutPayload)
{
	// Guard before reading the payload - the category below is inferred from it, and a consumed or
	// never-populated queue would infer from a stale action and still report success.
	if (!HasPendingInputBufferEvent())
	{
		return false;
	}

	const FGameplayTag InputCategoryTag = FindCategoryTagForInputAction(StateTreeQueue.InputEventPayload.InputAction);
	if (IsInputBufferOpenForCategory(InputCategoryTag) || IsInputBlockedForCategory(InputCategoryTag))
	{
		return false;
	}

	OutTag = StateTreeQueue.PendingTag;
	OutPayload = StateTreeQueue.InputEventPayload;
	FlushInputBuffer(true);
	return true;
}

void UGASC_InputBufferComponent::ResetStateTreeEventQueue()
{
	StateTreeQueue.Reset();
}

void UGASC_InputBufferComponent::MarkInputBufferOpenedThisFrame()
{
	// Call this when your animation track opens the buffer on frame 0
	StateTreeQueue.bOpenedThisFrame = true;
	StateTreeQueue.LastQueuedFrame = GFrameCounter; // block same-frame queue
}

bool UGASC_InputBufferComponent::ResolveOwnerObjects()
{
	OwningCharacter = Cast<AGASCourseCharacter>(GetOwner());
	if (!OwningCharacter)
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ResolveOwnerObjects: OwningCharacter null: %s"), *InputBufferComponentName);
		return false;
	}

	OwningPlayerController = Cast<AGASCoursePlayerController>(OwningCharacter->GetController());
	if (!OwningPlayerController)
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ResolveOwnerObjects: OwningPlayerController null: %s"), *InputBufferComponentName);
		return false;
	}

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(OwningPlayerController->InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ResolveOwnerObjects: EnhancedInputComponent null: %s"), *InputBufferComponentName);
		return false;
	}
	
	PlayerInput = Cast<UEnhancedPlayerInput>(OwningPlayerController->PlayerInput);
	if (!PlayerInput)
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ResolveOwnerObjects: PlayerInput null: %s"), *InputBufferComponentName);
		return false;
	}

	return true;
}

void UGASC_InputBufferComponent::TryInitializeBindings()
{
	if (bBindingsRegistered)
	{
		return;
	}

	if (!ResolveOwnerObjects())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UGASC_InputBufferComponent::TryInitializeBindings)
			);
		}
		return;
	}

	ListenToInputActions();
}

void UGASC_InputBufferComponent::ListenToInputActions()
{
	if (!EnhancedInputComponent || bBindingsRegistered)
	{
		return;
	}
	if (InputActionsToBuffer.IsEmpty())
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ListenToInputActions: Input Actions to Buffer is Empty: %s"), *InputBufferComponentName);
		return;
	}

	for (UInputAction* InputAction : InputActionsToBuffer)
	{
		if (!InputAction)
		{
			continue;
		}

		const FEnhancedInputActionEventBinding& TriggeredBinding =
			EnhancedInputComponent->BindActionValueLambda(
				InputAction,
				ETriggerEvent::Triggered,
				[this, InputAction](const FInputActionValue& Value)
				{
					AddInputActionToBuffer(InputAction);
				});

		BindingHandles.Add(TriggeredBinding.GetHandle());
	}
	
	if (!MovementInputActionToBuffer)
	{
		UE_LOG(LOG_GASC_InputBufferComponent, Verbose, TEXT("ListenToInputActions: MovementInputActionToBuffer is null: %s"), *InputBufferComponentName);
		return;	
	}

	const FEnhancedInputActionEventBinding& TriggeredBinding =
		EnhancedInputComponent->BindActionValueLambda(
			MovementInputActionToBuffer,
			ETriggerEvent::Triggered,
			[this](const FInputActionValue& Value)
			{
				FVector2D MovementInput = Value.Get<FVector2D>();
				CachedMovementInputVector = FVector(MovementInput.Y, MovementInput.X, 0.0f);
			});

	BindingHandles.Add(TriggeredBinding.GetHandle());
	bBindingsRegistered = true;

	UE_LOG(
		LOG_GASC_InputBufferComponent,
		Log,
		TEXT("Bindings %s for %s. BufferedActions=%d Movement=%s"),
		bBindingsRegistered ? TEXT("registered") : TEXT("not registered"),
		*InputBufferComponentName,
		InputActionsToBuffer.Num(),
		MovementInputActionToBuffer ? *MovementInputActionToBuffer->GetName() : TEXT("None")
	);
}

void UGASC_InputBufferComponent::RemoveBindings()
{
	if (EnhancedInputComponent)
	{
		for (const uint32 Handle : BindingHandles)
		{
			EnhancedInputComponent->RemoveActionEventBinding(Handle);
		}
	}

	BindingHandles.Empty();
	bBindingsRegistered = false;
}

void UGASC_InputBufferComponent::OpenInputBuffer_ForCategory_Implementation(const FGameplayTag& Category)
{
	BufferedInputActionsCategory.AddTag(Category);
	OnInputBufferOpenedEvent.Broadcast();
	
	UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Input Buffer Opened for category: %s"), *InputBufferComponentName, *Category.ToString());
}

void UGASC_InputBufferComponent::BlockInputCategoryFromBuffer_Implementation(const FGameplayTag& Category)
{
	OnInputBufferBlockInputEvent.Broadcast();
	BlockedInputActionsCategory.AddTag(Category);
	
	UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Input Buffer Blocked for category: %s"), *InputBufferComponentName, *Category.ToString());
}

void UGASC_InputBufferComponent::ReleaseBlockInputCategoryFromBuffer_Implementation(const FGameplayTag& Category)
{
	BlockedInputActionsCategory.RemoveTag(Category);
	UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Input Buffer Opened for category: %s"), *InputBufferComponentName, *Category.ToString());
}

void UGASC_InputBufferComponent::CloseInputBuffer_ForCategory_Implementation(const FGameplayTag& Category)
{
	BufferedInputActionsCategory.RemoveTag(Category);
	ActivateBufferedInputAbility();
	OnInputBufferClosedEvent.Broadcast();
}

bool UGASC_InputBufferComponent::FlushInputBuffer(bool bStateTreeFlush)
{
#if !UE_BUILD_SHIPPING
	FGASC_STInputEventPayload OutPayload = StateTreeQueue.InputEventPayload;
	UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("%s: Input Flushed state tree event: Input Action Value: %s, Was Buffered: %s, Cached Input Direction: %s"), 
	*GetNameSafe(OutPayload.InputAction), *OutPayload.InputActionValue.ToString(), OutPayload.bWasBufferedAction ? TEXT("true") : TEXT("false"), *OutPayload.InputDirection.ToString());
#endif
	
	if (bStateTreeFlush)
	{
		StateTreeQueue.Reset();
	}
	else
	{
		BufferedInputActions.Empty();
	}

	OnInputBufferFlushedEvent.Broadcast();
	return true;
}

bool UGASC_InputBufferComponent::IsInputBufferOpenForCategory(const FGameplayTag& Category) const
{
	for (const FGameplayTag& Tag : BufferedInputActionsCategory)
	{
		if (Category.MatchesTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool UGASC_InputBufferComponent::IsInputBlockedForCategory(const FGameplayTag& Category) const
{
	for (const FGameplayTag& Tag : BlockedInputActionsCategory)
	{
		if (Category.MatchesTag(Tag))
		{
			return true;
		}
	}

	return false;
}

FGameplayTag UGASC_InputBufferComponent::FindCategoryTagForInputAction(const UInputAction* InAction) const
{
	if (!IsValid(InAction))
	{
		return FGameplayTag();
	}
	
	for (const TPair<FGameplayTag, FGASC_BufferedInputActionsArray>& BufferedInputActionsPair : BufferedInputActionsByCategory)
	{
		if (BufferedInputActionsPair.Value.Actions.Contains(InAction))
		{
			return BufferedInputActionsPair.Key;
		}
	}

	return FGameplayTag();
}

void UGASC_InputBufferComponent::InputBufferTimeoutFlush()
{
	UWorld* World = GetWorld();
	ensure(World);
	
	World->GetTimerManager().ClearTimer(InputBufferOpenTimeoutHandle);
	
	World->GetTimerManager().SetTimer(
			InputBufferOpenTimeoutHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]
			{
				FlushInputBuffer(true);
			}),
			GetInputBufferTimeout(), false);
}

void UGASC_InputBufferComponent::ActivateBufferedInputAbility()
{
	if (!OwningCharacter)
	{
		return;
	}

	if (UGASCourseAbilitySystemComponent* ASC = OwningCharacter->GetAbilitySystemComponent())
	{
		if (BufferedInputActions.Num() > 0)
		{
			if (UInputAction* InputActionToSimulate = BufferedInputActions[0])
			{
				UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("Input Action Simulated: %s, %s"),
								*InputActionToSimulate->GetName(),
								*InputBufferComponentName);

				//SimulateInputAction(InputActionToSimulate);
				OnInputBufferedConsumedEvent.Broadcast(InputActionToSimulate);
				FlushInputBuffer(false);
			}
		}
	}
}

void UGASC_InputBufferComponent::AddInputActionToBuffer(UInputAction* InAction)
{
	FGameplayTag InputCategoryTag = FindCategoryTagForInputAction(InAction);	
	
	if (InAction && IsInputBufferOpenForCategory(InputCategoryTag))
	{
		UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("Input Action Added to Buffer: %s, %s"),
			*InAction->GetName(),
			*InputBufferComponentName);

		BufferedInputActions.AddUnique(InAction);
		InputBufferTimeoutFlush();
	}
}

FVector UGASC_InputBufferComponent::GetCachedMovementInputVector(bool bFlushMovementVector)
{
	FVector MovementVector = CachedMovementInputVector;
	if (bFlushMovementVector)
	{
		CachedMovementInputVector = FVector::ZeroVector;
	}
	return MovementVector;
}

void UGASC_InputBufferComponent::SimulateInputAction(const UInputAction* InputAction) const
{
	if (!OwningPlayerController || !InputAction)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayerController->GetLocalPlayer()))
	{
		const FInputActionValue Value(EInputActionValueType::Boolean, FVector(1.0f, 1.0f, 1.0f));
		const TArray<UInputModifier*> Modifiers;
		const TArray<UInputTrigger*> Triggers;

		Subsystem->InjectInputForAction(InputAction, Value, Modifiers, Triggers);
		
		UE_VLOG_UELOG(this, LOG_GASC_InputBufferComponent, Verbose, TEXT("Input Action Simulated: %s, %s"),
			*InputAction->GetName(),
			*InputBufferComponentName);
	}
}

TArray<UInputAction*> UGASC_InputBufferComponent::GetBufferedInputActions() const
{
	TArray<UInputAction*> Result;
	Result.Reserve(BufferedInputActions.Num());

	for (const TObjectPtr<UInputAction>& Action : BufferedInputActions)
	{
		Result.Add(Action.Get());
	}

	return Result;
}
