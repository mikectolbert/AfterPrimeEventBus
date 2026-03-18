// Copyright AfterPrime Systems. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "AP_EventBusTypes.h"
#include "AP_EventBusDemoCharacter.generated.h"

class UAP_EventBusComponent;

/**
 * AAP_EventBusDemoCharacter
 *
 * Demo character showcasing AP Event Bus usage.
 * Place on GameState: add UAP_EventBusComponent, register persistent events.
 * Reparent BP_AP_EventBus_DemoCharacter to this class.
 *
 * Key bindings (locally controlled only):
 *   1 — Publish Event.Demo.Damage  (All,        Magnitude=25, Instigator=self)
 *   2 — Publish Event.Demo.Heal    (All,        Magnitude=10, Instigator=self)
 *   3 — Publish Event.Demo.Alert   (ServerOnly, Magnitude=0,  Instigator=self)
 *   4 — Publish Event.Demo.UINotify(LocalOnly,  Magnitude=0,  Instigator=self)
 *
 * Note: demo tags are NOT shipped in a plugin .ini — they must be registered in
 * the project's Gameplay Tags config before use.
 */
UCLASS()
class APEVENTBUSRUNTIME_API AAP_EventBusDemoCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AAP_EventBusDemoCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	// -----------------------------------------------------------------------
	// Bus reference (cached in BeginPlay)
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category="AP|EventBus|Demo",
		meta=(ToolTip="Cached reference to the GameState's AP Event Bus component."))
	TObjectPtr<UAP_EventBusComponent> EventBus;

	// -----------------------------------------------------------------------
	// Input handlers
	// -----------------------------------------------------------------------

	/** Key 1: Publish Event.Demo.Damage — All scope, Magnitude=25 */
	void Input_PublishDamage();

	/** Key 2: Publish Event.Demo.Heal — All scope, Magnitude=10 */
	void Input_PublishHeal();

	/** Key 3: Publish Event.Demo.Alert — ServerOnly scope */
	void Input_PublishAlert();

	/** Key 4: Publish Event.Demo.UINotify — LocalOnly scope */
	void Input_PublishUINotify();

	// -----------------------------------------------------------------------
	// Event handlers (bound in BeginPlay)
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnEventReceived_Handler(FGameplayTag EventTag, FAP_EventPayload Payload);

	UFUNCTION()
	void OnListenedEventReceived_Handler(FGameplayTag EventTag, FAP_EventPayload Payload);
};
