#include <HibikiPhysics/IncrementalSpatialIndex.h>

namespace HibikiPhysics
{
const CapabilitySlot<IncrementalSpatialProvider> &IncrementalSpatialSlot()
{
	static const CapabilitySlot<IncrementalSpatialProvider> slot("spatial.incremental_overlap");
	return slot;
}

CapabilityRegistration SweepAndPruneUpdatePlugin()
{
	return IncrementalSpatialSlot().Factory("hibiki.sweep_and_prune", MakeSweepAndPruneUpdateProvider);
}

CapabilityRegistration FourWayUpdatePlugin()
{
	return IncrementalSpatialSlot().Factory("hibiki.four_way_bvh", MakeFourWayUpdateProvider);
}
} // namespace HibikiPhysics
