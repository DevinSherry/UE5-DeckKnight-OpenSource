// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASC_InputBufferComponent.generated.h"

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputBufferedConsumedEvent, UInputAction*, InputAction);

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

	void Reset()
	{
		bHasPending = false;
		PendingTag = FGameplayTag(); // invalid/empty [5](https://forums.unrealengine.com/t/why-state-tree-on-state-completed-transition-does-not-work/2630123)
		LastAcceptedFrameByAction.Reset();
		LastQueuedFrame = MAX_uint64;
		bOpenedThisFrame = false;
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
	void OpenInputBuffer();
	virtual void OpenInputBuffer_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input Buffer")
	void CloseInputBuffer();
	virtual void CloseInputBuffer_Implementation();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	bool FlushInputBuffer();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	void ActivateBufferedInputAbility();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	void AddInputActionToBuffer(UInputAction* InAction);

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	void FlushCachedMovementInputAxisValue();

	UFUNCTION(BlueprintCallable, Category="Input Buffer")
	FVector2D GetMovementInputAxisValue();

	UFUNCTION(BlueprintPure, Category="Input Buffer")
	bool IsInputBufferOpen() const { return bInputBufferOpen; }

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer", meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	TArray<TObjectPtr<UInputAction>> InputActionsToBuffer;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer",meta=(AssetDir="/Game/GASCourse/Game/Character/Input/Actions/"))
	UInputAction* MovementInputActionToBuffer = nullptr;

	
	// Called by input delegates:
	bool QueueStateTreeEventOncePerActionPerFrame(const UInputAction* Action, const FGameplayTag& Tag);

	// Called by StateTree task Tick:
	bool ConsumeQueuedStateTreeEvent(FGameplayTag& OutTag);

	void ResetStateTreeEventQueue();

	// If your animation track opens the input buffer on frame 0, call this when opening:
	void MarkInputBufferOpenedThisFrame();


protected:
	bool ResolveOwnerObjects();
	void ListenToInputActions();
	void SimulateInputAction(const UInputAction* InputAction) const;

protected:
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<UInputAction*> BufferedInputActions;

	UPROPERTY(Transient)
	TObjectPtr<AGASCourseCharacter> OwningCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AGASCoursePlayerController> OwningPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

	UPROPERTY(Transient)
	FVector2D MovementInputAxis = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D BufferedMovementInputAxis = FVector2D::ZeroVector;

	TArray<uint32> BindingHandles;

	FString InputBufferComponentName;

	bool bInputBufferOpen = false;
	bool bBindingsRegistered = false;
	
	
private:
	
	UPROPERTY(Transient)
	FGASC_StateTreeEventQueue StateTreeQueue;

};