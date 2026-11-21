#include <HibikiPhysics/Geometry.h>
#include <iostream>

namespace
{
void ShowQueries(const HibikiPhysics::GeometryLibrary &inGeometry)
{
	const std::array<double, 3>			  point{2, 0, -0.2};
	const HibikiPhysics::RectangularPatch patch({1, 0.5});
	const auto							  nearest = inGeometry.ClosestPoint(patch, point);
	std::cout << "Plane signed distance (m): " << inGeometry.SignedDistance(HibikiPhysics::Plane{}, point)
			  << "\nInside half-space: " << std::boolalpha << inGeometry.Contains(HibikiPhysics::HalfSpace{}, point)
			  << "\nNearest patch point (m): " << nearest[0] << ' ' << nearest[1] << ' ' << nearest[2] << '\n';

	const HibikiPhysics::Capsule  capsule(0.2, 0.5);
	const HibikiPhysics::Cylinder cylinder(0.2, 0.5);
	const std::array<double, 3>	  tipPoint{0, 0, 0.6};
	std::cout << "Inside capsule: " << inGeometry.Contains(capsule, tipPoint)
			  << "\nInside cylinder: " << inGeometry.Contains(cylinder, tipPoint)
			  << "\nCapsule local AABB max Z (m): " << inGeometry.LocalBounds(capsule).GetMax()[2] << '\n';
}
} // namespace

int main()
{
	HibikiPhysics::CapabilityConfig config;
	config.Add(HibikiPhysics::GeometryLibraryPlugin());
	auto	   example = HibikiPhysics::MakePlugin(ShowQueries, HibikiPhysics::GeometryLibrarySlot());
	const auto scope = config.Mount(example);
}
