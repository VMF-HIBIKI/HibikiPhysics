#include <HibikiPhysics/Geometry.h>

namespace HibikiPhysics
{
const CapabilitySlot<GeometryLibrary> &GeometryLibrarySlot()
{
	static const CapabilitySlot<GeometryLibrary> slot("geometry.library");
	return slot;
}

CapabilityRegistration GeometryLibraryPlugin()
{
	return GeometryLibrarySlot().Factory("hibiki.geometry", []
										 { return std::make_unique<GeometryLibrary>(); });
}
} // namespace HibikiPhysics
