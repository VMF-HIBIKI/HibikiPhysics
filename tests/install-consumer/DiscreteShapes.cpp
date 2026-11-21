#include <HibikiPhysics/Geometry.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace HibikiPhysics;

namespace
{
void Check(bool result, const char *message)
{
	if (!result)
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
	throw std::logic_error("Expected invalid_argument");
}

void VerifyBounds(const GeometryLibrary &g, const auto &shape, const auto &vertices, const Transform &pose)
{
	const auto			  box = g.WorldBounds(shape, pose);
	std::array<double, 3> low = {INFINITY, INFINITY, INFINITY}, high = {-INFINITY, -INFINITY, -INFINITY};
	for (Vec3 v : vertices)
	{
		const auto [x, y, z] = pose.TransformPoint(v);
		const std::array values{x, y, z};
		Check(box.Contains(values), "Transformed bounds enclose every transformed vertex");
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			low[axis] = std::min(low[axis], values[axis]);
			high[axis] = std::max(high[axis], values[axis]);
		}
	}
	for (std::size_t axis = 0; axis < 3; ++axis)
		Check(std::abs(low[axis] - box.GetMin()[axis]) < 1e-12 && std::abs(high[axis] - box.GetMax()[axis]) < 1e-12, "Tight transformed vertex bounds");
}

void Queries()
{
	GeometryLibrary	  g;
	const Transform	  pose({3, -4, 8}, Quat::FromAxisAngle({1, 2, 3}, 0.8));
	std::vector<Vec3> p{{-1, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}};
	Triangle		  triangle(p[2], p[1], p[0]);
	ConvexHull		  hull(p);
	auto			  meshVertices = p;
	meshVertices.push_back({999, 999, 999});
	TriangleMesh mesh(meshVertices, {{0, 1, 2}, {1, 2, 3}, {0, 3, 1}, {2, 3, 0}});
	TriangleMesh open(meshVertices, {{2, 1, 0}});
	for (const Vec3 q : {p[1], Vec3{0.5, 0, 0}, Vec3{0, 0.5, 0}})
	{
		const std::array point{q.mX, q.mY, q.mZ};
		Check(g.Contains(triangle, point) && g.Contains(open, point) && g.Contains(mesh, point) && g.Contains(hull, point), "Closed surface boundaries");
		const auto world = pose.TransformPoint(q);
		Check(g.Contains(triangle, pose, world) && g.Contains(open, pose, world) && g.Contains(mesh, pose, world) && g.Contains(hull, pose, world), "Posed boundaries");
	}
	const ConvexHull   copiedHull = hull;
	const TriangleMesh copiedMesh = mesh;
	Check(g.Contains(copiedHull, {0, 0, 0}) && g.Contains(hull, {0, 0, 0}), "Hull copies share queryable geometry");
	Check(g.Contains(copiedMesh, {0, 0, 0}) && g.Contains(mesh, {0, 0, 0}), "Mesh copies share queryable geometry");
	const double below = std::nextafter(0.0, -1.0);
	Check(!g.Contains(triangle, {0, 0.5, below}) && !g.Contains(open, {0, 0.5, below}) && !g.Contains(hull, {0, 0.5, below}), "Strict local AABB rejection");
	Check(!g.Contains(hull, {}, {0, 0.5, below}), "Strict identity-pose AABB rejection");
	for (double z : {-0.001, 0.001})
		Check(!g.Contains(triangle, {0, 0.5, z}) && !g.Contains(open, {0, 0.5, z}), "Both sides of triangle are empty");
	Check(!g.Contains(triangle, {2, 2, 0}), "Coplanar outside");
	Check(g.Contains(hull, {0, 0.25, 0.25}) && !g.Contains(mesh, {0, 0.25, 0.25}), "Hull interior and closed mesh surface");
	Check(!g.Contains(hull, {1.5, 1.5, 1.5}), "Outside slanted hull face");
	Check(g.LocalBounds(triangle).GetMax() == std::array{2., 2., 0.}, "Triangle local bounds");
	Check(g.LocalBounds(mesh).GetMax() == std::array{2., 2., 2.}, "Unused mesh vertex excluded from bounds");
	Check(g.LocalBounds(hull).GetMin() == std::array{-1., 0., 0.}, "Hull local bounds");
	VerifyBounds(g, triangle, triangle.GetVertices(), pose);
	VerifyBounds(g, hull, p, pose);
	VerifyBounds(g, mesh, p, pose);

	std::vector<Vec3> cube{{0, 0, 0}, {0, 0, 0}};
	for (double x : {-1., 1.})
		for (double y : {-1., 1.})
			for (double z : {-1., 1.})
				cube.push_back({x, y, z});
	for (int order = 0; order < 2; ++order)
	{
		ConvexHull solid(cube);
		for (Vec3 v : cube)
			Check(g.Contains(solid, {v.mX, v.mY, v.mZ}), "All supplied cube points in hull");
		Check(g.Contains(solid, {0, 0.5, 1}) && !g.Contains(solid, {0, 0.5, 1.01}), "Coplanar merged hull facets");
		std::reverse(cube.begin(), cube.end());
	}
	const double nan = std::numeric_limits<double>::quiet_NaN();
	Invalid([&]
			{ (void)g.Contains(mesh, {nan, 0, 0}); });
	Invalid([&]
			{ (void)g.Contains(triangle, pose, {0, nan, 0}); });
	Invalid([&]
			{ (void)g.Contains(hull, {0, 0, nan}); });
	Invalid([&]
			{ Triangle t(p[0], p[0], p[1]); });
	Invalid([&]
			{ Triangle t(p[0], p[1], {4, 0, 0}); });
	Invalid([&]
			{ TriangleMesh m(p, {}); });
	Invalid([&]
			{ TriangleMesh m(p, {{1, 2, 4}}); });
	Invalid([&]
			{ TriangleMesh m(p, {{1, 2, 1}}); });
	Invalid([&]
			{ ConvexHull h({p[0], p[1], p[2], {0, -1, 0}}); });
	Invalid([&]
			{ ConvexHull h({}); });
}
} // namespace
int main()
{
	try
	{
		Queries();
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
