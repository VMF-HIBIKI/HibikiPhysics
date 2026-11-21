#include <HibikiPhysics/TriangleMeshQueries.h>
#include <HibikiPhysics/MortonSpatialIndex.h>
#include "../fixtures/ThroughHolePlate.h"
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace HibikiPhysics;
namespace
{
void Check(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}
void Near(Vec3 a, Vec3 b)
{
	Check((a - b).Length() < 1e-12, "closest point");
}

void CheckSharedFeature(const TriangleMesh &mesh, Vec3 point, Vec3 witness, std::size_t firstFace)
{
	const auto distance = (point - witness).Length();
	const auto median = TriangleMeshQueries(mesh, *MakeMedianSpatialQueryProvider()).Closest(point);
	const auto morton = TriangleMeshQueries(mesh, *MakeMortonSpatialQueryProvider()).Closest(point);
	for (const auto &hit : {median, morton})
		Check(hit.mFace == firstFace && hit.mDistance == distance && (hit.mPoint - witness).Length() == 0, "shared feature distance and lowest face ID");
}
} // namespace
int main()
{
	// The adjacent faces independently scale their vertex/edge calculations by
	// 0.040 and 0.035. Both trees must preserve the original small X gap.
	const Vec3 probe{-0.025 + 91 * 0.0005, 0, 0}, witness{0.020, 0, 0};
	CheckSharedFeature(TriangleMesh({{-.020, 0, 0}, {.020, 0, 0}, {.020, .020, 0}, {-.015, 0, 0}, {.020, -.020, 0}}, {{0, 1, 2}, {3, 1, 4}}), probe, witness, 0);
	CheckSharedFeature(TriangleMesh({{-.020, .020, 0}, {.020, -.010, 0}, {.020, .010, 0}, {-.015, -.020, 0}}, {{0, 1, 2}, {3, 1, 2}}), probe, witness, 0);
	// One node from the half-cell-shifted 0.5 mm through-hole plate bake.
	CheckSharedFeature(Test::MakeThroughHolePlate(), {0.020749999999999998, -0.020250000000000001, 0.0054999999999999997}, {.020, -.020, .005}, 2);
	const TriangleMesh		  mesh({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {500, 500, 500}}, {{0, 1, 2}, {1, 3, 2}});
	const TriangleMeshQueries q(mesh);
	const auto				  interior = q.Closest({0.25, 0.5, 2});
	Near(interior.mPoint, {0.25, 0.5, 0});
	Check(interior.mDistance == 2 && interior.mFace == 0, "face interior");
	Near(q.Closest({0.75, -1, 0.5}).mPoint, {0.75, 0, 0});
	Near(q.Closest({-1, -2, 1}).mPoint, {0, 0, 0});
	Check(q.Closest({1, 1, 1}).mFace == 0, "shared edge tie");
	Check(q.GetIndex().Overlaps(AABB({499, 499, 499}, {501, 501, 501})).empty(), "unused vertex excluded");
	const auto direct = q.RayHits({0.25, 0.5, 2}, {0, 0, -4});
	Check(direct.size() == 1 && direct[0].mFeature == TriangleRayFeature::Face && direct[0].mParameter == 0.5, "face ray");
	Check(q.RayHits({4, 4, 1}, {0, 0, -1}).empty(), "miss");
	auto edge = q.RayHits({1, 1, 3}, {0, 0, -1});
	Check(edge.size() == 2 && edge[0].mFace == 0 && edge[1].mFace == 1 && edge[0].mFeature == TriangleRayFeature::Edge && edge[1].mFeature == TriangleRayFeature::Edge, "both shared-edge features");
	const auto vertex = q.RayHits({2, 0, 3}, {0, 0, -1});
	Check(vertex.size() == 2 && vertex[0].mFeature == TriangleRayFeature::Vertex && vertex[1].mFeature == TriangleRayFeature::Vertex, "vertex features");
	const auto along = q.RayHits({-1, 0.5, 0}, {1, 0, 0});
	Check(along.size() == 2 && along.front().mParameter == 1 && along[0].mFeature == TriangleRayFeature::Coplanar, "coplanar segment");
	Check(q.RayHits({1.5, 3, 0}, {1, -0.1, 0}).empty(), "coplanar miss");
	const TriangleMeshQueries a(TriangleMesh(mesh.GetVertices(), {{0, 1, 2}})), b(TriangleMesh(mesh.GetVertices(), {{1, 3, 2}}));
	for (int i = -4; i < 12; ++i)
		for (int j = -3; j < 8; ++j)
		{
			const Vec3 p{0.31 * i, 0.27 * j, 0.19 * (i - j)};
			const auto best = q.Closest(p);
			Check(std::abs(best.mDistance - std::min(a.Closest(p).mDistance, b.Closest(p).mDistance)) < 1e-12, "accelerated vs exhaustive");
			Check(std::abs((best.mPoint - p).Length() - best.mDistance) < 1e-12, "witness distance");
		}
	try
	{
		(void)q.RayHits({}, {});
	}
	catch (const std::invalid_argument &)
	{
		return 0;
	}
	return 1;
}
