#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
// CPU linear BVH built from Morton-ordered primitive centers and stable caller IDs.
[[nodiscard]] std::unique_ptr<SpatialQueryProvider>	  MakeMortonSpatialQueryProvider();
[[nodiscard]] std::unique_ptr<SpatialOverlapProvider> MakeMortonSpatialOverlapProvider();
[[nodiscard]] CapabilityRegistration				  MortonSpatialQueryPlugin();
[[nodiscard]] CapabilityRegistration				  MortonSpatialOverlapPlugin();
} // namespace HibikiPhysics
