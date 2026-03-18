// Copyright AfterPrime Systems. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AP_EventBusTypes.h"
#include "AP_EventBusComponent.generated.h"

/**
 * UAP_EventBusComponent
 *
 * Core pub/sub event bus component. Place on your GameState for a project-wide
 * global bus, or on individual Actors for per-actor buses.
 *
 * - Publish events with PublishEvent. Handles client-to-server routing automatically.
 * - Listen for all events via OnEventReceived (catch-all).
 * - Listen for specific tags via ListenForEvent + OnListenedEventReceived.
 * - Cache persistent events for late-joining clients via RegisterPersistentEvent.
 */
UCLASS(ClassGroup="AP", meta=(BlueprintSpawnableComponent, DisplayName="AP_EventBus"))
class APEVENTBUSRUNTIME_API UAP_EventBusComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UAP_EventBusComponent();

	// -----------------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------------

	/** Fires for every event on this bus. Bind for catch-all listening. */
	UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events",
		meta=(ToolTip="Fires for every event on this bus. Bind for catch-all listening."))
	FAP_OnEventReceived OnEventReceived;

	/** Fires only for events matching tags registered via ListenForEvent. Bind for focused per-tag listening. */
	UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events",
		meta=(ToolTip="Fires only for events matching tags registered via ListenForEvent. Bind for focused per-tag listening."))
	FAP_OnEventReceived OnListenedEventReceived;

	// -----------------------------------------------------------------------
	// Event Publishing
	// -----------------------------------------------------------------------

	/**
	 * Publish an event onto the bus with the specified delivery scope.
	 * Handles client-to-server routing automatically — always call this, never call Server RPCs directly.
	 */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Events",
		meta=(ToolTip="Publish an event onto the bus with the specified delivery scope. Handles client-to-server routing automatically."))
	void PublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope = EAP_EventScope::All);

	// -----------------------------------------------------------------------
	// Listener Registration
	// -----------------------------------------------------------------------

	/** Register to receive filtered events for this tag via OnListenedEventReceived. Idempotent. */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners",
		meta=(ToolTip="Register to receive filtered events for this tag via OnListenedEventReceived. Idempotent."))
	void ListenForEvent(FGameplayTag Tag);

	/** Stop receiving filtered events for this tag. No-op if not currently listening. */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners",
		meta=(ToolTip="Stop receiving filtered events for this tag. No-op if not currently listening."))
	void StopListeningForEvent(FGameplayTag Tag);

	/** Remove all tag filters. Called automatically on EndPlay. */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners",
		meta=(ToolTip="Remove all tag filters. Called automatically on EndPlay."))
	void StopAllListening();

	/** Returns true if this component is listening for the specified tag. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Listeners",
		meta=(ToolTip="Returns true if this component is listening for the specified tag."))
	bool IsListeningForEvent(FGameplayTag Tag) const;

	// -----------------------------------------------------------------------
	// Late-Join Cache
	// -----------------------------------------------------------------------

	/**
	 * Mark this event tag as persistent. The last payload will be cached on the server
	 * and sent to late-joining clients. Server-only.
	 */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache",
		meta=(ToolTip="Mark this event tag as persistent. The last payload will be cached on the server and sent to late-joining clients."))
	void RegisterPersistentEvent(FGameplayTag Tag);

	/** Remove this event tag from the persistent cache. Clears any cached payload. Server-only. */
	UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache",
		meta=(ToolTip="Remove this event tag from the persistent cache. Clears any cached payload."))
	void UnregisterPersistentEvent(FGameplayTag Tag);

	// -----------------------------------------------------------------------
	// Query (BlueprintPure)
	// -----------------------------------------------------------------------

	/** Returns true if the specified event tag has been fired and is in the persistent cache. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Query",
		meta=(ToolTip="Returns true if the specified event tag has been fired and is in the persistent cache."))
	bool WasEventFired(FGameplayTag Tag) const;

	/** Gets the last cached payload for a persistent event tag. Returns false if not cached. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Query",
		meta=(ToolTip="Gets the last cached payload for a persistent event tag. Returns false if not cached."))
	bool GetLastPayload(FGameplayTag Tag, FAP_EventPayload& OutPayload) const;

	/** Returns all event tags currently in the persistent cache. */
	UFUNCTION(BlueprintPure, Category="AP|EventBus|Query",
		meta=(ToolTip="Returns all event tags currently in the persistent cache."))
	FGameplayTagContainer GetAllCachedEventTags() const;

	// -----------------------------------------------------------------------
	// UActorComponent overrides
	// -----------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// -----------------------------------------------------------------------
	// RPCs — do not call these directly. Use PublishEvent.
	// -----------------------------------------------------------------------

	/** Client -> Server. Routes non-local publish calls from clients to the server. */
	UFUNCTION(Server, Reliable)
	void ServerPublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope);

	/** Server -> All. Delivers EAP_EventScope::All events to server + all clients. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPublishEvent(FAP_EventPayload Payload);

	/** Server -> Owning Client. Delivers EAP_EventScope::OwnerOnly events. */
	UFUNCTION(Client, Reliable)
	void ClientDeliverEvent(FAP_EventPayload Payload);

	/** Client -> Server. Requests late-join cached events on BeginPlay. */
	UFUNCTION(Server, Reliable)
	void ServerRequestCachedEvents();

	/** Server -> Client. Delivers one cached event to a late-joining client. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveCachedEvent(FAP_EventPayload Payload);

private:

	/** Internal dispatch — fires both delegates for all dispatch paths. */
	void DispatchEvent(const FAP_EventPayload& Payload);

	/** Per-machine tag filter set for OnListenedEventReceived. Not replicated. */
	TArray<FGameplayTag> ListenedTags;

	/** Server-only: last event payload per persistent tag. Not replicated. */
	TMap<FGameplayTag, FAP_EventPayload> LastEventCache;

	/** Server-only: which tags should cache their last payload. Not replicated. */
	TSet<FGameplayTag> PersistentEventTags;
};
