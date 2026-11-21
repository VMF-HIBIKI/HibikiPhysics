#include <HibikiPhysics/SolidMeshValidation.h>
#include <HibikiPhysics/DiscreteShapes.h>
#include <HibikiPhysics/SpatialIndex.h>
#include "Orientation.h"
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace HibikiPhysics
{
namespace
{
using Face = TriangleMesh::Face;
using Detail::Orientation;
using Detail::Point2;
using Detail::Point3;
using Positions = std::array<Point3, 3>;

struct EdgeUse
{
	std::size_t mFace, mCorner;
	bool		mPaired = false;
};

struct VertexStar
{
	std::size_t mFace = 0, mCount = 0;
};

Point3 Coordinates(Vec3 v)
{
	return {v.mX, v.mY, v.mZ};
}

std::string SourceEdge(const TriangleMesh &mesh, std::size_t face, std::size_t edge)
{
	const auto &indices = mesh.GetFaces()[face];
	return "source face " + std::to_string(face) + " edge (" + std::to_string(indices[edge]) + ", " + std::to_string(indices[(edge + 1) % 3]) + ")";
}

// Return the face adjacency after verifying edge incidence and the link of each vertex.
std::vector<Face> CheckManifold(const TriangleMesh &mesh)
{
	const auto											  &source = mesh.GetFaces();
	std::vector<Face>									   welded(source.size()), neighbors(source.size());
	std::vector<VertexStar>								   stars(mesh.GetVertices().size());
	std::map<Point3, std::size_t>						   representatives;
	std::map<Face, std::size_t>							   seenFaces;
	std::map<std::pair<std::size_t, std::size_t>, EdgeUse> edges;
	for (std::size_t i = 0; i < source.size(); ++i)
	{
		auto &face = welded[i];
		for (std::size_t corner = 0; corner < 3; ++corner)
		{
			const auto vertex = source[i][corner];
			face[corner] = representatives.try_emplace(Coordinates(mesh.GetVertices()[vertex]), vertex).first->second;
			auto &star = stars[face[corner]];
			star.mFace = i;
			++star.mCount;
		}
		auto key = face;
		std::sort(key.begin(), key.end());
		auto [duplicate, unique] = seenFaces.try_emplace(key, i);
		if (!unique)
			throw std::invalid_argument("Duplicate source faces " + std::to_string(duplicate->second) + " and " + std::to_string(i));
		for (std::size_t e = 0; e < 3; ++e)
		{
			auto [it, first] = edges.try_emplace(std::minmax(face[e], face[(e + 1) % 3]), EdgeUse{i, e});
			if (first)
				continue;
			auto &use = it->second;
			if (use.mPaired)
				throw std::invalid_argument("Nonmanifold " + SourceEdge(mesh, i, e) + " also incident to source faces " + std::to_string(use.mFace) + " and " + std::to_string(neighbors[use.mFace][use.mCorner]));
			neighbors[i][e] = use.mFace;
			neighbors[use.mFace][use.mCorner] = i;
			use.mPaired = true;
		}
	}
	for (const auto &entry : edges)
		if (!entry.second.mPaired)
			throw std::invalid_argument("Open boundary at " + SourceEdge(mesh, entry.second.mFace, entry.second.mCorner));

	// Each incident face has two neighbors about the vertex. Following that cycle must visit
	// all incident faces, including when separate input indices describe the same corner.
	for (std::size_t vertex = 0; vertex < stars.size(); ++vertex)
	{
		const auto &star = stars[vertex];
		if (star.mCount == 0)
			continue;
		std::size_t current = star.mFace, previous = source.size(), count = 0;
		do
		{
			const auto &f = welded[current];
			const auto	corner = static_cast<std::size_t>(std::find(f.begin(), f.end(), vertex) - f.begin());
			const auto &n = neighbors[current];
			const auto	next = n[corner] == previous ? n[(corner + 2) % 3] : n[corner];
			previous = current;
			current = next;
			++count;
		} while (current != star.mFace);
		if (count != star.mCount)
			throw std::invalid_argument("Nonmanifold source vertex " + std::to_string(vertex) + " at source face " + std::to_string(star.mFace) + " has disconnected incident fans");
	}
	return neighbors;
}

void CheckConnected(const std::vector<Face> &neighbors)
{
	std::vector<bool>		 visited(neighbors.size());
	std::vector<std::size_t> stack{0};
	visited[0] = true;
	while (!stack.empty())
	{
		const auto face = stack.back();
		stack.pop_back();
		for (auto adjacent : neighbors[face])
			if (!visited[adjacent])
			{
				visited[adjacent] = true;
				stack.push_back(adjacent);
			}
	}
	const auto other = std::find(visited.begin(), visited.end(), false);
	if (other != visited.end())
		throw std::invalid_argument("Disconnected solid shells at source faces 0 and " + std::to_string(other - visited.begin()));
}

Positions FacePositions(const TriangleMesh &mesh, std::size_t face)
{
	const auto &indices = mesh.GetFaces()[face];
	const auto &vertices = mesh.GetVertices();
	return {Coordinates(vertices[indices[0]]), Coordinates(vertices[indices[1]]), Coordinates(vertices[indices[2]])};
}

bool SameSideOrBoundary(int a, int b, int c)
{
	return std::min({a, b, c}) >= 0 || std::max({a, b, c}) <= 0;
}

Point2 Project(Point3 p, std::size_t axis)
{
	return {p[(axis + 1) % 3], p[(axis + 2) % 3]};
}

bool Inside(const std::array<Point2, 3> &t, Point2 p)
{
	return SameSideOrBoundary(Orientation(t[0], t[1], p), Orientation(t[1], t[2], p), Orientation(t[2], t[0], p));
}

bool InSegmentInterior(Point2 a, Point2 b, Point2 p)
{
	return p != a && p != b && p[0] >= std::min(a[0], b[0]) && p[0] <= std::max(a[0], b[0]) && p[1] >= std::min(a[1], b[1]) && p[1] <= std::max(a[1], b[1]);
}

bool ImproperEdgeContact(Point3 p, Point3 q, const Positions &t)
{
	const bool sharedP = std::find(t.begin(), t.end(), p) != t.end();
	const bool sharedQ = std::find(t.begin(), t.end(), q) != t.end();
	if (sharedP && sharedQ)
		return false;
	const int pSide = sharedP ? 0 : Orientation(t[0], t[1], t[2], p);
	const int qSide = sharedQ ? 0 : Orientation(t[0], t[1], t[2], q);
	if (pSide * qSide > 0 || (sharedP && qSide != 0) || (sharedQ && pSide != 0))
		return false;
	if (pSide * qSide < 0)
		return SameSideOrBoundary(Orientation(p, q, t[0], t[1]), Orientation(p, q, t[1], t[2]), Orientation(p, q, t[2], t[0]));

	// Project onto a plane where the triangle has nonzero area. This preserves every
	// incidence on its plane, including collinear overlaps and contacts at a single point.
	std::size_t			  axis = 0;
	std::array<Point2, 3> projected{};
	do
	{
		projected = {Project(t[0], axis), Project(t[1], axis), Project(t[2], axis)};
		if (Orientation(projected[0], projected[1], projected[2]) != 0)
			break;
		++axis;
	} while (axis < 3);
	const auto a = Project(p, axis), b = Project(q, axis);
	if ((pSide == 0 && !sharedP && Inside(projected, a)) || (qSide == 0 && !sharedQ && Inside(projected, b)))
		return true;
	if (pSide != 0 || qSide != 0)
		return false;
	for (std::size_t i = 0; i < 3; ++i)
	{
		const auto c = projected[i], d = projected[(i + 1) % 3];
		const int  cSide = Orientation(a, b, c), dSide = Orientation(a, b, d);
		if (cSide == 0 && InSegmentInterior(a, b, c))
			return true;
		if (cSide * dSide < 0 && Orientation(c, d, a) * Orientation(c, d, b) < 0)
			return true;
	}
	return false;
}

bool ImproperContact(const Positions &a, const Positions &b)
{
	for (std::size_t i = 0; i < 3; ++i)
		if (ImproperEdgeContact(a[i], a[(i + 1) % 3], b) || ImproperEdgeContact(b[i], b[(i + 1) % 3], a))
			return true;
	return false;
}

} // namespace

void ValidateClosedSolid(const TriangleMesh &inMesh, const SpatialOverlapIndex &inFaceIndex)
{
	const auto neighbors = CheckManifold(inMesh);
	for (const auto &[first, second] : inFaceIndex.OverlapPairs())
	{
		if (first >= neighbors.size() || second >= neighbors.size() || first == second)
			throw std::invalid_argument("Overlap index contains an invalid source face pair (" + std::to_string(first) + ", " + std::to_string(second) + ")");
		if (ImproperContact(FacePositions(inMesh, first), FacePositions(inMesh, second)))
			throw std::invalid_argument("Intersecting source faces " + std::to_string(first) + " and " + std::to_string(second));
	}
	CheckConnected(neighbors);
}
} // namespace HibikiPhysics
