// Copyright AfterPrime Systems. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AP_EventBusTypes.generated.h"

// ---------------------------------------------------------------------------
// EAP_EventScope — delivery scope for published events
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EAP_EventScope : uint8
{
	All         UMETA(DisplayName="All",        ToolTip="Server fires + NetMulticast to all clients"),
	OwnerOnly   UMETA(DisplayName="OwnerOnly",  ToolTip="Server fires + Client RPC to owning connection only"),
	ServerOnly  UMETA(DisplayName="ServerOnly", ToolTip="Server fires locally only, no replication"),
	LocalOnly   UMETA(DisplayName="LocalOnly",  ToolTip="Fires on calling machine only, zero network cost")
};

// ---------------------------------------------------------------------------
// FAP_EventPayload — self-contained event data
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct APEVENTBUSRUNTIME_API FAP_EventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus",
		meta=(ToolTip="Gameplay Tag identifying this event"))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus",
		meta=(ToolTip="The actor that initiated this event. May be null."))
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus",
		meta=(ToolTip="The target actor of this event. May be null."))
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus",
		meta=(ToolTip="General-purpose numeric value (damage, healing, distance, etc.)"))
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AP|EventBus",
		meta=(ToolTip="Additional context tags for filtering or metadata"))
	FGameplayTagContainer ContextTags;
};

// ---------------------------------------------------------------------------
// FAP_OnEventReceived — shared delegate signature for both delegate properties
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAP_OnEventReceived,
	FGameplayTag, EventTag,
	FAP_EventPayload, Payload);
