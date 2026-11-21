#include <HibikiPhysics/ClosedMeshSdf.h>
#include <HibikiPhysics/SolidMeshValidation.h>
#include "Orientation.h"
#include <stdexcept>
#include <utility>

namespace HibikiPhysics
{
namespace
{
using Detail::Orientation;
using Detail::Point2;
using Detail::Point3;
using Face = std::array<Point3, 3>;

Point3 Coordinates(Vec3 p)
{
	return {p.mX, p.mY, p.mZ};
}

Face FacePositions(const TriangleMesh &mesh, std::size_t id)
{
	const auto &[a, b, c] = mesh.GetFaces().at(id);
	const auto &v = mesh.GetVertices();
	return {Coordinates(v[a]), Coordinates(v[b]), Coordinates(v[c])};
}

Point2 Project(Point3 p, std::size_t axis)
{
	return {p[(axis + 1) % 3], p[(axis + 2) % 3]};
}

// Closed tests include the entire surface. The half-open test assigns an edge to the
// CCW triangle for which it points up (or left when horizontal) in this projection.
enum class Coverage
{
	Outside,
	UnownedEdge,
	Owned
};

Coverage ProjectedCoverage(const Face &face, Point3 p, std::size_t axis, int winding)
{
	const Point2 q = Project(p, axis);
	auto		 coverage = Coverage::Owned;
	for (std::size_t i = 0; i < 3; ++i)
	{
		auto a = Project(face[i], axis), b = Project(face[(i + 1) % 3], axis);
		if (winding < 0)
			std::swap(a, b);
		const int side = Orientation(a, b, q);
		if (side < 0)
			return Coverage::Outside;
		if (side == 0 && !(b[1] > a[1] || (b[1] == a[1] && b[0] < a[0])))
			coverage = Coverage::UnownedEdge;
	}
	return coverage;
}

enum class RayContact
{
	Miss,
	Crossing,
	Boundary
};

RayContact ClassifyFace(const Face &face, const std::array<int, 3> &windings, Point3 p, std::size_t axis)
{
	const int  winding = windings[axis];
	const bool ahead = p[axis] < std::min({face[0][axis], face[1][axis], face[2][axis]});
	if (winding == 0)
	{
		// A parallel face contributes no crossing, but it may contain the origin.
		if (ahead || Orientation(face[0], face[1], face[2], p) != 0)
			return RayContact::Miss;
		const std::size_t other = windings[(axis + 1) % 3] != 0 ? (axis + 1) % 3 : (axis + 2) % 3;
		return ProjectedCoverage(face, p, other, windings[other]) == Coverage::Outside ? RayContact::Miss : RayContact::Boundary;
	}
	const auto coverage = ProjectedCoverage(face, p, axis, winding);
	if (coverage == Coverage::Outside)
		return RayContact::Miss;
	const int side = ahead ? winding : Orientation(face[0], face[1], face[2], p);
	if (side == 0)
		return RayContact::Boundary;
	return side == winding && coverage == Coverage::Owned ? RayContact::Crossing : RayContact::Miss;
}
} // namespace

struct ClosedMeshSdf::Data
{
	TriangleMeshQueries				queries;
	std::vector<std::array<int, 3>> windings;
	Point3							low, high;
	Data(TriangleMesh inMesh, const SpatialQueryProvider &inProvider) : queries(std::move(inMesh), inProvider)
	{
		const auto &mesh = queries.GetMesh();
		ValidateClosedSolid(mesh, queries.GetIndex());
		low = high = Coordinates(mesh.GetVertices()[mesh.GetFaces().front()[0]]);
		windings.reserve(mesh.GetFaces().size());
		for (std::size_t i = 0; i < mesh.GetFaces().size(); ++i)
		{
			const auto		   face = FacePositions(mesh, i);
			std::array<int, 3> signs{};
			for (std::size_t axis = 0; axis < 3; ++axis)
			{
				signs[axis] = Orientation(Project(face[0], axis), Project(face[1], axis), Project(face[2], axis));
				low[axis] = std::min({low[axis], face[0][axis], face[1][axis], face[2][axis]});
				high[axis] = std::max({high[axis], face[0][axis], face[1][axis], face[2][axis]});
			}
			windings.push_back(signs);
		}
	}
};

ClosedMeshSdf::ClosedMeshSdf(TriangleMesh inMesh) : ClosedMeshSdf(std::move(inMesh), *MakeCpuSpatialQueryProvider())
{
}

ClosedMeshSdf::ClosedMeshSdf(TriangleMesh inMesh, const SpatialQueryProvider &inProvider) : mData(std::make_shared<const Data>(std::move(inMesh), inProvider))
{
}

const TriangleMesh &ClosedMeshSdf::GetMesh() const noexcept
{
	return mData->queries.GetMesh();
}

SolidPointLocation ClosedMeshSdf::Classify(Vec3 inPoint, RayAxis inAxis) const
{
	const auto p = Coordinates(inPoint);
	const auto axis = static_cast<std::size_t>(inAxis);
	if (axis >= 3 || !std::ranges::all_of(p, [](double v)
										  { return std::isfinite(v); }))
		throw std::invalid_argument("Expected a finite query point and a valid ray axis");
	for (std::size_t i = 0; i < 3; ++i)
		if (p[i] < mData->low[i] || p[i] > mData->high[i])
			return SolidPointLocation::Exterior;
	Point3 direction{};
	direction[axis] = 1;
	bool odd = false;
	for (const auto id : mData->queries.GetIndex().RayCandidates(p, direction))
	{
		const auto contact = ClassifyFace(FacePositions(GetMesh(), id), mData->windings.at(id), p, axis);
		if (contact == RayContact::Boundary)
			return SolidPointLocation::Boundary;
		if (contact == RayContact::Crossing)
			odd = !odd;
	}
	return odd ? SolidPointLocation::Interior : SolidPointLocation::Exterior;
}

double ClosedMeshSdf::SignedDistance(Vec3 inPoint) const
{
	const auto location = Classify(inPoint);
	if (location == SolidPointLocation::Boundary)
		return 0;
	const double distance = mData->queries.Closest(inPoint).mDistance;
	return location == SolidPointLocation::Interior ? -distance : distance;
}
} // namespace HibikiPhysics
