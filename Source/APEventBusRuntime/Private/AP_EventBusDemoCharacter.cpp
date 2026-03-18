// Copyright AfterPrime Systems. All Rights Reserved.

#include "AP_EventBusDemoCharacter.h"
#include "AP_EventBusComponent.h"
#include "AP_EventBusBlueprintLibrary.h"
#include "APEventBusRuntime.h"
#include "GameplayTagsManager.h"
#include "Components/InputComponent.h"

AAP_EventBusDemoCharacter::AAP_EventBusDemoCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAP_EventBusDemoCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Cache a reference to the global bus on the GameState.
	EventBus = UAP_EventBusBlueprintLibrary::GetEventBusFromGameState(this);

	if (!IsValid(EventBus))
	{
		UE_LOG(LogEventBus, Warning,
			TEXT("AAP_EventBusDemoCharacter: No AP_EventBusComponent found on GameState. "
			     "Add UAP_EventBusComponent to your GameState Blueprint."));
		return;
	}

	// Bind catch-all delegate — fires for every event on the bus.
	EventBus->OnEventReceived.AddDynamic(this, &AAP_EventBusDemoCharacter::OnEventReceived_Handler);

	// Register tag filters for OnListenedEventReceived.
	EventBus->ListenForEvent(FGameplayTag::RequestGameplayTag(FName("Event.Demo.Damage")));
	EventBus->ListenForEvent(FGameplayTag::RequestGameplayTag(FName("Event.Demo.Heal")));
	EventBus->ListenForEvent(FGameplayTag::RequestGameplayTag(FName("Event.Demo.Alert")));

	// Bind filtered delegate — fires only for the tags registered above.
	EventBus->OnListenedEventReceived.AddDynamic(this, &AAP_EventBusDemoCharacter::OnListenedEventReceived_Handler);
}

void AAP_EventBusDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Only bind demo input on the locally controlled pawn.
	if (!IsLocallyControlled())
	{
		return;
	}

	PlayerInputComponent->BindAction("DemoPublishDamage",  IE_Pressed, this, &AAP_EventBusDemoCharacter::Input_PublishDamage);
	PlayerInputComponent->BindAction("DemoPublishHeal",    IE_Pressed, this, &AAP_EventBusDemoCharacter::Input_PublishHeal);
	PlayerInputComponent->BindAction("DemoPublishAlert",   IE_Pressed, this, &AAP_EventBusDemoCharacter::Input_PublishAlert);
	PlayerInputComponent->BindAction("DemoPublishUINotify",IE_Pressed, this, &AAP_EventBusDemoCharacter::Input_PublishUINotify);
}

// ---------------------------------------------------------------------------
// Input Handlers
// ---------------------------------------------------------------------------

void AAP_EventBusDemoCharacter::Input_PublishDamage()
{
	if (!IsValid(EventBus)) return;

	FAP_EventPayload Payload;
	Payload.EventTag  = FGameplayTag::RequestGameplayTag(FName("Event.Demo.Damage"));
	Payload.Instigator = this;
	Payload.Magnitude  = 25.0f;

	EventBus->PublishEvent(Payload, EAP_EventScope::All);
}

void AAP_EventBusDemoCharacter::Input_PublishHeal()
{
	if (!IsValid(EventBus)) return;

	FAP_EventPayload Payload;
	Payload.EventTag  = FGameplayTag::RequestGameplayTag(FName("Event.Demo.Heal"));
	Payload.Instigator = this;
	Payload.Magnitude  = 10.0f;

	EventBus->PublishEvent(Payload, EAP_EventScope::All);
}

void AAP_EventBusDemoCharacter::Input_PublishAlert()
{
	if (!IsValid(EventBus)) return;

	FAP_EventPayload Payload;
	Payload.EventTag  = FGameplayTag::RequestGameplayTag(FName("Event.Demo.Alert"));
	Payload.Instigator = this;

	EventBus->PublishEvent(Payload, EAP_EventScope::ServerOnly);
}

void AAP_EventBusDemoCharacter::Input_PublishUINotify()
{
	if (!IsValid(EventBus)) return;

	FAP_EventPayload Payload;
	Payload.EventTag  = FGameplayTag::RequestGameplayTag(FName("Event.Demo.UINotify"));
	Payload.Instigator = this;

	EventBus->PublishEvent(Payload, EAP_EventScope::LocalOnly);
}

// ---------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------

void AAP_EventBusDemoCharacter::OnEventReceived_Handler(FGameplayTag EventTag, FAP_EventPayload Payload)
{
	UE_LOG(LogEventBus, Log,
		TEXT("[AP EventBus Demo] OnEventReceived — Tag: %s | Magnitude: %.1f | Instigator: %s"),
		*EventTag.ToString(),
		Payload.Magnitude,
		IsValid(Payload.Instigator) ? *Payload.Instigator->GetName() : TEXT("None"));
}

void AAP_EventBusDemoCharacter::OnListenedEventReceived_Handler(FGameplayTag EventTag, FAP_EventPayload Payload)
{
	UE_LOG(LogEventBus, Log,
		TEXT("[AP EventBus Demo] OnListenedEventReceived — Tag: %s | Magnitude: %.1f | Instigator: %s"),
		*EventTag.ToString(),
		Payload.Magnitude,
		IsValid(Payload.Instigator) ? *Payload.Instigator->GetName() : TEXT("None"));
}
