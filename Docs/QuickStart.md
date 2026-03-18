# AP Event Bus — Quick Start Guide

## Overview

AP Event Bus is a server-authoritative, replicated event messaging system for UE5 multiplayer.
Any actor can publish a named event (identified by a Gameplay Tag) onto a bus. Any actor can
listen for events by tag — no direct references required between publishers and subscribers.

**Free product. No GAS dependency. Blueprint-first.**

---

## Installation

1. Copy the `AfterPrimeEventBus` folder into your project's `Plugins/` directory.
2. Open your project in UE5.
3. Go to **Edit → Plugins**, search for **"AP Event Bus"**, enable it, and restart the editor.
4. The plugin is now available. No additional configuration required.

---

## Recommended Setup — Global Bus on GameState

The most common pattern is a single global bus placed on the GameState. This makes the bus
accessible from any actor in the world at any time.

1. Open (or create) your **GameState Blueprint**.
2. In the Components panel, click **Add** and search for **AP_EventBus**.
3. Add the component. It will appear as `AP_EventBus` in the panel.
4. Set this GameState in your GameMode (**Class Defaults → Game State Class**).

That's all the setup required for a global bus.

### Alternative: Per-Actor Bus

You can also place `UAP_EventBusComponent` on any individual Actor (Character, Pawn,
PlayerState, etc.) to create a bus scoped to that actor. Each component instance is an
independent bus — events published to one do not reach listeners on another.

---

## Getting a Reference to the Bus

### From Blueprint

Use the **Get Event Bus from Game State** node (found under **AP|EventBus|Utilities**):

```
Get Event Bus from Game State (WorldContextObject = Self)
    → Return Value (UAP_EventBusComponent reference)
```

Store this reference in a variable. Call it once in **BeginPlay** — add a short `Delay (0.1)`
before the call to ensure the GameState is fully replicated before accessing it.

### From C++

```cpp
#include "AP_EventBusBlueprintLibrary.h"

UAP_EventBusComponent* Bus = UAP_EventBusBlueprintLibrary::GetEventBusFromGameState(this);
```

---

## Publishing Events

Use **Publish Event** on the bus component reference. You always call this one node —
it handles client-to-server routing automatically.

### Blueprint

```
[Bus Reference] → Publish Event
    Payload Event Tag:  Event.MyGame.SomethingHappened
    Payload Instigator: Self
    Payload Target:     (optional)
    Payload Magnitude:  100.0
    Scope:              All
```

Use **Make Event Payload** (under **AP|EventBus|Utilities**) to construct payloads as a
pure node without splitting struct pins manually.

### C++

```cpp
FAP_EventPayload Payload;
Payload.EventTag   = FGameplayTag::RequestGameplayTag(FName("Event.MyGame.SomethingHappened"));
Payload.Instigator = this;
Payload.Magnitude  = 100.0f;

Bus->PublishEvent(Payload, EAP_EventScope::All);
```

### Transparent Client-to-Server Routing

You do **not** need to check authority or call different functions from client vs. server.
`PublishEvent` automatically detects if it's called on a client and routes to the server
via a Server RPC. One node works from anywhere.

---

## Delivery Scopes

| Scope | What fires |
|-------|------------|
| `All` | Server fires + NetMulticast to all connected clients |
| `OwnerOnly` | Server fires + Client RPC to the owning connection only |
| `ServerOnly` | Server fires locally only — no replication to any client |
| `LocalOnly` | Fires on the calling machine only — zero network cost. Use for UI-only events. |

---

## Listening for Events

There are two complementary approaches. You can use both simultaneously on the same component.

### Catch-All: `OnEventReceived`

Fires for **every** event published to the bus, regardless of tag.

**Blueprint** — In the Character or Actor Blueprint that cached the bus reference:
1. Drag off the bus reference → **Assign On Event Received** (or use Bind Event to)
2. Wire the generated event to your logic

**C++:**
```cpp
Bus->OnEventReceived.AddDynamic(this, &AMyActor::HandleAnyEvent);

void AMyActor::HandleAnyEvent(FGameplayTag EventTag, FAP_EventPayload Payload)
{
    // Called for every event on the bus
}
```

### Per-Tag Filtering: `ListenForEvent` + `OnListenedEventReceived`

Fires only for specific tags you register. Useful when an actor only cares about a subset
of bus traffic.

**Blueprint:**
1. Call **Listen for Event** with the tag you want to filter on
2. Bind **Assign On Listened Event Received** (or Bind Event to)
3. The bound event fires only when that specific tag is published

**C++:**
```cpp
Bus->ListenForEvent(FGameplayTag::RequestGameplayTag(FName("Event.MyGame.PlayerDied")));
Bus->OnListenedEventReceived.AddDynamic(this, &AMyActor::HandlePlayerDied);
```

Call `StopListeningForEvent(Tag)` or `StopAllListening()` to remove filters at any time.
`StopAllListening()` is called automatically in `EndPlay`.

### Dual Binding Note (Multiplayer)

In multiplayer, delegates fire via different paths on server vs. client:
- **Server** receives events via direct `DispatchEvent` or `MulticastPublishEvent_Implementation`
- **Client** receives events via `MulticastPublishEvent_Implementation` or `ClientDeliverEvent_Implementation`

If you need an actor to catch events on **both** network roles (e.g., a Character that runs
on both server and client), bind the delegates on both. BeginPlay fires on each machine
independently, so binding in BeginPlay naturally covers both.

---

## Late-Join Cache (Persistent Events)

Some events represent state that late-joining clients should receive immediately on connect
(e.g., a match phase change, an objective that was captured). Register these tags as
persistent so their last payload is cached server-side and delivered to new clients.

### Setup (in GameState BeginPlay — server only)

**Blueprint:**
```
Event BeginPlay
    → [AP_EventBus] Register Persistent Event (Tag = Event.MyGame.ObjectiveCaptured)
```

**C++:**
```cpp
EventBus->RegisterPersistentEvent(
    FGameplayTag::RequestGameplayTag(FName("Event.MyGame.ObjectiveCaptured")));
```

### How it works

1. Server calls `RegisterPersistentEvent(Tag)` at setup
2. Whenever `PublishEvent` fires for that tag, the server caches the last payload
3. When a new client connects, their `AP_EventBusComponent::BeginPlay` automatically calls
   `ServerRequestCachedEvents` (you don't need to do this manually)
4. The server responds with `ClientReceiveCachedEvent` for each cached tag
5. The client's `OnEventReceived` (and `OnListenedEventReceived` if registered) fire as if
   the event just happened — no special handling needed on the client side

### Cache Query API

```cpp
// Check if a persistent event has fired
bool bFired = Bus->WasEventFired(Tag);

// Get the last cached payload
FAP_EventPayload LastPayload;
bool bFound = Bus->GetLastPayload(Tag, LastPayload);

// Get all currently cached tags
FGameplayTagContainer CachedTags = Bus->GetAllCachedEventTags();
```

All three are **BlueprintPure** nodes (no execution pin) in Blueprint.

---

## Gameplay Tags

AP Event Bus uses Gameplay Tags as event identifiers. **No tags are shipped with the plugin.**
Buyers define their own tags in their project configuration.

To add tags: **Project Settings → Project → Gameplay Tags → Add New Gameplay Tag**

Recommended hierarchy: `Event.GameName.EventName` (e.g., `Event.MyGame.PlayerDied`)

---

## Full Example (Blueprint)

**GameState BeginPlay:**
```
BeginPlay
    → [AP_EventBus] Register Persistent Event (Tag = Event.Demo.MatchStarted)
    → Delay (2.0)
    → [AP_EventBus] Publish Event
          Payload Event Tag = Event.Demo.MatchStarted
          Scope             = All
```

**Character BeginPlay:**
```
BeginPlay
    → Delay (0.1)
    → Get Event Bus from Game State → (store as variable "EventBus")
    → [EventBus] Listen for Event (Tag = Event.Demo.MatchStarted)
    → [EventBus] Assign On Listened Event Received
          → Print String (payload details)
```

**Character Input (Key press):**
```
[EventBus] Publish Event
    Payload Event Tag = Event.Demo.PlayerAction
    Payload Instigator = Self
    Scope = All
```

---

## What the Plugin Does NOT Do

- No GameplayAbilities (GAS) dependency — fully standalone
- No cross-plugin dependencies
- No persistent state across level loads or game sessions — all cache is transient
- No listener priority ordering (planned for v1.1)
- No wildcard or parent-tag event matching (planned for v1.1)
- No timed or delayed events (planned for v1.2)
