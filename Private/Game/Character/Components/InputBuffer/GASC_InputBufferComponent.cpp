// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "GASCourse/GASCourseCharacter.h"

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
	MovementInputAxis = FVector2D::ZeroVector;
	BufferedMovementInputAxis = FVector2D::ZeroVector;
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


bool UGASC_InputBufferComponent::QueueStateTreeEventOncePerActionPerFrame(const UInputAction* Action, const FGameplayTag& Tag)
{
	if (!Action || !Tag.IsValid())
		return false;

	const uint64 Frame = GFrameCounter;

	// Optional: if buffer is open OR opened this frame, block
	if (IsInputBufferOpen() || StateTreeQueue.bOpenedThisFrame)
		return false;

	// Once per action per frame
	const TObjectPtr<const UInputAction> Key(Action);
	uint64& LastFrame = StateTreeQueue.LastAcceptedFrameByAction.FindOrAdd(Key);
	if (LastFrame == Frame)
		return false;
	LastFrame = Frame;

	// Optional: only allow one queued event per frame total
	if (StateTreeQueue.LastQueuedFrame == Frame)
		return false;

	// Only one pending event at a time
	if (StateTreeQueue.bHasPending)
		return false;

	StateTreeQueue.LastQueuedFrame = Frame;
	StateTreeQueue.bHasPending = true;
	StateTreeQueue.PendingTag = Tag;
	return true;
}

bool UGASC_InputBufferComponent::ConsumeQueuedStateTreeEvent(FGameplayTag& OutTag)
{
	if (!StateTreeQueue.bHasPending || !StateTreeQueue.PendingTag.IsValid())
		return false;

	OutTag = StateTreeQueue.PendingTag;
	StateTreeQueue.PendingTag = FGameplayTag(); // clear [5](https://forums.unrealengine.com/t/why-state-tree-on-state-completed-transition-does-not-work/2630123)
	StateTreeQueue.bHasPending = false;
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
				MovementInputAxis = Value.Get<FVector2D>();
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

void UGASC_InputBufferComponent::OpenInputBuffer_Implementation()
{
	UE_LOG(LOG_GASC_InputBufferComponent, Log, TEXT("Input Buffer Open: %s"), *InputBufferComponentName);
	
	bInputBufferOpen = true;
	OnInputBufferOpenedEvent.Broadcast();
}

void UGASC_InputBufferComponent::CloseInputBuffer_Implementation()
{
	UE_LOG(LOG_GASC_InputBufferComponent, Log, TEXT("Input Buffer Closed: %s"), *InputBufferComponentName);

	bInputBufferOpen = false;
	ActivateBufferedInputAbility();
	FlushInputBuffer();
	FlushCachedMovementInputAxisValue();
	OnInputBufferClosedEvent.Broadcast();
}

bool UGASC_InputBufferComponent::FlushInputBuffer()
{
	UE_LOG(LOG_GASC_InputBufferComponent, Log, TEXT("Input Buffer Flushed: %s"), *InputBufferComponentName);

	BufferedInputActions.Empty();
	OnInputBufferFlushedEvent.Broadcast();
	return true;
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
				UE_LOG(
					LOG_GASC_InputBufferComponent,
					Log,
					TEXT("Input Action Simulated: %s, %s"),
					*InputActionToSimulate->GetName(),
					*InputBufferComponentName
				);

				SimulateInputAction(InputActionToSimulate);
				OnInputBufferedConsumedEvent.Broadcast(InputActionToSimulate);
			}
		}
	}
}

void UGASC_InputBufferComponent::AddInputActionToBuffer(UInputAction* InAction)
{
	if (InAction && IsInputBufferOpen())
	{
		UE_LOG(
			LOG_GASC_InputBufferComponent,
			Log,
			TEXT("Input Action Added to Buffer: %s, %s"),
			*InAction->GetName(),
			*InputBufferComponentName
		);

		BufferedInputActions.AddUnique(InAction);
		BufferedMovementInputAxis = MovementInputAxis;

		UE_LOGFMT(
			LOG_GASC_InputBufferComponent,
			Warning,
			"BufferedMovementInputAxis: {0}, {1}",
			BufferedMovementInputAxis.ToString(),
			InputBufferComponentName
		);
	}
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

		UE_LOG(
			LOG_GASC_InputBufferComponent,
			Log,
			TEXT("Input Action Simulated: %s, %s"),
			*InputAction->GetName(),
			*InputBufferComponentName
		);
	}
}

void UGASC_InputBufferComponent::FlushCachedMovementInputAxisValue()
{
	BufferedMovementInputAxis = FVector2D::ZeroVector;
	MovementInputAxis = FVector2D::ZeroVector;
}

FVector2D UGASC_InputBufferComponent::GetMovementInputAxisValue()
{
	const FVector2D MovementInputAxisValue = BufferedMovementInputAxis;
	UE_LOG(LogTemp, Warning, TEXT("INPUT BUFFER: Buffered Movement Input: %s"), *MovementInputAxisValue.ToString());
	FlushCachedMovementInputAxisValue();
	return MovementInputAxisValue;
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
