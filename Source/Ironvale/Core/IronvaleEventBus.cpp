// =============================================================================
// IronvaleEventBus.cpp — Central event bus implementation
// Project Ironvale
//
// The event bus is a UWorldSubsystem with no tick logic — it's purely a
// delegate holder. All logic lives in the systems that broadcast/subscribe.
// =============================================================================

#include "Core/IronvaleEventBus.h"

// Intentionally minimal — the event bus is a delegate container.
// Systems bind to delegates in their BeginPlay/Initialize and unbind in EndPlay/Deinitialize.
// No tick or update logic needed here.
