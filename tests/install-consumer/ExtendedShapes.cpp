#include <HibikiPhysics/Geometry.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::logic_error(message);
}

void Invalid(auto operation)
{
	try
	{
		operation();
	}
	catch (const std::invalid_argument &)
	{
		return;
	}
	throw std::logic_error("Expected invalid geometry input");
}

void CheckQueries(const GeometryLibrary &g)
{
	const Cone			 cone(2, 3);
	const Frustum		 frustum(1, 3, 2);
	const TaperedCapsule taper(1, 3, 2);
	Check(g.Contains(cone, {2, 0, -3}) && g.Contains(cone, {0, 0, 3}) && !g.Contains(cone, {1e-15, 0, 3}), "Cone base and apex");
	Check(g.Contains(frustum, {2, 0, 0}) && !g.Contains(frustum, {2.01, 0, 0}), "Frustum side");
	Check(g.Contains(Frustum(0, 2, 3), {0, 0, -3}) && !g.Contains(Frustum(0, 2, 3), {0.1, 0, -3}), "Reversed apex");
	Check(g.Contains(Frustum(2, 2, 3), {2, 0, 1}), "Equal radius cylinder");
	Check(g.Contains(taper, {2.3, 0, 0}) && !g.Contains(taper, {2.32, 0, 0}), "Common tangent between endpoint balls");
	Check(g.Contains(TaperedCapsule(3, 1, 2), {2.3, 0, 0}), "Reversed taper");
	Check(g.Contains(taper, {0, 0, -3}) && g.Contains(taper, {0, 0, 5}) && !g.Contains(taper, {0.1, 0, 5}), "Spherical tips");
	Check(g.Contains(TaperedCapsule(1, 2, 0), {2, 0, 0}) && !g.Contains(TaperedCapsule(3, 1, 0.5), {0, 0, 2.6}), "Coincident and nested balls");
	Check(g.Contains(TaperedCapsule(2, 2, 3), {2, 0, -1}), "Equal radius capsule");

	const auto bounds = g.LocalBounds(taper);
	Check(bounds.GetMin() == std::array{-3., -3., -3.} && bounds.GetMax() == std::array{3., 3., 5.}, "Asymmetric capsule bounds");
	Check(g.LocalBounds(cone).GetMin() == std::array{-2., -2., -3.} && g.LocalBounds(frustum).GetMax() == std::array{3., 3., 2.}, "Cone and frustum bounds");
	const Transform pose({3, -1, 4}, Quat::FromAxisAngle({0, 1, 0}, std::numbers::pi / 4));
	auto			world = [&](const auto &shape, double minX, double maxX)
	{
		auto box = g.WorldBounds(shape, pose);
		Check(std::abs(box.GetMin()[0] - minX) < 1e-12 && std::abs(box.GetMax()[0] - maxX) < 1e-12, "Oblique tight support bounds");
		Check(g.Contains(shape, pose, pose.TransformPoint({0, 0, 0})) && !g.Contains(shape, pose, pose.TransformPoint({10, 0, 0})), "World containment");
	};
	const double invRoot2 = 1 / std::sqrt(2.);
	world(cone, 3 - 5 * invRoot2, 3 + 3 * invRoot2);
	world(frustum, 3 - 3 * invRoot2, 3 + 5 * invRoot2);
	world(taper, 3 - 2 * invRoot2 - 1, 3 + 2 * invRoot2 + 3);

	const double max = std::numeric_limits<double>::max(), nan = std::numeric_limits<double>::quiet_NaN();
	Check(!g.Contains(TaperedCapsule(1e-200, 3e-200, max), {2.1e-200, 0, 0}), "Underflowing spine slope preserves the midpoint radius");
	Check(g.Contains(Frustum(1, 3, max), {2, 0, 0}) && g.Contains(TaperedCapsule(1, 3, max), {1.9, 0, 0}), "Full height may exceed double range");
	Invalid([]
			{ (void)Cone(0, 1); });
	Invalid([]
			{ (void)Frustum(0, 0, 1); });
	Invalid([&]
			{ (void)Frustum(1, nan, 1); });
	Invalid([]
			{ (void)TaperedCapsule(1, 3, -1); });
	Invalid([&]
			{ (void)g.Contains(taper, {0, nan, 0}); });
}
} // namespace

int main()
{
	try
	{
		HibikiPhysics::CapabilityConfig config;
		config.Add(HibikiPhysics::GeometryLibraryPlugin());
		auto consumer = HibikiPhysics::MakePlugin(CheckQueries, HibikiPhysics::GeometryLibrarySlot());
		(void)config.Mount(consumer);
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
