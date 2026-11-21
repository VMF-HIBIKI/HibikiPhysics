#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
// CPU sweep-and-prune over finite closed AABBs. Both operations return sorted IDs or ID pairs.
[[nodiscard]] CapabilityRegistration				  SweepAndPrunePlugin();
[[nodiscard]] std::unique_ptr<SpatialOverlapProvider> MakeSweepAndPruneProvider();
} // namespace HibikiPhysics
