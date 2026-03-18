// Copyright AfterPrime Systems. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "AP_EventBusTypes.h"
#include "AP_EventBusBlueprintLibrary.generated.h"

class UAP_EventBusComponent;

/**
 * UAP_EventBusBlueprintLibrary
 *
 * Static convenience helpers for finding the event bus and publishing events.
 * Use GetEventBusFromGameState for the recommended global bus pattern.
 */
UCLASS()
class APEVENTBUSRUNTIME_API UAP_EventBusBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Find the EventBus component on an Actor. Returns null if not found. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities",
		meta=(ToolTip="Find the EventBus component on an Actor. Returns null if not found."))
	static UAP_EventBusComponent* GetEventBusComponent(AActor* Actor);

	/**
	 * Get the EventBus component from the current GameState.
	 * Most common accessor when using the recommended global bus pattern.
	 */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities",
		meta=(ToolTip="Get the EventBus component from the current GameState. Most common accessor for global bus pattern.", WorldContext="WorldContextObject"))
	static UAP_EventBusComponent* GetEventBusFromGameState(const UObject* WorldContextObject);

	/** Find the EventBus on the target Actor and publish an event. No-op if no bus found. */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Utilities",
		meta=(ToolTip="Find the EventBus on the target Actor and publish an event. No-op if no bus found."))
	static void PublishEventOnBus(AActor* BusOwner, FAP_EventPayload Payload, EAP_EventScope Scope = EAP_EventScope::All);

	/** Convenience function to construct an event payload. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities",
		meta=(ToolTip="Convenience function to construct an event payload."))
	static FAP_EventPayload MakeEventPayload(
		FGameplayTag EventTag,
		AActor* Instigator = nullptr,
		AActor* Target = nullptr,
		float Magnitude = 0.0f,
		FGameplayTagContainer ContextTags = FGameplayTagContainer());
};
