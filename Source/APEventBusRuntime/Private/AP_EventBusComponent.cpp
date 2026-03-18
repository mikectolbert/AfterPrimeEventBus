// Copyright AfterPrime Systems. All Rights Reserved.

#include "AP_EventBusComponent.h"
#include "APEventBusRuntime.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// ---------------------------------------------------------------------------
// T09 — Constructor and Lifecycle
// ---------------------------------------------------------------------------

UAP_EventBusComponent::UAP_EventBusComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UAP_EventBusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// No replicated UPROPERTYs — all replication is RPC-based.
	// Override required for component replication support.
}

void UAP_EventBusComponent::BeginPlay()
{
	Super::BeginPlay();

	// Clients request any cached persistent events from the server.
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerRequestCachedEvents();
	}
}

void UAP_EventBusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllListening();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// T10 — DispatchEvent (internal helper)
// ---------------------------------------------------------------------------

void UAP_EventBusComponent::DispatchEvent(const FAP_EventPayload& Payload)
{
	OnEventReceived.Broadcast(Payload.EventTag, Payload);

	if (ListenedTags.Contains(Payload.EventTag))
	{
		OnListenedEventReceived.Broadcast(Payload.EventTag, Payload);
	}
}

// ---------------------------------------------------------------------------
// T11 — PublishEvent routing
// ---------------------------------------------------------------------------

void UAP_EventBusComponent::PublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope)
{
	if (!Payload.EventTag.IsValid())
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("UAP_EventBusComponent::PublishEvent — EventTag is invalid. Event discarded."));
		return;
	}

	// LocalOnly: fire on calling machine, no authority check, no RPC.
	if (Scope == EAP_EventScope::LocalOnly)
	{
		DispatchEvent(Payload);
		return;
	}

	// Non-local scope on client: transparently route to server.
	if (!GetOwner()->HasAuthority())
	{
		ServerPublishEvent(Payload, Scope);
		return;
	}

	// Authority path — update cache if this is a persistent tag.
	if (PersistentEventTags.Contains(Payload.EventTag))
	{
		LastEventCache.Add(Payload.EventTag, Payload);
	}

	switch (Scope)
	{
		case EAP_EventScope::All:
			// NetMulticast fires on server + all clients — do NOT call DispatchEvent separately.
			MulticastPublishEvent(Payload);
			break;

		case EAP_EventScope::OwnerOnly:
			// Fire on server directly, then RPC to owning client.
			DispatchEvent(Payload);
			ClientDeliverEvent(Payload);
			break;

		case EAP_EventScope::ServerOnly:
			// Server only — no replication.
			DispatchEvent(Payload);
			break;

		default:
			break;
	}
}

// ---------------------------------------------------------------------------
// T12 — RPC Implementations
// ---------------------------------------------------------------------------

void UAP_EventBusComponent::ServerPublishEvent_Implementation(FAP_EventPayload Payload, EAP_EventScope Scope)
{
	// Arrived on server from client — process as if server published it.
	PublishEvent(Payload, Scope);
}

void UAP_EventBusComponent::MulticastPublishEvent_Implementation(FAP_EventPayload Payload)
{
	// Fires on server and all clients via NetMulticast.
	DispatchEvent(Payload);
}

void UAP_EventBusComponent::ClientDeliverEvent_Implementation(FAP_EventPayload Payload)
{
	// Fires on owning client only for OwnerOnly scope.
	DispatchEvent(Payload);
}

void UAP_EventBusComponent::ServerRequestCachedEvents_Implementation()
{
	// Send all cached persistent event payloads to this client.
	for (const TPair<FGameplayTag, FAP_EventPayload>& Entry : LastEventCache)
	{
		ClientReceiveCachedEvent(Entry.Value);
	}
}

void UAP_EventBusComponent::ClientReceiveCachedEvent_Implementation(FAP_EventPayload Payload)
{
	// Late-join delivery — fire delegates as if the event just happened.
	DispatchEvent(Payload);
}

// ---------------------------------------------------------------------------
// T13 — Listener Functions
// ---------------------------------------------------------------------------

void UAP_EventBusComponent::ListenForEvent(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("UAP_EventBusComponent::ListenForEvent — Tag is invalid. Ignored."));
		return;
	}

	// Idempotent — only add if not already listening.
	if (!ListenedTags.Contains(Tag))
	{
		ListenedTags.Add(Tag);
	}
}

void UAP_EventBusComponent::StopListeningForEvent(FGameplayTag Tag)
{
	// No-op if not found.
	ListenedTags.Remove(Tag);
}

void UAP_EventBusComponent::StopAllListening()
{
	ListenedTags.Empty();
}

bool UAP_EventBusComponent::IsListeningForEvent(FGameplayTag Tag) const
{
	return ListenedTags.Contains(Tag);
}

// ---------------------------------------------------------------------------
// T14 — Cache Functions
// ---------------------------------------------------------------------------

void UAP_EventBusComponent::RegisterPersistentEvent(FGameplayTag Tag)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("UAP_EventBusComponent::RegisterPersistentEvent — must be called on server. Ignored."));
		return;
	}

	if (!Tag.IsValid())
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("UAP_EventBusComponent::RegisterPersistentEvent — Tag is invalid. Ignored."));
		return;
	}

	PersistentEventTags.Add(Tag);
}

void UAP_EventBusComponent::UnregisterPersistentEvent(FGameplayTag Tag)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("UAP_EventBusComponent::UnregisterPersistentEvent — must be called on server. Ignored."));
		return;
	}

	PersistentEventTags.Remove(Tag);
	LastEventCache.Remove(Tag);
}

// ---------------------------------------------------------------------------
// T15 — Query Functions
// ---------------------------------------------------------------------------

bool UAP_EventBusComponent::WasEventFired(FGameplayTag Tag) const
{
	return LastEventCache.Contains(Tag);
}

bool UAP_EventBusComponent::GetLastPayload(FGameplayTag Tag, FAP_EventPayload& OutPayload) const
{
	if (const FAP_EventPayload* Found = LastEventCache.Find(Tag))
	{
		OutPayload = *Found;
		return true;
	}
	return false;
}

FGameplayTagContainer UAP_EventBusComponent::GetAllCachedEventTags() const
{
	FGameplayTagContainer Result;
	for (const TPair<FGameplayTag, FAP_EventPayload>& Entry : LastEventCache)
	{
		Result.AddTag(Entry.Key);
	}
	return Result;
}
