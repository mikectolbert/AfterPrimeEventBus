// Copyright AfterPrime Systems. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AP_EventBusComponent.h"
#include "AP_EventBusTypes.h"
#include "GameplayTagsManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------
// Helper — build a minimal valid payload for test use
// ---------------------------------------------------------------------------

static FAP_EventPayload MakeTestPayload(const FName& TagName, float Magnitude = 1.0f)
{
	FAP_EventPayload Payload;
	Payload.EventTag  = FGameplayTag::RequestGameplayTag(TagName);
	Payload.Magnitude = Magnitude;
	return Payload;
}

// ---------------------------------------------------------------------------
// T01 — PublishEvent dispatches to OnEventReceived
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_PublishFiresOnEventReceived,
	"AfterPrime.EventBus.PublishEvent.FiresOnEventReceived",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_PublishFiresOnEventReceived::RunTest(const FString& Parameters)
{
	// STUB: Create a UAP_EventBusComponent on a transient actor, bind OnEventReceived,
	// call PublishEvent(LocalOnly), verify the delegate fired exactly once with the
	// correct EventTag and Magnitude.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_PublishFiresOnEventReceived not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T02 — ListenForEvent + matching publish fires OnListenedEventReceived
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_ListenForMatchingTagFires,
	"AfterPrime.EventBus.Listeners.MatchingTagFiresOnListenedEventReceived",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_ListenForMatchingTagFires::RunTest(const FString& Parameters)
{
	// STUB: Call ListenForEvent(Tag), bind OnListenedEventReceived, publish with that tag
	// (LocalOnly), verify OnListenedEventReceived fired.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_ListenForMatchingTagFires not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T03 — ListenForEvent + non-matching publish does NOT fire OnListenedEventReceived
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_ListenForNonMatchingTagDoesNotFire,
	"AfterPrime.EventBus.Listeners.NonMatchingTagDoesNotFireOnListenedEventReceived",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_ListenForNonMatchingTagDoesNotFire::RunTest(const FString& Parameters)
{
	// STUB: Call ListenForEvent(TagA), bind OnListenedEventReceived, publish with TagB,
	// verify OnListenedEventReceived did NOT fire.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_ListenForNonMatchingTagDoesNotFire not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T04 — StopListeningForEvent prevents further filtered dispatch
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_StopListeningPreventsFilteredDispatch,
	"AfterPrime.EventBus.Listeners.StopListeningPreventsFilteredDispatch",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_StopListeningPreventsFilteredDispatch::RunTest(const FString& Parameters)
{
	// STUB: ListenForEvent(Tag) → verify fires → StopListeningForEvent(Tag) →
	// publish again → verify OnListenedEventReceived does NOT fire second time.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_StopListeningPreventsFilteredDispatch not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T05 — RegisterPersistentEvent + PublishEvent stores in cache
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_RegisterPersistentEventCachesPayload,
	"AfterPrime.EventBus.Cache.RegisterPersistentEventCachesPayload",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_RegisterPersistentEventCachesPayload::RunTest(const FString& Parameters)
{
	// STUB: On an authority component, call RegisterPersistentEvent(Tag), then
	// PublishEvent(Payload, LocalOnly), verify WasEventFired(Tag) returns true.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_RegisterPersistentEventCachesPayload not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T06 — WasEventFired returns correct state
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_WasEventFiredReturnsCorrectState,
	"AfterPrime.EventBus.Query.WasEventFiredReturnsCorrectState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_WasEventFiredReturnsCorrectState::RunTest(const FString& Parameters)
{
	// STUB: Verify WasEventFired returns false before publish, true after publish
	// (for a persistent tag), and false after UnregisterPersistentEvent.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_WasEventFiredReturnsCorrectState not yet implemented."));
	return true;
}

// ---------------------------------------------------------------------------
// T07 — GetLastPayload returns correct cached payload
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAP_EventBus_GetLastPayloadReturnsCachedPayload,
	"AfterPrime.EventBus.Query.GetLastPayloadReturnsCachedPayload",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAP_EventBus_GetLastPayloadReturnsCachedPayload::RunTest(const FString& Parameters)
{
	// STUB: RegisterPersistentEvent(Tag), publish payload with Magnitude=42,
	// call GetLastPayload(Tag, Out), verify Out.Magnitude == 42 and return is true.
	// Publish again with Magnitude=99, verify cache updates to 99.
	//
	// TODO: Implement once test world harness is available.
	AddWarning(TEXT("STUB — FAP_EventBus_GetLastPayloadReturnsCachedPayload not yet implemented."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
