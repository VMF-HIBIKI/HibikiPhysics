#include <HibikiPhysics/ClosedMeshSdf.h>
#include <HibikiPhysics/Geometry.h>
#include "../fixtures/ThroughHolePlate.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>

using namespace HibikiPhysics;

namespace
{
using enum SolidPointLocation;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

void ExpectLocation(const ClosedMeshSdf &solid, Vec3 p, SolidPointLocation location)
{
	for (auto axis : {RayAxis::X, RayAxis::Y, RayAxis::Z})
		Check(solid.Classify(p, axis) == location, "classification differs from expected on a cardinal ray");
	const double distance = solid.SignedDistance(p);
	if (!(location == Boundary ? distance == 0 && !std::signbit(distance) : (location == Interior ? distance < 0 : distance > 0)))
	{
		std::cerr << std::hexfloat << "At (" << p.mX << ", " << p.mY << ", " << p.mZ << ") got " << distance << '\n';
		throw std::runtime_error("wrong signed distance");
	}
}

TriangleMesh ReverseAndMix(const TriangleMesh &mesh)
{
	auto faces = mesh.GetFaces();
	std::ranges::reverse(faces);
	for (std::size_t i = 0; i < faces.size(); i += 2)
		std::swap(faces[i][0], faces[i][1]);
	return TriangleMesh(mesh.GetVertices(), std::move(faces));
}

void CheckPlate(const TriangleMesh &mesh)
{
	const ClosedMeshSdf solid(mesh);
	ExpectLocation(solid, {0, 0, 0}, Exterior);
	ExpectLocation(solid, {0.005, 0, 0}, Boundary);
	ExpectLocation(solid, {0.007, 0, 0}, Interior);
	Check(std::abs(solid.SignedDistance({0, 0, 0}) - 0.005) <= 1e-15, "hole centre 5 mm");
	Check(std::abs(solid.SignedDistance({0.007, 0, 0}) + 0.002) <= 1e-15, "inside material -2 mm");
	ExpectLocation(solid, {0, 0, 0.005}, Exterior); // In the plane of the top annulus.
	ExpectLocation(solid, {0, 0.005, 0}, Boundary); // Coplanar ray along the inner wall.
	ExpectLocation(solid, {0.005, 0.005, 0.005}, Boundary);
	ExpectLocation(solid, {0.010, 0.010, 0.005}, Boundary);
	ExpectLocation(solid, {std::nextafter(0.005, 0.), 0, 0}, Exterior);
	ExpectLocation(solid, {std::nextafter(0.005, 1.), 0, 0}, Interior);
	Check(!GeometryLibrary().Contains(solid.GetMesh(), {0.007, 0, 0}), "triangle mesh containment must stay surface only");
}

void CheckGrazes(const TriangleMesh &mesh)
{
	const ClosedMeshSdf solid(mesh);
	ExpectLocation(solid, {}, Interior);			   // Each ray exits through a single vertex.
	ExpectLocation(solid, {-0.5, 1, 0}, Exterior);	   // Touches the top vertex without entering.
	ExpectLocation(solid, {-0.5, 0.5, 0.5}, Exterior); // Touches a silhouette edge.
	ExpectLocation(solid, {0, 0.25, 0}, Interior);	   // Crosses a shared edge.
	ExpectLocation(solid, {0, 0.5, 0.5}, Boundary);
	ExpectLocation(solid, {1, 0, 0}, Boundary);
	ExpectLocation(solid, {0.25, 0.25, 0.5}, Boundary);
	ExpectLocation(solid, {0.25, 0.25, std::nextafter(0.5, 0.)}, Interior);
	ExpectLocation(solid, {0.25, 0.25, std::nextafter(0.5, 1.)}, Exterior);
}

void CheckNearSurfaceMagnitude(const TriangleMesh &mesh)
{
	for (int exponent : {-700, 0, 700})
	{
		const double s = std::ldexp(1., exponent);
		auto		 vertices = mesh.GetVertices();
		for (auto &v : vertices)
			v = s * v;
		const ClosedMeshSdf solid(TriangleMesh(vertices, mesh.GetFaces()));
		auto				checkDistance = [&](Vec3 p, double expected)
		{
			const double computed = solid.SignedDistance(p);
			Check(std::abs(computed / expected - 1.) < 2e-15, "one-ULP distance magnitude");
		};
		for (double target : {0., s})
		{
			const double z = std::nextafter(0.5 * s, target);
			checkDistance({0.25 * s, 0.25 * s, z}, (z - 0.5 * s) / std::sqrt(3.));
		}
		const double y = std::nextafter(0.5 * s, s);
		checkDistance({0.5 * s, y, 0}, (y - 0.5 * s) / std::sqrt(2.));
		const double x = std::nextafter(s, 2 * s);
		checkDistance({x, 0, 0}, x - s);
	}
}

class CountBuilds final : public SpatialQueryProvider
{
  public:
	mutable int								 mBuilds = 0;
	std::unique_ptr<const SpatialQueryIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		++mBuilds;
		return MakeCpuSpatialQueryProvider()->Build(entries);
	}
};

void CheckPreparation(const TriangleMesh &mesh)
{
	CountBuilds					 provider;
	std::optional<ClosedMeshSdf> retained;
	{
		const ClosedMeshSdf solid(mesh, provider);
		retained.emplace(solid);
		Check(&retained->GetMesh() == &solid.GetMesh(), "copies share the prepared mesh");
	}
	Check(retained->SignedDistance({}) > 0 && provider.mBuilds == 1, "one prepared index survives the source handle");
	try
	{
		(void)ClosedMeshSdf(TriangleMesh(mesh.GetVertices(), {mesh.GetFaces().front()}));
		throw std::runtime_error("open mesh accepted");
	}
	catch (const std::invalid_argument &)
	{
	}
	try
	{
		(void)retained->SignedDistance({1, 0, std::numeric_limits<double>::quiet_NaN()});
		throw std::runtime_error("nonfinite point accepted");
	}
	catch (const std::invalid_argument &)
	{
	}
}
} // namespace

int main()
{
	try
	{
		const auto plate = Test::MakeThroughHolePlate();
		CheckPlate(plate);
		CheckPlate(ReverseAndMix(plate));
		const TriangleMesh octahedron({{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}},
									  {{0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4}, {0, 1, 5}, {1, 2, 5}, {2, 3, 5}, {3, 0, 5}});
		CheckGrazes(octahedron);
		CheckGrazes(ReverseAndMix(octahedron));
		CheckNearSurfaceMagnitude(octahedron);
		CheckNearSurfaceMagnitude(ReverseAndMix(octahedron));
		CheckPreparation(plate);
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
