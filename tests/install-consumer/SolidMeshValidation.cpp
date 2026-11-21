#include <HibikiPhysics/Geometry.h>
#include <HibikiPhysics/SolidMeshValidation.h>
#include <HibikiPhysics/SpatialIndex.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace HibikiPhysics;

namespace
{
struct Surface
{
	std::vector<Vec3>				mVertices;
	std::vector<TriangleMesh::Face> mFaces;
	void							Quad(std::size_t a, std::size_t b, std::size_t c, std::size_t d)
	{
		mFaces.push_back({a, b, c});
		mFaces.push_back({a, c, d});
	}
};

Surface Cube()
{
	Surface mesh{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, {}};
	mesh.Quad(0, 3, 2, 1);
	mesh.Quad(4, 5, 6, 7);
	mesh.Quad(0, 1, 5, 4);
	mesh.Quad(1, 2, 6, 5);
	mesh.Quad(2, 3, 7, 6);
	mesh.Quad(3, 0, 4, 7);
	return mesh;
}

Surface ThroughHole()
{
	Surface mesh;
	for (double z : {0., 1.})
		for (double radius : {2., 1.})
			for (auto [x, y] : {std::array{-1., -1.}, std::array{1., -1.}, std::array{1., 1.}, std::array{-1., 1.}})
				mesh.mVertices.push_back({radius * x, radius * y, z});
	for (std::size_t i = 0; i < 4; ++i)
	{
		const auto j = (i + 1) % 4;
		mesh.Quad(i, j, 4 + j, 4 + i);
		mesh.Quad(8 + i, 12 + i, 12 + j, 8 + j);
		mesh.Quad(i, 8 + i, 8 + j, j);
		mesh.Quad(4 + i, 4 + j, 12 + j, 12 + i);
	}
	return mesh;
}

void AppendCube(Surface &surface, Vec3 origin, double size)
{
	const auto offset = surface.mVertices.size();
	const auto cube = Cube();
	for (auto vertex : cube.mVertices)
		surface.mVertices.push_back(origin + vertex * size);
	for (auto [a, b, c] : cube.mFaces)
		surface.mFaces.push_back({offset + a, offset + b, offset + c});
}

void Validate(const Surface &surface)
{
	TriangleMesh			  mesh(surface.mVertices, surface.mFaces);
	GeometryLibrary			  geometry;
	std::vector<SpatialEntry> entries;
	for (const auto &[a, b, c] : mesh.GetFaces())
	{
		const auto &vertices = mesh.GetVertices();
		entries.push_back({entries.size(), geometry.LocalBounds(Triangle(vertices[a], vertices[b], vertices[c]))});
	}
	const auto index = MakeCpuSpatialQueryProvider()->Build(entries);
	ValidateClosedSolid(mesh, *index);
}

void Reject(const Surface &surface, const std::string &diagnostic)
{
	try
	{
		Validate(surface);
	}
	catch (const std::invalid_argument &error)
	{
		if (std::string(error.what()).find(diagnostic) != std::string::npos)
			return;
		throw std::logic_error("Expected diagnostic '" + diagnostic + "', got '" + error.what() + "'");
	}
	throw std::logic_error("Expected rejection: " + diagnostic);
}

void CheckSolids()
{
	auto cube = Cube();
	Validate(cube);
	Validate(ThroughHole());

	Surface unwelded;
	for (auto face : cube.mFaces)
	{
		const auto base = unwelded.mVertices.size();
		for (auto i : face)
			unwelded.mVertices.push_back(cube.mVertices[i]);
		unwelded.mFaces.push_back({base + 1, base, base + 2});
	}
	std::swap(unwelded.mFaces[0][0], unwelded.mFaces[0][1]);
	unwelded.mVertices[0].mX = -0.;
	unwelded.mVertices.push_back({9, 9, 9});
	Validate(unwelded);
	unwelded.mVertices[0].mX = std::nextafter(0., 1.);
	Reject(unwelded, "Open boundary at source face");
	cube.mFaces.pop_back();
	Reject(cube, "edge (");

	cube = Cube();
	cube.mFaces.push_back({2, 3, 0});
	Reject(cube, "Duplicate source faces 0 and 12");

	cube = Cube();
	AppendCube(cube, {1, 1, 0}, 1);
	Reject(cube, "Nonmanifold source face");
	cube = Cube();
	AppendCube(cube, {1, 1, 1}, 1);
	Reject(cube, "Nonmanifold source vertex");
	cube = Cube();
	AppendCube(cube, {2, 0, 0}, 1);
	Reject(cube, "Disconnected solid shells at source faces 0 and 12");
	cube = Cube();
	AppendCube(cube, {0.25, 0.25, 0.25}, 0.5);
	Reject(cube, "Disconnected solid shells");
	cube = Cube();
	AppendCube(cube, {0.3, 0.4, 0.5}, 1);
	Reject(cube, "Intersecting source faces");
	cube = Cube();
	AppendCube(cube, {0.3, 0.4, 1}, 1);
	Reject(cube, "Intersecting source faces");
	cube = Cube();
	cube.mVertices[6] = {0.7, 0.8, -0.25};
	Reject(cube, "Intersecting source faces");

	// A tetrahedron with its fourth vertex on the base overlaps even though every face
	// pair shares an edge. Moving that point one ULP off the plane makes a valid solid.
	for (int exponent : {-850, 0, 850})
	{
		const double scale = std::ldexp(1., exponent);
		Surface		 tetra{{{0, 0, 0}, {4, 0, 4}, {0, 4, 4}, {1, 1, 2}}, {{0, 1, 2}, {0, 1, 3}, {1, 2, 3}, {0, 2, 3}}};
		for (auto &p : tetra.mVertices)
			p = p * scale;
		Reject(tetra, "Intersecting source faces");
		tetra.mVertices[3].mZ = std::nextafter(2 * scale, 3 * scale);
		Validate(tetra);
	}
}
} // namespace

int main()
{
	try
	{
		CheckSolids();
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
