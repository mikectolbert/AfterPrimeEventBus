# AP Event Bus Suite Plugin — CLAUDE.md

## Plugin Folder Name
The plugin folder and repo name is `AfterPrimeEventBus`. The .uplugin file is `AfterPrimeEventBus.uplugin`. All C++ class names, module names, and API macros use the `AP_` prefix convention — they do NOT match the folder name. This is intentional and consistent with all AfterPrime plugins.

## Publisher
AfterPrime Systems — AP_ prefix applies to all class names and asset prefixes. Plugin folder uses the AfterPrime long-form name.

## Overview
AP Event Bus is a server-authoritative, replicated event messaging system for UE5 multiplayer. Any actor can publish a named event (identified by FGameplayTag) with a structured payload onto a central bus. Any actor can listen for events by tag without holding a direct reference to the publisher. Supports four delivery scopes (All, OwnerOnly, ServerOnly, LocalOnly), a late-join cache for persistent events, and transparent client-to-server routing. No GAS dependency. Blueprint-first — fully usable without C++. Free product serving as lead magnet for the AfterPrime plugin family.

## Engine Target
- Minimum UE version: 5.7
- IDE: Visual Studio 2022 with UnrealVS extension
- Platform: Win64

## Module Structure
- Module: APEventBusRuntime (Runtime)

Single Runtime module. No Editor module for v1.

## Classes

- `UAP_EventBusComponent` — Core ActorComponent. Manages event publishing, listener registration, RPC dispatch, scoped delivery, and late-join cache. Place on GameState for a global bus.
- `FAP_EventPayload` — USTRUCT(BlueprintType). Self-contained event data: tag, instigator, target, magnitude, context tags.
- `EAP_EventScope` — UENUM(BlueprintType). Delivery scope: All, OwnerOnly, ServerOnly, LocalOnly.
- `UAP_EventBusBlueprintLibrary` — UBlueprintFunctionLibrary. Static convenience helpers for publishing events and finding the bus.

## Public API

### EAP_EventScope

```cpp
UENUM(BlueprintType)
enum class EAP_EventScope : uint8
{
    All         UMETA(ToolTip="Server fires + NetMulticast to all clients"),
    OwnerOnly   UMETA(ToolTip="Server fires + Client RPC to owning connection only"),
    ServerOnly  UMETA(ToolTip="Server fires locally only, no replication"),
    LocalOnly   UMETA(ToolTip="Fires on calling machine only, zero network cost")
};
```

### FAP_EventPayload

```cpp
USTRUCT(BlueprintType)
struct APEVENTBUSRUNTIME_API FAP_EventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus", meta=(ToolTip="Gameplay Tag identifying this event"))
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus", meta=(ToolTip="The actor that initiated this event. May be null."))
    TObjectPtr<AActor> Instigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus", meta=(ToolTip="The target actor of this event. May be null."))
    TObjectPtr<AActor> Target = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus", meta=(ToolTip="General-purpose numeric value (damage, healing, distance, etc.)"))
    float Magnitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus", meta=(ToolTip="Additional context tags for filtering or metadata"))
    FGameplayTagContainer ContextTags;
};
```

Single struct used for both local dispatch and RPC parameters. TObjectPtr<AActor> serializes natively across RPCs for replicated actors.

### Delegate Signature

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAP_OnEventReceived,
    FGameplayTag, EventTag,
    FAP_EventPayload, Payload);
```

Used for both OnEventReceived (catch-all) and OnListenedEventReceived (filtered).

### UAP_EventBusComponent

**UCLASS Configuration:**
```cpp
UCLASS(ClassGroup="AP", meta=(BlueprintSpawnableComponent, DisplayName="AP_EventBus"))
class APEVENTBUSRUNTIME_API UAP_EventBusComponent : public UActorComponent
```

**Constructor Requirements:**
- SetIsReplicatedByDefault(true)
- PrimaryComponentTick.bCanEverTick = false

**Functions — Event Publishing:**

```cpp
// Main publish function. Buyers always call this — handles client-to-server routing transparently.
// For All/OwnerOnly/ServerOnly: requires authority. If called on client, auto-routes to ServerPublishEvent RPC.
// For LocalOnly: fires on calling machine, no authority check, no RPC.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Events", meta=(ToolTip="Publish an event onto the bus with the specified delivery scope. Handles client-to-server routing automatically."))
void PublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope = EAP_EventScope::All);

// Client -> Server RPC. Called internally by PublishEvent when client tries to fire a non-Local event.
// Buyers do NOT call this directly — PublishEvent routes to it automatically.
UFUNCTION(Server, Reliable)
void ServerPublishEvent(FAP_EventPayload Payload, EAP_EventScope Scope);
// Implementation: calls PublishEvent on server after arrival.

// Server -> All RPC. Delivers EAP_EventScope::All events.
// Fires on server + all clients via NetMulticast.
UFUNCTION(NetMulticast, Reliable)
void MulticastPublishEvent(FAP_EventPayload Payload);
// Implementation: calls DispatchEvent(Payload).

// Server -> Owning Client RPC. Delivers EAP_EventScope::OwnerOnly events.
UFUNCTION(Client, Reliable)
void ClientDeliverEvent(FAP_EventPayload Payload);
// Implementation: calls DispatchEvent(Payload).
```

**PublishEvent routing logic (internal):**
```
if (Scope == LocalOnly)
    -> DispatchEvent(Payload) directly, return

if (!HasAuthority())
    -> ServerPublishEvent(Payload, Scope) RPC, return

// We have authority from here:
if (Scope == All)
    -> Update LastEventCache if persistent tag
    -> MulticastPublishEvent(Payload) — fires on server + all clients

if (Scope == OwnerOnly)
    -> Update LastEventCache if persistent tag
    -> DispatchEvent(Payload) on server directly
    -> ClientDeliverEvent(Payload) to owning client

if (Scope == ServerOnly)
    -> Update LastEventCache if persistent tag
    -> DispatchEvent(Payload) on server directly
```

**Functions — Listener Registration:**

```cpp
// Register to receive filtered events for this tag via OnListenedEventReceived.
// Idempotent — calling twice with the same tag is safe.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners", meta=(ToolTip="Register to receive filtered events for this tag via OnListenedEventReceived. Idempotent."))
void ListenForEvent(FGameplayTag Tag);

// Stop receiving filtered events for this tag. No-op if not currently listening.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners", meta=(ToolTip="Stop receiving filtered events for this tag. No-op if not currently listening."))
void StopListeningForEvent(FGameplayTag Tag);

// Remove all tag filters. Called automatically on EndPlay.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Listeners", meta=(ToolTip="Remove all tag filters. Called automatically on EndPlay."))
void StopAllListening();

// Returns true if this component is listening for the specified tag.
UFUNCTION(BlueprintPure, Category="AP|EventBus|Listeners", meta=(ToolTip="Returns true if this component is listening for the specified tag"))
bool IsListeningForEvent(FGameplayTag Tag) const;
```

**Functions — Late-Join Cache:**

```cpp
// Mark this event tag as persistent. Last payload for this tag is cached server-side
// and sent to late-joining clients.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache", meta=(ToolTip="Mark this event tag as persistent. The last payload will be cached on the server and sent to late-joining clients."))
void RegisterPersistentEvent(FGameplayTag Tag);
// Authority: Server only. HasAuthority() check at top.

// Remove this tag from the persistent cache.
UFUNCTION(BlueprintCallable, Category="AP|EventBus|Cache", meta=(ToolTip="Remove this event tag from the persistent cache. Clears any cached payload."))
void UnregisterPersistentEvent(FGameplayTag Tag);
// Authority: Server only. HasAuthority() check at top.

// Client -> Server RPC. Called in BeginPlay on client.
// Server iterates LastEventCache and calls ClientReceiveCachedEvent for each persistent entry.
UFUNCTION(Server, Reliable)
void ServerRequestCachedEvents();

// Server -> Client RPC. Delivers one cached event to a late-joining client.
UFUNCTION(Client, Reliable)
void ClientReceiveCachedEvent(FAP_EventPayload Payload);
// Implementation: calls DispatchEvent(Payload).
```

**Functions — Query (BlueprintPure):**

```cpp
// Returns true if the specified event tag has been fired and is in the persistent cache.
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query", meta=(ToolTip="Returns true if the specified event tag has been fired and is in the persistent cache."))
bool WasEventFired(FGameplayTag Tag) const;

// Gets the last cached payload for a persistent event tag. Returns false if not cached.
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query", meta=(ToolTip="Gets the last cached payload for a persistent event tag. Returns false if not cached."))
bool GetLastPayload(FGameplayTag Tag, FAP_EventPayload& OutPayload) const;

// Returns all event tags currently in the persistent cache.
UFUNCTION(BlueprintPure, Category="AP|EventBus|Query", meta=(ToolTip="Returns all event tags currently in the persistent cache"))
FGameplayTagContainer GetAllCachedEventTags() const;
```

**Delegates:**

```cpp
// Fires for EVERY event on this bus, regardless of tag.
// Server: fires from DispatchEvent (called by direct dispatch or MulticastPublishEvent).
// Client: fires from DispatchEvent (called by MulticastPublishEvent, ClientDeliverEvent, or ClientReceiveCachedEvent).
UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events", meta=(ToolTip="Fires for every event on this bus. Bind for catch-all listening."))
FAP_OnEventReceived OnEventReceived;

// Fires only for events matching tags registered via ListenForEvent.
// Same firing paths as OnEventReceived, but filtered by ListenedTags set.
UPROPERTY(BlueprintAssignable, Category="AP|EventBus|Events", meta=(ToolTip="Fires only for events matching tags registered via ListenForEvent. Bind for focused per-tag listening."))
FAP_OnEventReceived OnListenedEventReceived;
```

Both delegates are self-contained — payload carries all data, no need to read component state after firing.

**Private Members:**

```cpp
// Tag filter set for OnListenedEventReceived. Local state, not replicated.
TArray<FGameplayTag> ListenedTags;

// Server-only: last event payload per persistent tag.
// NOT replicated. TMap is server-only by design.
TMap<FGameplayTag, FAP_EventPayload> LastEventCache;

// Server-only: which tags should have their last payload cached.
// NOT replicated. TSet is server-only by design.
TSet<FGameplayTag> PersistentEventTags;
```

**Internal Helper:**

```cpp
// Called by all dispatch paths: MulticastPublishEvent_Implementation,
// ClientDeliverEvent_Implementation, ClientReceiveCachedEvent_Implementation,
// and direct dispatch for ServerOnly/LocalOnly.
// Fires OnEventReceived unconditionally.
// Checks ListenedTags — if EventTag matches, also fires OnListenedEventReceived.
void DispatchEvent(const FAP_EventPayload& Payload);
```

**Lifecycle:**

```cpp
// BeginPlay: if not HasAuthority (i.e., client), call ServerRequestCachedEvents RPC.
virtual void BeginPlay() override;

// EndPlay: call StopAllListening() to clean up listener state.
virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
```

### UAP_EventBusBlueprintLibrary

```cpp
UCLASS()
class APEVENTBUSRUNTIME_API UAP_EventBusBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Find the EventBus component on an Actor. Returns null if not found.
    UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities", meta=(ToolTip="Find the EventBus component on an Actor. Returns null if not found."))
    static UAP_EventBusComponent* GetEventBusComponent(AActor* Actor);

    // Get the EventBus component from the current GameState.
    // Most common accessor when using the recommended global bus pattern.
    UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities", meta=(ToolTip="Get the EventBus component from the current GameState. Most common accessor for global bus pattern.", WorldContext="WorldContextObject"))
    static UAP_EventBusComponent* GetEventBusFromGameState(const UObject* WorldContextObject);

    // Find the EventBus on the target Actor and publish an event. No-op if no bus found.
    UFUNCTION(BlueprintCallable, Category="AP|EventBus|Utilities", meta=(ToolTip="Find the EventBus on the target Actor and publish an event. No-op if no bus found."))
    static void PublishEventOnBus(AActor* BusOwner, FAP_EventPayload Payload, EAP_EventScope Scope = EAP_EventScope::All);

    // Convenience function to construct an event payload.
    UFUNCTION(BlueprintPure, Category="AP|EventBus|Utilities", meta=(ToolTip="Convenience function to construct an event payload."))
    static FAP_EventPayload MakeEventPayload(FGameplayTag EventTag, AActor* Instigator = nullptr, AActor* Target = nullptr, float Magnitude = 0.0f, FGameplayTagContainer ContextTags = FGameplayTagContainer());
};
```

## Replication Strategy

### Authority Model
Server owns all event dispatch for All, OwnerOnly, and ServerOnly scopes. LocalOnly has no authority requirement. Events are ephemeral — no replicated UPROPERTYs. All replication is RPC-based.

### RPC Table

| RPC | Direction | Type | Purpose |
|-----|-----------|------|---------|
| ServerPublishEvent | Client -> Server | Server, Reliable | Client requests event publication |
| MulticastPublishEvent | Server -> All | NetMulticast, Reliable | EAP_EventScope::All delivery |
| ClientDeliverEvent | Server -> Owning Client | Client, Reliable | EAP_EventScope::OwnerOnly delivery |
| ServerRequestCachedEvents | Client -> Server | Server, Reliable | Client requests late-join cache |
| ClientReceiveCachedEvent | Server -> Client | Client, Reliable | Server delivers one cached event |

### NetMulticast Double-Fire Prevention
MulticastPublishEvent fires on server + all clients natively. For All scope, the server calls MulticastPublishEvent only — no separate direct dispatch. The Multicast implementation calls DispatchEvent which handles both delegates. This prevents double-firing.

For OwnerOnly scope, the server dispatches directly (DispatchEvent) then sends ClientDeliverEvent to the owning client. No double-fire risk since Client RPCs don't fire on server.

### Transparent Client-to-Server Routing
Buyers always call PublishEvent. If called on a client with non-Local scope, PublishEvent internally routes to ServerPublishEvent RPC. No separate "request" function needed in Blueprint — one node works from anywhere.

### Late-Join Cache Flow
1. Server calls RegisterPersistentEvent(Tag) at setup
2. PublishEvent stores payload in LastEventCache for persistent tags
3. New client's BeginPlay calls ServerRequestCachedEvents RPC
4. Server iterates LastEventCache, calls ClientReceiveCachedEvent per entry
5. Client's ClientReceiveCachedEvent calls DispatchEvent — delegates fire as if event just happened

### What Does NOT Replicate
- ListenedTags — local filter set per machine
- LastEventCache — server-only TMap
- PersistentEventTags — server-only TSet
- No replicated UPROPERTYs at all — 100% RPC-based

## Gameplay Tags
- No plugin-shipped .ini file for v1 — buyers supply their own tags
- GameplayTags module used as key type only
- StartupModule / ShutdownModule are empty stubs
- No AddTagIniSearchPath call needed

## Build.cs Configuration
```
PublicDependencyModuleNames: Core, CoreUObject, Engine, NetCore, GameplayTags
PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs
```

## Log Category
```
LogEventBus — DECLARE_LOG_CATEGORY_EXTERN in APEventBusRuntime.h, DEFINE_LOG_CATEGORY in APEventBusRuntime.cpp
```

Use UE_LOG(LogEventBus, Warning, ...) for invalid state warnings (empty tag, non-replicated actor with network scope, etc.).

## UE5 Coding Standards to Enforce
- U/A/F/E/I/T/b prefixes on all types and booleans, plus AP_ product prefix on all public classes
- UFUNCTION(BlueprintCallable, Category="AP|EventBus|...", meta=(ToolTip="...")) on all public functions
- UFUNCTION(BlueprintPure, Category="AP|EventBus|...", meta=(ToolTip="...")) on all getter/query functions
- GetLifetimeReplicatedProps override required (component replicates for RPC support)
- HasAuthority() check at top of all server-only functions
- TObjectPtr for owned refs, TWeakObjectPtr where ownership is ambiguous
- No raw new/delete — use NewObject, MakeShared, CreateDefaultSubobject
- No editor includes in Runtime module
- #pragma once in every header — no #ifndef guards
- IWYU-compliant: each header includes only what it directly uses
- Module API macro on every public class: APEVENTBUSRUNTIME_API
- Copyright notice first line of every source file: // Copyright AfterPrime Systems. All Rights Reserved.
- All delegate payloads are self-contained — never require reading component state after the event fires
- SetIsReplicatedByDefault(true) and PrimaryComponentTick.bCanEverTick = false in component constructor
- ClassGroup="AP", meta=(DisplayName="AP_EventBus") on component UCLASS

## Scope Boundaries
This plugin does NOT:
- Depend on GameplayAbilities module or any GAS classes
- Use FastArray replication — events are ephemeral, all replication is RPC-based
- Persist event state across level loads or game sessions — cache is transient
- Include an Editor module — no custom detail panels, no editor tools
- Include a UMG widget base class
- Include any built-in Gameplay Tag .ini or demo tags — buyers supply their own
- Implement listener priority ordering (v1.1 candidate)
- Implement event filtering by tag hierarchy or wildcard matching (v1.1 candidate)
- Implement timed or delayed events (v1.2 candidate)
- Depend on any other AfterPrime plugin — fully standalone

Claude Code must not implement anything outside these boundaries without explicit user instruction.

## Known UE5 Constraints
- **TMap cannot replicate.** LastEventCache and PersistentEventTags are server-only TMap/TSet fields. This is correct and intentional — they never leave the server.
- **NetMulticast fires on server too.** MulticastPublishEvent fires DispatchEvent on the server as well as all clients. For All scope, the server must NOT also call DispatchEvent separately — the Multicast handles it. Double-fire prevention is critical.
- **LocalOnly scope skips all authority checks.** No HasAuthority, no RPC. Fires delegate directly on calling machine. Useful for UI-only events that should never travel the network.
- **Component placement matters.** Placing UAP_EventBusComponent on GameState creates a single global bus. Placing it on Character creates per-player buses. The recommended pattern is GameState. Document this prominently in ToolTip and QuickStart.
- **Listener cleanup.** StopAllListening() must be called in EndPlay to prevent stale delegate handles. This is handled automatically.
- **RPC struct serialization.** FAP_EventPayload uses TObjectPtr<AActor> for Instigator/Target. These serialize natively across RPCs for replicated actors. If the referenced actor is not replicated, the pointer arrives as null on the remote end — this is expected UE5 behavior.
- **Late-join timing.** Client calls ServerRequestCachedEvents in BeginPlay. If the component's owner (e.g., GameState) hasn't replicated yet, UE5's RPC buffering will queue the call until it can be delivered.

## Editor Setup Requirements
These steps must be completed in the UE5 Editor — Claude Code cannot do them:
- [ ] Create a GameState Blueprint (e.g., BP_AP_EventBus_DemoGameState) and add UAP_EventBusComponent from the Add Component panel (displays as "AP_EventBus")
- [ ] Create a GameMode Blueprint that uses the demo GameState
- [ ] In the GameState Blueprint Event Graph, call RegisterPersistentEvent for any tags that should be cached for late-joining clients
- [ ] In a test Blueprint Actor or Character, get a reference to the EventBus component (via GetEventBusFromGameState node) and bind OnEventReceived delegate to a Print String node for testing
- [ ] Create demo Character Blueprint: get EventBus reference, call ListenForEvent for specific tags, bind OnListenedEventReceived to Print String
- [ ] Configure multiplayer PIE: Edit -> Editor Preferences -> Level Editor -> Play -> set Number of Players to 2, enable Run Dedicated Server

## Editor Verification Checklist
After each compile, verify in the UE5 Editor:
- [ ] UAP_EventBusComponent appears as "AP_EventBus" in the Add Component panel on any Blueprint Actor
- [ ] PublishEvent node appears when dragging off the component reference in Blueprint, under "AP|EventBus|Events" category
- [ ] All BlueprintPure query functions (WasEventFired, GetLastPayload, GetAllCachedEventTags, IsListeningForEvent) appear without execution pins
- [ ] OnEventReceived and OnListenedEventReceived appear as bindable events when right-clicking the component
- [ ] GetEventBusFromGameState appears in the Blueprint palette as a static function under "AP|EventBus|Utilities"
- [ ] MakeEventPayload appears as a pure node for constructing payloads
- [ ] In standalone PIE: Publish an event -> verify OnEventReceived fires with correct payload
- [ ] In standalone PIE: Call ListenForEvent for a specific tag -> publish that tag -> verify OnListenedEventReceived fires. Publish a different tag -> verify OnListenedEventReceived does NOT fire.
- [ ] In multiplayer PIE (dedicated server + 1 client): Publish event with All scope from server -> verify both server and client receive it via OnEventReceived
- [ ] In multiplayer PIE: Publish event with OwnerOnly scope -> verify only owning client receives ClientDeliverEvent
- [ ] In multiplayer PIE: Publish event with ServerOnly scope -> verify only server fires delegate, client does NOT
- [ ] In multiplayer PIE: Client calls PublishEvent (non-Local scope) -> verify it transparently routes to server via ServerPublishEvent RPC -> server processes and broadcasts
- [ ] In multiplayer PIE: Register a persistent event tag -> publish it -> have a second client join late -> verify late-joiner receives cached event via ClientReceiveCachedEvent
- [ ] Verify PublishEvent with empty/invalid tag logs a warning via LogEventBus and does not crash
