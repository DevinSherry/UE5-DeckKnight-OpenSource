// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Components/ActorComponent.h"
#include "GASC_InputBufferComponent.generated.h"

class UEnhancedPlayerInput;
enum class ETriggerEvent : uint8;
class UEnhancedInputComponent;
class UInputAction;
class UGASC_InputBuffer_Settings;
class AGASCourseCharacter;
class AGASCoursePlayerController;
class UGASCourseAbilitySystemComponent;

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_InputBufferComponent, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInputBufferOpenedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInputBufferClosedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInputBufferFlushedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInputBufferBlockInputEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputBufferedConsumedEvent, UInputAction*, InputAction);

USTRUCT(Blueprintable)
struct FGASC_BufferedInputActionsArray
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Input")
	TArray<TObjectPtr<UInputAction>> Actions;
};

USTRUCT(Blueprintable)
struct FGASC_STInputEventPayload
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Input")
	TObjectPtr<UInputAction> InputAction = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Input")
	bool bWasBufferedAction = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Input")
	FInputActionValue InputActionValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Input")
	FVector InputDirection = FVector::ZeroVector;
};

USTRUCT()
struct FGASC_StateTreeEventQueue
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	bool bHasPending = false;

	UPROPERTY(Transient)
	FGameplayTag PendingTag;

	// Per action, last frame we accepted an event (once-per-action-per-frame)
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UInputAction>, uint64> LastAcceptedFrameByAction;

	// One pending event per frame (optional)
	UPROPERTY(Transient)
	uint64 LastQueuedFrame = MAX_uint64;

	// Block “opened this frame” if you use animation buffer opening (optional latch)
	UPROPERTY(Transient)
	bool bOpenedThisFrame = false;
	
	UPROPERTY(Transient)
	FGASC_STInputEventPayload InputEventPayload;

	void Reset()
	{
		bHasPending = false;
		PendingTag = FGameplayTag(); // invalid/empty [5](https://forums.unrealengine.com/t/why-state-tree-on-state-completed-transition-does-not-work/2630123)
		LastAcceptedFrameByAction.Reset();
		LastQueuedFrame = MAX_uint64;
		bOpenedThisFrame = false;
		InputEventPayload.InputAction = nullptr;
		InputEventPayload.InputActionValue = FInputActionValue();
		InputEventPayload.bWasBufferedAction = false;
		InputEventPayload.InputDirection = FVector::ZeroVector;
	}
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent),Blueprintable)
class GASCOURSE_API UGASC_InputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASC_InputBufferComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void TryInitializeBindings();
	void RemoveBindings();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input Buffer")
	void OpenInputBuffer_ForCategory(const FGameplayTag& Category);
	virtual void OpenInputBuffer_ForCategory_Implementation(const FGameplayTag& Category);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input Buffer")
	void BlockInputCategoryFromBuffer(const FGameplayTag& Category);
	virtual void BlockInputCategoryFromBuffer_Implementation(const FGameplayTag& Category);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input Buffer")
	void ReleaseBlockInputCategoryFromBuffer(const FGameplayTag& Category);
	virtual void ReleaseBlockInputCategoryFromBuffer_Implementation(const FGameplayTag& Category);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input Buffer")
	void CloseInputBuffer_ForCategory(const FGameplayTag& Category);
	virtual void CloseInputBuffer_ForCategory_Implementation(const FGameplayTag& Category);

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	bool FlushInputBuffer(bool bStateTreeFlush = false);
	
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool IsInputBufferOpenForCategory(const FGameplayTag& Category) const;
	
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool IsInputBlockedForCategory(const FGameplayTag& Category) const;
	
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	FGameplayTag FindCategoryTagForInputAction(const UInputAction* InAction) const;

	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool IsInputBufferBlockingInput() const { return  !BlockedInputActionsCategory.IsEmpty(); }
	
	/** True while any category has an open buffer window. Derived - do not cache into a bool. */
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool IsInputBufferOpen() const { return !BufferedInputActionsCategory.IsEmpty(); }
	
	UFUNCTION()
	void InputBufferTimeoutFlush();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	void ActivateBufferedInputAbility();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	void AddInputActionToBuffer(UInputAction* InAction);
	
	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	FVector GetCachedMovementInputVector(bool bFlushMovementVector);
	
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool HasPendingInputBufferEvent() const { return StateTreeQueue.bHasPending && StateTreeQueue.PendingTag.IsValid(); }
	
	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	FORCEINLINE void SetInputBufferTimeout(float NewInputBufferTimeoutOverride)
	{
		InputBufferTimeout = NewInputBufferTimeoutOverride;
	};
	
	UFUNCTION(BlueprintPure, Category="Input Buffer")
	float GetInputBufferTimeout() const { return InputBufferTimeout; }

	UFUNCTION(BlueprintPure, Category="Input Buffer")
	TArray<UInputAction*> GetBufferedInputActions() const;

	UPROPERTY(BlueprintAssignable, Category="Input Buffer")
	FOnInputBufferOpenedEvent OnInputBufferOpenedEvent;

	UPROPERTY(BlueprintAssignable, Category="Input Buffer")
	FOnInputBufferClosedEvent OnInputBufferClosedEvent;

	UPROPERTY(BlueprintAssignable, Category="Input Buffer")
	FOnInputBufferFlushedEvent OnInputBufferFlushedEvent;

	UPROPERTY(BlueprintAssignable, Category="Input Buffer")
	FOnInputBufferedConsumedEvent OnInputBufferedConsumedEvent;
	
	UPROPERTY(BlueprintAssignable, Category="Input Buffer")
	FOnInputBufferBlockInputEvent OnInputBufferBlockInputEvent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer", meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	TArray<TObjectPtr<UInputAction>> InputActionsToBuffer;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer",meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	TObjectPtr<UInputAction> MovementInputActionToBuffer;

	
	// Called by input delegates:
	bool QueueStateTreeEventOncePerActionPerFrame(const UInputAction* Action, const FGameplayTag& Tag, ETriggerEvent TriggerEvent,  const FInputActionValue& InputActionValue, bool bWasBufferedAction = false);

	// Called by StateTree task Tick:
	bool ConsumeQueuedStateTreeEvent(FGameplayTag& OutTag, FGASC_STInputEventPayload& OutPayload);

	void ResetStateTreeEventQueue();

	// If your animation track opens the input buffer on frame 0, call this when opening:
	void MarkInputBufferOpenedThisFrame();


protected:
	
	bool ResolveOwnerObjects();
	void ListenToInputActions();
	void SimulateInputAction(const UInputAction* InputAction) const;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float InputBufferTimeout = 0.4;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<UInputAction*> BufferedInputActions;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", Categories = "InputBuffer.Category"))
	TMap<FGameplayTag, FGASC_BufferedInputActionsArray> BufferedInputActionsByCategory;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FGameplayTagContainer BufferedInputActionsCategory;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedInputActionsCategory;

	UPROPERTY(Transient)
	TObjectPtr<AGASCourseCharacter> OwningCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AGASCoursePlayerController> OwningPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedPlayerInput> PlayerInput = nullptr;
	
	UPROPERTY(Transient)
	FVector CachedMovementInputVector = FVector::ZeroVector;

	TArray<uint32> BindingHandles;
	FString InputBufferComponentName;
	bool bBindingsRegistered = false;
	FTimerHandle InputBufferOpenTimeoutHandle;
	
private:
	
	UPROPERTY(Transient)
	FGASC_StateTreeEventQueue StateTreeQueue;

};