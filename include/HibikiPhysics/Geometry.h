#pragma once

#include <HibikiPhysics/Plugin.h>

namespace HibikiPhysics
{
// The geometry module's service identity. Shape APIs are introduced with their implementations.
class GeometryLibrary
{
};

[[nodiscard]] const CapabilitySlot<GeometryLibrary> &GeometryLibrarySlot();
[[nodiscard]] CapabilityRegistration				 GeometryLibraryPlugin();
} // namespace HibikiPhysics
