# AP Event Bus — API Reference

**Version:** 1.0.0
**Module:** APEventBusRuntime
**API Macro:** `APEVENTBUSRUNTIME_API`

---

## Contents

- [EAP_EventScope](#eap_eventscope)
- [FAP_EventPayload](#fap_eventpayload)
- [FAP_OnEventReceived](#fap_oneventreceived)
- [UAP_EventBusComponent](#uap_eventbuscomponent)
  - [Delegates](#delegates)
  - [Publishing](#publishing)
  - [Listeners](#listeners)
  - [Cache](#cache)
  - [Query](#query)
  - [Lifecycle](#lifecycle)
- [UAP_EventBusBlueprintLibrary](#uap_eventbusblueprintlibrary)
- [AAP_EventBusDemoCharacter](#aap_eventbussdemocharacter)
- [Log Category](#log-category)

---

## EAP_EventScope

```cpp
UENUM(BlueprintType)
enum class EAP_EventScope : uint8
```

Controls how an event is delivered across the network.

| Value | Description |
|-------|-------------|
| `All` | Server fires locally + NetMulticast to all connected clients. |
| `OwnerOnly` | Server fires locally + Client RPC to the owning connection only. Best for player-specific events when the bus is on a player-owned actor. |
| `ServerOnly` | Server fires locally only. No replication. Zero client cost. |
| `LocalOnly` | Fires on the calling machine only. No authority check, no RPC. Zero network cost. Use for UI-only events. |

**Blueprint location:** `AP|EventBus` enum picker on any Scope pin.

---

## FAP_EventPayload

```cpp
USTRUCT(BlueprintType)
struct APEVENTBUSRUNTIME_API FAP_EventPayload
```

Self-contained event data. All fields are optional except `EventTag`. Passed by value through all publish and RPC paths — no component state needs to be read after the delegate fires.

| Property | Type | Description |
|----------|------|-------------|
| `EventTag` | `FGameplayTag` | **Required.** Gameplay Tag identifying the event. Must be a registered tag — `PublishEvent` validates and discards if invalid. |
| `Instigator` | `TObjectPtr<AActor>` | The actor that triggered the event. May be null. |
| `Target` | `TObjectPtr<AActor>` | The target actor of the event. May be null. |
| `Magnitude` | `float` | General-purpose numeric value. Default: `0.0`. |
| `ContextTags` | `FGameplayTagContainer` | Additional tags for filtering or metadata. |

> **Multiplayer note:** `Instigator` and `Target` are actor object pointers. They serialize correctly across RPCs for replicated actors. If the referenced actor is not replicated, the pointer arrives as `null` on the remote end — this is expected UE5 behavior.

---

## FAP_OnEventReceived

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FAP_OnEventReceived,
    FGameplayTag, EventTag,
    FAP_EventPayload, Payload);
```

Shared delegate signature used by both `OnEventReceived` and `OnListenedEventReceived`.

---

## UAP_EventBusComponent

```cpp
UCLASS(ClassGroup="AP", meta=(BlueprintSpawnableComponent, DisplayName="AP_EventBus"))
class APEVENTBUSRUNTIME_API UAP_EventBusComponent : public UActorComponent
```

Core pub/sub component. Place on your **GameState** for a project-wide global bus, or on any individual Actor for a scoped per-actor bus. Each component instance is an independent bus.

**Blueprint category:** `AP|EventBus`
**Editor display name:** `AP_EventBus`

---

### Delegates

---

#### OnEventReceived

```cpp
UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events")
FAP_OnEventReceived OnEventReceived;
```

Fires for **every** event dispatched on this bus, regardless of tag. Bind this for catch-all listening.

Fires on:
- Server — via direct `DispatchEvent` (ServerOnly, OwnerOnly) or `MulticastPublishEvent_Implementation` (All)
- Client — via `MulticastPublishEvent_Implementation` (All), `ClientDeliverEvent_Implementation` (OwnerOnly), or `ClientReceiveCachedEvent_Implementation` (late-join cache)

---

#### OnListenedEventReceived

```cpp
UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events")
FAP_OnEventReceived OnListenedEventReceived;
```

Fires only for events whose `EventTag` matches a tag registered via `ListenForEvent`. Use alongside `OnEventReceived` when an actor only cares about a specific subset of bus traffic.

Fires on the same paths as `OnEventReceived`, but gated by the local `ListenedTags` filter set.

---

### Publishing

---

#### PublishEvent

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Events")
void PublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope = EAP_EventScope::All);
```

Publish an event onto the bus. This is the **only publish function buyers should call** — it handles all routing automatically.

**Routing logic:**

| Condition | Action |
|-----------|--------|
| `Scope == LocalOnly` | `DispatchEvent` immediately on calling machine. No authority check. |
| `!HasAuthority() && Scope != LocalOnly` | Routes to `ServerPublishEvent` RPC. Arrives on server, re-enters `PublishEvent` with authority. |
| `HasAuthority() && Scope == All` | Updates cache if persistent, then calls `MulticastPublishEvent` (fires on server + all clients). |
| `HasAuthority() && Scope == OwnerOnly` | Updates cache if persistent, then `DispatchEvent` on server + `ClientDeliverEvent` to owning client. |
| `HasAuthority() && Scope == ServerOnly` | Updates cache if persistent, then `DispatchEvent` on server only. |

**Validation:** If `Payload.EventTag` is not valid, logs `LogEventBus Warning` and returns without firing.

---

### Listeners

---

#### ListenForEvent

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners")
void ListenForEvent(FGameplayTag Tag);
```

Register a tag filter so that `OnListenedEventReceived` fires when events with this tag are dispatched. Idempotent — calling twice with the same tag is safe. Has no effect on `OnEventReceived` (which fires unconditionally).

---

#### StopListeningForEvent

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners")
void StopListeningForEvent(FGameplayTag Tag);
```

Remove a single tag from the filter set. No-op if the tag was not registered. Does not affect `OnEventReceived` bindings.

---

#### StopAllListening

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners")
void StopAllListening();
```

Clear all tag filters. Called automatically in `EndPlay` — you do not need to call this manually on actor destruction.

---

#### IsListeningForEvent

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Listeners")
bool IsListeningForEvent(FGameplayTag Tag) const;
```

Returns `true` if the specified tag is currently in the filter set.

---

### Cache

---

#### RegisterPersistentEvent

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache")
void RegisterPersistentEvent(FGameplayTag Tag);
```

Mark a tag as persistent. After registration, every call to `PublishEvent` for this tag stores the payload in the server-side cache. Late-joining clients automatically receive the last cached payload for all persistent tags when their component's `BeginPlay` fires.

**Authority:** Server only. Logs `LogEventBus Warning` and returns if called on a client.

**Recommended placement:** GameState `BeginPlay` — ensures the tag is registered before any events fire.

---

#### UnregisterPersistentEvent

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache")
void UnregisterPersistentEvent(FGameplayTag Tag);
```

Remove a tag from the persistent set and clear its cached payload. Future publishes for this tag will no longer be cached. Late joiners will not receive this event.

**Authority:** Server only. Logs `LogEventBus Warning` and returns if called on a client.

---

### Query

All query functions are `BlueprintPure` — they appear as nodes without execution pins in Blueprint.

---

#### WasEventFired

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query")
bool WasEventFired(FGameplayTag Tag) const;
```

Returns `true` if the specified tag is in the persistent cache (i.e., `RegisterPersistentEvent` was called for it AND at least one `PublishEvent` has fired for it since registration).

> This queries the local cache. Meaningful on the server. On clients, the cache is empty — use the `OnEventReceived` delegate to react to events on clients.

---

#### GetLastPayload

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query")
bool GetLastPayload(FGameplayTag Tag, FAP_EventPayload& OutPayload) const;
```

Retrieve the last cached payload for a persistent event tag.

| Return | Meaning |
|--------|---------|
| `true` | `OutPayload` is populated with the last published payload for this tag. |
| `false` | Tag is not in the cache. `OutPayload` is unchanged. |

---

#### GetAllCachedEventTags

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query")
FGameplayTagContainer GetAllCachedEventTags() const;
```

Returns a `FGameplayTagContainer` containing all tags currently in the persistent cache. Returns an empty container if no persistent events have been fired.

---

### Lifecycle

These are standard `UActorComponent` overrides. You do not call these directly.

| Function | Behavior |
|----------|----------|
| `BeginPlay` | If not authority (client), automatically calls `ServerRequestCachedEvents` to receive any cached persistent events from the server. |
| `EndPlay` | Calls `StopAllListening()` to clean up the tag filter set. |
| `GetLifetimeReplicatedProps` | Required override for component replication support. No replicated `UPROPERTY`s — all replication is RPC-based. |

---

### RPCs (Internal)

These RPCs are called internally by `PublishEvent`. **Do not call them directly.**

| RPC | Specifier | Direction | Purpose |
|-----|-----------|-----------|---------|
| `ServerPublishEvent` | `Server, Reliable` | Client → Server | Routes non-local publish calls from clients to the server. |
| `MulticastPublishEvent` | `NetMulticast, Reliable` | Server → All | Delivers `All` scope events to server and all clients. |
| `ClientDeliverEvent` | `Client, Reliable` | Server → Owning Client | Delivers `OwnerOnly` scope events to the owning connection. |
| `ServerRequestCachedEvents` | `Server, Reliable` | Client → Server | Called in `BeginPlay` on clients to request cached persistent events. |
| `ClientReceiveCachedEvent` | `Client, Reliable` | Server → Client | Delivers one cached event to a late-joining client. |

---

## UAP_EventBusBlueprintLibrary

```cpp
UCLASS()
class APEVENTBUSRUNTIME_API UAP_EventBusBlueprintLibrary : public UBlueprintFunctionLibrary
```

Static convenience functions. Available in Blueprint under **AP|EventBus|Utilities**.

---

#### GetEventBusComponent

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities")
static UAP_EventBusComponent* GetEventBusComponent(AActor* Actor);
```

Find the `UAP_EventBusComponent` on any Actor. Returns `null` if the actor is invalid or has no bus component.

---

#### GetEventBusFromGameState

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities",
    meta=(WorldContext="WorldContextObject"))
static UAP_EventBusComponent* GetEventBusFromGameState(const UObject* WorldContextObject);
```

Get the `UAP_EventBusComponent` from the current world's GameState. The most common accessor when using the recommended global bus pattern.

Returns `null` if the world, GameState, or component is not found.

> **Timing:** In multiplayer, call this after a short delay in `BeginPlay` (e.g., `Delay 0.1`) to ensure the GameState has replicated before access.

---

#### PublishEventOnBus

```cpp
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Utilities")
static void PublishEventOnBus(AActor* BusOwner, FAP_EventPayload Payload,
    EAP_EventScope Scope = EAP_EventScope::All);
```

Convenience wrapper: find the bus on `BusOwner` and call `PublishEvent`. No-op if no bus component is found on the actor.

---

#### MakeEventPayload

```cpp
UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities")
static FAP_EventPayload MakeEventPayload(
    FGameplayTag EventTag,
    AActor* Instigator   = nullptr,
    AActor* Target       = nullptr,
    float Magnitude      = 0.0f,
    FGameplayTagContainer ContextTags = FGameplayTagContainer());
```

Construct and return a `FAP_EventPayload`. Appears as a pure node in Blueprint — use this instead of splitting struct pins manually.

---

## AAP_EventBusDemoCharacter

```cpp
UCLASS()
class APEVENTBUSRUNTIME_API AAP_EventBusDemoCharacter : public ACharacter
```

C++ reference implementation demonstrating AP Event Bus usage from code. Ships as a demo/example — not required for plugin usage.

On `BeginPlay`, caches the GameState bus, binds both delegates, and registers three demo tag filters. On `SetupPlayerInputComponent`, binds four input actions (locally controlled only).

| Input Action | Scope | Tag | Magnitude |
|---|---|---|---|
| `DemoPublishDamage` | All | `Event.Demo.Damage` | 25 |
| `DemoPublishHeal` | All | `Event.Demo.Heal` | 10 |
| `DemoPublishAlert` | ServerOnly | `Event.Demo.Alert` | 0 |
| `DemoPublishUINotify` | LocalOnly | `Event.Demo.UINotify` | 0 |

> **Note:** Demo tags are not shipped in a plugin `.ini`. Register them in your project's Gameplay Tags config before use.

**Key property:**

| Property | Type | Description |
|----------|------|-------------|
| `EventBus` | `TObjectPtr<UAP_EventBusComponent>` | Cached bus reference. `BlueprintReadOnly`. |

---

## Log Category

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogEventBus, Log, All)
```

All plugin log output uses the `LogEventBus` category. Filter in the Output Log with:

```
LogEventBus
```

**Warning messages emitted by the plugin:**

| Message | Cause |
|---------|-------|
| `PublishEvent — EventTag is invalid. Event discarded.` | `Payload.EventTag` is not a registered Gameplay Tag. |
| `ListenForEvent — Tag is invalid. Ignored.` | Tag passed to `ListenForEvent` is not valid. |
| `RegisterPersistentEvent — must be called on server. Ignored.` | Called on a client. |
| `RegisterPersistentEvent — Tag is invalid. Ignored.` | Tag is not valid. |
| `UnregisterPersistentEvent — must be called on server. Ignored.` | Called on a client. |
| `AAP_EventBusDemoCharacter: No AP_EventBusComponent found on GameState.` | Demo character could not find a bus on the GameState. |
