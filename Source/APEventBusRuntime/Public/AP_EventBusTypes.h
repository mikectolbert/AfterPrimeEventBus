// Copyright AfterPrime Systems. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AP_EventBusTypes.generated.h"

UENUM(BlueprintType)
enum class EAP_EventScope : uint8
{
	All         UMETA(DisplayName="All"),
	OwnerOnly   UMETA(DisplayName="OwnerOnly"),
	ServerOnly  UMETA(DisplayName="ServerOnly"),
	LocalOnly   UMETA(DisplayName="LocalOnly")
};

USTRUCT(BlueprintType)
struct FAP_EventPayload
{
	GENERATED_BODY()
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAP_OnEventReceived, FGameplayTag, EventTag, FAP_EventPayload, Payload);
