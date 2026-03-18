# Changelog

## [1.0.0] - Initial Release

- `UAP_EventBusComponent` with full pub/sub event messaging
- Gameplay Tag based event identification
- Four delivery scopes: All, OwnerOnly, ServerOnly, LocalOnly
- Transparent client-to-server routing — one `PublishEvent` node works from anywhere
- Late-join cache for persistent events via `RegisterPersistentEvent`
- Two-tier delegate system: catch-all (`OnEventReceived`) and filtered (`OnListenedEventReceived`)
- `UAP_EventBusBlueprintLibrary` with convenience helpers: `GetEventBusFromGameState`, `GetEventBusComponent`, `PublishEventOnBus`, `MakeEventPayload`
- Server-authoritative multiplayer replication via RPCs — no replicated UPROPERTYs
- Cache query API: `WasEventFired`, `GetLastPayload`, `GetAllCachedEventTags`
- Listener management: `ListenForEvent`, `StopListeningForEvent`, `StopAllListening`, `IsListeningForEvent`
- Demo character C++ class (`AAP_EventBusDemoCharacter`) with input-driven publish examples
- Blueprint-first: fully usable from Blueprint with no C++ required
- No GAS dependency — completely standalone
- Free product — no purchase required
