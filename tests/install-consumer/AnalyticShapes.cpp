#include <HibikiPhysics/Geometry.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>

namespace
{
using namespace HibikiPhysics;

void Check(bool inCondition, const char *inMessage)
{
	if (!inCondition)
		throw std::logic_error(inMessage);
}

template <class Error, class Operation>
void ExpectError(Operation inOperation)
{
	try
	{
		inOperation();
	}
	catch (const Error &)
	{
		return;
	}
	throw std::logic_error("Expected geometry operation to reject input");
}

void CheckQueries(const GeometryLibrary &inGeometry)
{
	const HalfSpace ground;
	const auto		plane = ground.GetBoundary();
	for (double z : {-0.2, 0.0, 0.2})
	{
		const std::array<double, 3> point{2, 0, z};
		Check(inGeometry.SignedDistance(plane, point) == z, "Plane has outward normal +Z");
		Check(inGeometry.Contains(ground, point) == (z <= 0), "Half-space includes its boundary");
	}
	const RectangularPatch patch({1, 0.5});
	Check(patch.GetHalfExtents() == std::array{1.0, 0.5}, "XY patch dimensions");
	Check(inGeometry.ClosestPoint(patch, {2, 0, 0}) == std::array{1.0, 0.0, 0.0}, "Coplanar point beyond edge");
	Check(inGeometry.ClosestPoint(patch, {0.5, -0.25, 3}) == std::array{0.5, -0.25, 0.0}, "Projection onto interior");
	for (double x : {-1.0, 1.0})
		for (double y : {-0.5, 0.5})
		{
			const std::array<double, 3> corner{x, y, 0};
			Check(inGeometry.ClosestPoint(patch, corner) == corner, "Closed patch includes corners");
			Check(inGeometry.ClosestPoint(patch, {2 * x, 2 * y, -3}) == corner, "Projection onto corner from below");
		}
	for (double invalid : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			std::array<double, 3> point{};
			point[axis] = invalid;
			ExpectError<std::invalid_argument>([&]
											   { (void)inGeometry.SignedDistance(plane, point); });
			ExpectError<std::invalid_argument>([&]
											   { (void)inGeometry.Contains(ground, point); });
			ExpectError<std::invalid_argument>([&]
											   { (void)inGeometry.ClosestPoint(patch, point); });
		}
}

void CheckDimensions()
{
	Check(Sphere(0.2).GetRadius() == 0.2, "Sphere radius");
	const auto box = Box({1, 0.5, 0.2}).GetHalfExtents();
	Check(std::array{2 * box[0], 2 * box[1], 2 * box[2]} == std::array{2.0, 1.0, 0.4}, "Box full lengths");
	const Capsule capsule(0.2, 0.5);
	const auto	  capsuleExtents = capsule.GetHalfExtents();
	Check(capsule.GetRadius() == 0.2 && 2 * capsule.GetHalfCylinderLength() == 1, "Capsule spine length");
	Check(capsuleExtents[0] == 0.2 && capsuleExtents[1] == 0.2 && std::abs(2 * capsuleExtents[2] - 1.4) < 1e-14, "Capsule Z length including caps");
	Check(Capsule(0.2, 0).GetHalfExtents() == std::array{0.2, 0.2, 0.2}, "Capsule spherical limit");
	const Cylinder cylinder(0.2, 0.5);
	Check(cylinder.GetRadius() == 0.2 && cylinder.GetHalfHeight() == 0.5 && cylinder.GetHalfExtents() == std::array{0.2, 0.2, 0.5}, "Cylinder ends at +/- half height on Z");
	Check(Ellipsoid({2, 0.5, 3}).GetSemiAxes() == std::array{2.0, 0.5, 3.0}, "Ellipsoid XYZ semi-axes");
}

void CheckBounds(const GeometryLibrary &g)
{
	using Point = std::array<double, 3>;
	const double inf = std::numeric_limits<double>::infinity();
	const auto	 bounds = [](const AABB &box, Point min, Point max)
	{
		Check(box.GetMin() == min && box.GetMax() == max, "Local AABB endpoints");
	};
	bounds(g.LocalBounds(Plane{}), {-inf, -inf, 0}, {inf, inf, 0});
	bounds(g.LocalBounds(HalfSpace{}), {-inf, -inf, -inf}, {inf, inf, 0});
	bounds(g.LocalBounds(RectangularPatch({2, 1})), {-2, -1, 0}, {2, 1, 0});
	bounds(g.LocalBounds(Sphere(2)), {-2, -2, -2}, {2, 2, 2});
	bounds(g.LocalBounds(Box({1, 2, 3})), {-1, -2, -3}, {1, 2, 3});
	bounds(g.LocalBounds(Capsule(0.25, 0.5)), {-0.25, -0.25, -0.75}, {0.25, 0.25, 0.75});
	bounds(g.LocalBounds(Capsule(0.25, 0)), {-0.25, -0.25, -0.25}, {0.25, 0.25, 0.25});
	bounds(g.LocalBounds(Cylinder(1, 2)), {-1, -1, -2}, {1, 1, 2});
	bounds(g.LocalBounds(Ellipsoid({2, 3, 1})), {-2, -3, -1}, {2, 3, 1});
	const auto patch = g.LocalBounds(RectangularPatch({2, 1}));
	Check(patch.Contains({-2, 1, 0}) && !patch.Contains({0, 0, 0.01}), "Zero thickness includes edges");
	Check(g.LocalBounds(HalfSpace{}).Contains({1e300, -1e300, -1e300}) && !g.LocalBounds(Plane{}).Contains({0, 0, -1}), "Unbounded intervals");
	const double tiny = std::numeric_limits<double>::epsilon() / 4;
	Check(g.LocalBounds(Capsule(tiny, 1)).GetMax()[2] == std::nextafter(1.0, inf), "Capsule AABB encloses rounded axial sum");
	ExpectError<std::overflow_error>([&]
									 { (void)g.LocalBounds(Capsule(1, std::numeric_limits<double>::max())); });
	ExpectError<std::invalid_argument>([]
									   { (void)AABB({0, 2, 0}, {1, 1, 1}); });
	ExpectError<std::invalid_argument>([&]
									   { (void)AABB({inf, 0, 0}, {inf, 0, 0}); });
	ExpectError<std::invalid_argument>([]
									   { (void)AABB({0, 0, 0}, {0, 0, std::numeric_limits<double>::quiet_NaN()}); });
}

void CheckSolids(const GeometryLibrary &g)
{
	const Sphere	sphere(5);
	const Box		box({1, 2, 3});
	const Cylinder	cylinder(0.2, 0.5);
	const Capsule	capsule(0.2, 0.5);
	const Ellipsoid ellipsoid({2, 1, 0.5});
	Check(g.Contains(sphere, {3, 4, 0}) && !g.Contains(sphere, {4, 4, 0}), "Sphere curved boundary");
	Check(g.Contains(box, {-1, 2, 3}) && !g.Contains(box, {0, -2, 3.01}), "Box corner");
	Check(g.Contains(cylinder, {0.2, 0, -0.5}) && !g.Contains(cylinder, {0.2, 0.2, 0}), "Cylinder rim and radial exterior");
	for (double sign : {-1.0, 1.0})
	{
		const std::array<double, 3> point{0, 0, sign * 0.6};
		Check(g.Contains(capsule, point) && !g.Contains(cylinder, point), "Capsule cap beyond cylinder end");
		Check(g.Contains(Capsule(0.25, 0.5), {0, 0, sign * 0.75}), "Closed capsule tip");
	}
	Check(!g.Contains(capsule, {0.19, 0, 0.65}) && g.LocalBounds(capsule).Contains({0.19, 0, 0.65}), "Bounding box and curved cap");
	Check(g.Contains(ellipsoid, {0, 0, -0.5}) && g.Contains(ellipsoid, {1.0, 0.5, 0.25}) && !g.Contains(ellipsoid, {1.5, 0.75, 0}), "Ellipsoid semi-axes");
	for (const auto &p : std::array<std::array<double, 3>, 4>{{{0, 0, 0}, {3, 4, 0}, {0, 0, -5}, {4, 4, 1}}})
		Check(g.Contains(Capsule(5, 0), p) == g.Contains(sphere, p), "Degenerate capsule agrees with sphere");
	const double small = std::numeric_limits<double>::min(), large = std::numeric_limits<double>::max() / 2;
	for (double r : {small, large})
		Check(g.Contains(Sphere(r), {0, 0, r}) && !g.Contains(Sphere(r), {r, r, 0}) && !g.Contains(Ellipsoid({r, r, r}), {0.9 * r, 0.9 * r, 0}), "Extreme finite scale");
	for (double bad : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
	{
		const std::array<double, 3> p{0, 0, bad};
		ExpectError<std::invalid_argument>([&]
										   { (void)g.Contains(sphere, p); });
		ExpectError<std::invalid_argument>([&]
										   { (void)g.Contains(box, p); });
		ExpectError<std::invalid_argument>([&]
										   { (void)g.Contains(capsule, p); });
		ExpectError<std::invalid_argument>([&]
										   { (void)g.Contains(cylinder, p); });
		ExpectError<std::invalid_argument>([&]
										   { (void)g.Contains(ellipsoid, p); });
		ExpectError<std::invalid_argument>([&]
										   { (void)g.LocalBounds(Plane{}).Contains(p); });
	}
}

void CheckWorldContainment(const GeometryLibrary &g)
{
	// This exact rotation takes local X to world Y, Y to Z and Z to X.
	const Transform pose({3, 2, -4}, Quat::FromXYZW({0.5, 0.5, 0.5, 0.5}));
	const auto		check = [&](const auto &shape, Vec3 inside, Vec3 boundary, Vec3 outside)
	{
		for (Vec3 point : {inside, boundary, outside})
			Check(g.Contains(shape, pose, pose.TransformPoint(point)) == g.Contains(shape, {point.mX, point.mY, point.mZ}), "World and local containment agree");
		Check(g.Contains(shape, pose, pose.TransformPoint(inside)), "Interior of placed shape");
		Check(g.Contains(shape, pose, pose.TransformPoint(boundary)), "Closed boundary of placed shape");
		Check(!g.Contains(shape, pose, pose.TransformPoint(outside)), "Exterior of placed shape");
	};
	check(HalfSpace{}, {3, -2, -1}, {3, -2, 0}, {3, -2, 0.25});
	check(Sphere(1), {0.25, 0.5, 0}, {0, -1, 0}, {0.75, 0.75, 0});
	check(Box({1, 0.5, 0.25}), {0.75, 0.25, 0}, {-1, 0.5, 0.25}, {0, 0.75, 0});
	check(Capsule(0.5, 1), {0, 0, 1.25}, {0, 0, -1.5}, {0.5, 0, 1.25});
	check(Cylinder(0.5, 1), {0.25, 0, 0.75}, {0.5, 0, 1}, {0, 0, 1.25});
	check(Ellipsoid({2, 1, 0.5}), {1, 0.5, 0.25}, {-2, 0, 0}, {1.5, 0.75, 0});
	const double nan = std::numeric_limits<double>::quiet_NaN();
	ExpectError<std::invalid_argument>([&]
									   { (void)g.Contains(HalfSpace{}, pose, {nan, 0, 0}); });
}

void CheckCornerBounds(const GeometryLibrary &g, const Box &box, const Transform &pose)
{
	const AABB bounds = g.WorldBounds(box, pose);
	const auto [hx, hy, hz] = box.GetHalfExtents();
	const double		  inf = std::numeric_limits<double>::infinity();
	std::array<double, 3> low{inf, inf, inf}, high{-inf, -inf, -inf};
	for (double x : {-hx, hx})
		for (double y : {-hy, hy})
			for (double z : {-hz, hz})
			{
				const Vec3		 world = pose.TransformPoint({x, y, z});
				const std::array corner{world.mX, world.mY, world.mZ};
				Check(bounds.Contains(corner), "World AABB encloses each transformed corner");
				for (std::size_t i = 0; i < 3; ++i)
				{
					low[i] = std::min(low[i], corner[i]);
					high[i] = std::max(high[i], corner[i]);
				}
			}
	for (std::size_t i = 0; i < 3; ++i)
	{
		Check(std::abs(bounds.GetMin()[i] - low[i]) <= 1e-12, "World AABB minimum matches corner extrema");
		Check(std::abs(bounds.GetMax()[i] - high[i]) <= 1e-12, "World AABB maximum matches corner extrema");
	}
}

void CheckWorldBox(const GeometryLibrary &g)
{
	const Box		box({1, 0.5, 0.2});
	const Vec3		centre{3, 2, 0};
	const Transform quarterTurn(centre, Quat::FromAxisAngle({0, 0, 1}, std::numbers::pi / 2));
	const Transform eighthTurn(centre, Quat::FromAxisAngle({0, 0, 1}, std::numbers::pi / 4));
	for (const auto &pose : {Transform{}, Transform(centre, {}), quarterTurn, eighthTurn, Transform({-2, 0.75, 3}, Quat::FromAxisAngle({1, -2, 3}, 1.13))})
		CheckCornerBounds(g, box, pose);

	const auto local = g.LocalBounds(box);
	const auto identity = g.WorldBounds(box, {});
	Check(identity.GetMin() == local.GetMin() && identity.GetMax() == local.GetMax(), "Identity preserves local bounds");
	Check(g.Contains(box, quarterTurn, {3.25, 2.75, 0.1}), "Inside the quarter-turned box");
	Check(g.Contains(box, quarterTurn, {3, 3, 0}), "Centre of the rotated +X face");
	Check(g.WorldBounds(box, eighthTurn).Contains({4, 1, 0}), "45-degree AABB includes the sample point");
	Check(!g.Contains(box, eighthTurn, {4, 1, 0}), "45-degree box excludes the same point");

	const double	max = std::numeric_limits<double>::max();
	const Transform far({max, 0, 0}, {});
	ExpectError<std::overflow_error>([&]
									 { (void)g.WorldBounds(Box({max, 1, 1}), far); });
	ExpectError<std::overflow_error>([&]
									 { (void)g.Contains(box, far, {-max, 0, 0}); });
}

void CheckWorldFiniteBounds(const GeometryLibrary &g)
{
	const Vec3		centre{3, -2, 1};
	const Transform turn(centre, Quat::FromAxisAngle({0, 1, 0}, std::numbers::pi / 4));
	const auto		checkExtent = [&](const auto &shape, std::array<double, 3> half)
	{
		const auto		 bounds = g.WorldBounds(shape, turn);
		const std::array offset{centre.mX, centre.mY, centre.mZ};
		for (std::size_t i = 0; i < 3; ++i)
		{
			Check(std::abs(bounds.GetMin()[i] - (offset[i] - half[i])) < 1e-12, "Tilted shape minimum");
			Check(std::abs(bounds.GetMax()[i] - (offset[i] + half[i])) < 1e-12, "Tilted shape maximum");
		}
		const auto identity = g.WorldBounds(shape, {}), local = g.LocalBounds(shape);
		Check(identity.GetMin() == local.GetMin() && identity.GetMax() == local.GetMax(), "Identity pose reproduces local bounds");
	};
	const double root2 = std::numbers::sqrt2;
	checkExtent(RectangularPatch({2, 3}), {root2, 3, root2});
	checkExtent(Capsule(0.5, 2), {root2 + 0.5, 0.5, root2 + 0.5});
	checkExtent(Cylinder(1, 2), {3 / root2, 1, 3 / root2});
	checkExtent(Ellipsoid({3, 2, 1}), {std::sqrt(5.0), 2, std::sqrt(5.0)});

	const Sphere sphere(2);
	for (const auto &rotation : {Quat{}, turn.GetRotation(), Quat::FromAxisAngle({1, -2, 3}, 1.13)})
	{
		const Transform pose(centre, rotation);
		const auto		bounds = g.WorldBounds(sphere, pose);
		Check(bounds.GetMin() == std::array{1., -4., -1.} && bounds.GetMax() == std::array{5., 0., 3.}, "Sphere bounds are rotation invariant");
		const auto capsule = g.WorldBounds(Capsule(2, 0), pose);
		Check(capsule.GetMin() == bounds.GetMin() && capsule.GetMax() == bounds.GetMax(), "Zero-spine capsule agrees with sphere");
	}
	const Capsule thin(std::numeric_limits<double>::epsilon() / 4, 1);
	Check(g.WorldBounds(thin, {}).GetMax() == g.LocalBounds(thin).GetMax(), "Capsule projection preserves outward rounding");
	const double max = std::numeric_limits<double>::max();
	ExpectError<std::overflow_error>([&]
									 { (void)g.WorldBounds(Box({max, 1, max}), turn); });
}

void CheckWorldUnboundedIntervals(const GeometryLibrary &g)
{
	using Point = std::array<double, 3>;
	const double inf = std::numeric_limits<double>::infinity();
	const Vec3	 centre{3, -2, 1};
	const auto	 check = [](const AABB &bounds, Point low, Point high)
	{
		Check(bounds.GetMin() == low && bounds.GetMax() == high, "Unbounded AABB endpoints and signs");
	};
	const Transform translated(centre, {});
	check(g.WorldBounds(Plane{}, translated), {-inf, -inf, 1}, {inf, inf, 1});
	check(g.WorldBounds(HalfSpace{}, translated), {-inf, -inf, -inf}, {inf, inf, 1});

	// Exact signed permutations map the local normal to +X and -X.
	const Transform positiveX(centre, Quat::FromXYZW({0.5, 0.5, 0.5, 0.5}));
	const Transform negativeX(centre, Quat::FromXYZW({-0.5, -0.5, 0.5, 0.5}));
	check(g.WorldBounds(Plane{}, positiveX), {3, -inf, -inf}, {3, inf, inf});
	check(g.WorldBounds(Plane{}, negativeX), {3, -inf, -inf}, {3, inf, inf});
	check(g.WorldBounds(HalfSpace{}, positiveX), {-inf, -inf, -inf}, {3, inf, inf});
	check(g.WorldBounds(HalfSpace{}, negativeX), {3, -inf, -inf}, {inf, inf, inf});

	for (const auto &rotation : {Quat::FromAxisAngle({1, 2, 3}, 0.7), Quat::FromAxisAngle({0, 1, 0}, 1e-14)})
	{
		const Transform tilted(centre, rotation);
		check(g.WorldBounds(Plane{}, tilted), {-inf, -inf, -inf}, {inf, inf, inf});
		check(g.WorldBounds(HalfSpace{}, tilted), {-inf, -inf, -inf}, {inf, inf, inf});
	}
}

void CheckInvalidDimensions()
{
	for (double bad : {0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
	{
		ExpectError<std::invalid_argument>([&]
										   { (void)Sphere(bad); });
		ExpectError<std::invalid_argument>([&]
										   { (void)Cylinder(bad, 1); });
		ExpectError<std::invalid_argument>([&]
										   { (void)Cylinder(1, bad); });
		ExpectError<std::invalid_argument>([&]
										   { (void)Capsule(bad, 1); });
		if (bad != 0)
			ExpectError<std::invalid_argument>([&]
											   { (void)Capsule(1, bad); });
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			std::array<double, 3> dims{1, 1, 1};
			dims[axis] = bad;
			ExpectError<std::invalid_argument>([&]
											   { (void)Box{dims}; });
			ExpectError<std::invalid_argument>([&]
											   { (void)Ellipsoid{dims}; });
			if (axis < 2)
				ExpectError<std::invalid_argument>([&]
												   { (void)RectangularPatch({dims[0], dims[1]}); });
		}
	}
	const double max = std::numeric_limits<double>::max();
	ExpectError<std::overflow_error>([&]
									 { (void)Capsule(max, max).GetHalfExtents(); });
}
} // namespace

int main()
{
	try
	{
		CheckDimensions();
		CheckInvalidDimensions();
		HibikiPhysics::CapabilityConfig config;
		config.Add(HibikiPhysics::GeometryLibraryPlugin());
		auto		consumer = HibikiPhysics::MakePlugin(CheckQueries, HibikiPhysics::GeometryLibrarySlot());
		const auto	scope = config.Mount(consumer);
		const auto &geometry = HibikiPhysics::GeometryLibrarySlot().Get(*scope);
		CheckBounds(geometry);
		CheckSolids(geometry);
		CheckWorldContainment(geometry);
		CheckWorldBox(geometry);
		CheckWorldFiniteBounds(geometry);
		CheckWorldUnboundedIntervals(geometry);
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
