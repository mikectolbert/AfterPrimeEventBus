// Copyright AfterPrime Systems. All Rights Reserved.

#include "AP_EventBusBlueprintLibrary.h"
#include "AP_EventBusComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAP_EventBusComponent* UAP_EventBusBlueprintLibrary::GetEventBusComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	return Actor->FindComponentByClass<UAP_EventBusComponent>();
}

UAP_EventBusComponent* UAP_EventBusBlueprintLibrary::GetEventBusFromGameState(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
	{
		return nullptr;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!IsValid(GameState))
	{
		return nullptr;
	}

	return GetEventBusComponent(GameState);
}

void UAP_EventBusBlueprintLibrary::PublishEventOnBus(AActor* BusOwner, FAP_EventPayload Payload, EAP_EventScope Scope)
{
	UAP_EventBusComponent* Bus = GetEventBusComponent(BusOwner);
	if (IsValid(Bus))
	{
		Bus->PublishEvent(Payload, Scope);
	}
}

FAP_EventPayload UAP_EventBusBlueprintLibrary::MakeEventPayload(
	FGameplayTag EventTag,
	AActor* Instigator,
	AActor* Target,
	float Magnitude,
	FGameplayTagContainer ContextTags)
{
	FAP_EventPayload Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Instigator;
	Payload.Target = Target;
	Payload.Magnitude = Magnitude;
	Payload.ContextTags = ContextTags;
	return Payload;
}
