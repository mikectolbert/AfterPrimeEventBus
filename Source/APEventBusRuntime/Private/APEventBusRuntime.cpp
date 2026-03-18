// Copyright AfterPrime Systems. All Rights Reserved.

#include "APEventBusRuntime.h"

DEFINE_LOG_CATEGORY(LogEventBus);

#define LOCTEXT_NAMESPACE "FAPEventBusRuntimeModule"

void FAPEventBusRuntimeModule::StartupModule()
{
}

void FAPEventBusRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAPEventBusRuntimeModule, APEventBusRuntime)
