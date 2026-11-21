#include <HibikiPhysics/Geometry.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace HibikiPhysics
{
namespace
{
constexpr double Tolerance = 64 * std::numeric_limits<double>::epsilon();

double MaxComponent(Vec3 v)
{
	return std::max({std::abs(v.mX), std::abs(v.mY), std::abs(v.mZ)});
}

void RequireFinite(Vec3 v)
{
	if (!std::isfinite(v.mX) || !std::isfinite(v.mY) || !std::isfinite(v.mZ))
		throw std::invalid_argument("Expected finite geometry coordinates");
}

Vec3 Divide(Vec3 v, double scale)
{
	return {v.mX / scale, v.mY / scale, v.mZ / scale};
}

double ShapeScale(const auto &vertices)
{
	double scale = 0;
	for (Vec3 v : vertices)
	{
		RequireFinite(v);
		scale = std::max(scale, MaxComponent(v - vertices.front()));
	}
	if (!std::isfinite(scale))
		throw std::overflow_error("Shape extent exceeds finite range");
	if (scale == 0)
		throw std::invalid_argument("Expected distinct vertices");
	return scale;
}

Vec3 UnitNormal(Vec3 ab, Vec3 ac)
{
	Vec3		 n = ab.Cross(ac);
	const double length = n.Length();
	if (length <= Tolerance)
		throw std::invalid_argument("Expected a numerically nondegenerate face");
	return Divide(n, length);
}

bool NearShape(Vec3 delta, double scale)
{
	return MaxComponent(Divide(delta, scale)) <= 1 + 4 * Tolerance;
}

bool OnTriangle(const std::array<Vec3, 3> &v, Vec3 point)
{
	const double scale = ShapeScale(v);
	if (!NearShape(point - v[0], scale))
		return false;
	const Vec3 b = Divide(v[1] - v[0], scale), c = Divide(v[2] - v[0], scale);
	const Vec3 p = Divide(point - v[0], scale), n = UnitNormal(b, c);
	if (std::abs(p.Dot(n)) > Tolerance)
		return false;
	return b.Cross(p).Dot(n) >= -Tolerance * b.Length() &&
		   (c - b).Cross(p - b).Dot(n) >= -Tolerance * (c - b).Length() &&
		   (-c).Cross(p - c).Dot(n) >= -Tolerance * c.Length();
}

void ValidateTriangle(const std::array<Vec3, 3> &v)
{
	const double scale = ShapeScale(v);
	UnitNormal(Divide(v[1] - v[0], scale), Divide(v[2] - v[0], scale));
}

} // namespace

Triangle::Triangle(Vec3 inA, Vec3 inB, Vec3 inC) : mVertices{inA, inB, inC}
{
	ValidateTriangle(mVertices);
}

TriangleMesh::TriangleMesh(std::vector<Vec3> inVertices, std::vector<Face> inFaces)
	: mData(std::make_shared<const Data>(std::move(inVertices), std::move(inFaces)))
{
}

TriangleMesh::Data::Data(std::vector<Vec3> inVertices, std::vector<Face> inFaces)
	: mVertices(std::move(inVertices)), mFaces(std::move(inFaces))
{
	if (mFaces.empty())
		throw std::invalid_argument("Expected at least one indexed triangle");
	for (Vec3 v : mVertices)
		RequireFinite(v);
	for (const auto &[a, b, c] : mFaces)
	{
		if (std::max({a, b, c}) >= mVertices.size())
			throw std::invalid_argument("Triangle index out of range");
		ValidateTriangle({mVertices[a], mVertices[b], mVertices[c]});
	}
}

ConvexHull::ConvexHull(std::vector<Vec3> inPoints) : mData(std::make_shared<const Data>(std::move(inPoints)))
{
}

ConvexHull::Data::Data(std::vector<Vec3> inPoints) : mVertices(std::move(inPoints))
{
	if (mVertices.size() < 4)
		throw std::invalid_argument("Expected four affinely independent hull points");
	mScale = ShapeScale(mVertices);
	std::vector<Vec3> p;
	p.reserve(mVertices.size());
	for (Vec3 v : mVertices)
		p.push_back(Divide(v - mVertices.front(), mScale));

	// Seed a tetrahedron with the most distant point, line distance and plane distance.
	const auto farthest = [&](auto distance) -> std::size_t
	{
		auto best = std::max_element(p.begin(), p.end(), [&](Vec3 lhs, Vec3 rhs)
									 { return distance(lhs) < distance(rhs); });
		return static_cast<std::size_t>(best - p.begin());
	};
	const auto b = farthest([](Vec3 v)
							{ return v.Length(); });
	const auto c = farthest([&](Vec3 v)
							{ return p[b].Cross(v).Length(); });
	const Vec3 base = UnitNormal(p[b], p[c]);
	const auto d = farthest([&](Vec3 v)
							{ return std::abs(base.Dot(v)); });
	if (std::abs(base.Dot(p[d])) <= Tolerance)
		throw std::invalid_argument("Expected a full-dimensional convex hull");
	const Vec3 interior = (p[b] + p[c] + p[d]) * 0.25;
	const auto addFace = [&](std::size_t i, std::size_t j, std::size_t k)
	{
		Vec3 n = UnitNormal(p[j] - p[i], p[k] - p[i]);
		if (n.Dot(interior - p[i]) > 0)
		{
			std::swap(j, k);
			n = -n;
		}
		mFaces.push_back({{i, j, k}, n});
	};
	addFace(0, b, c);
	addFace(0, b, d);
	addFace(0, c, d);
	addFace(b, c, d);

	for (std::size_t i = 0; i < p.size(); ++i)
	{
		// Edges shared by two visible faces cancel. The remaining horizon
		// bounds the hole to close with triangles incident to the new point.
		std::set<std::pair<std::size_t, std::size_t>> horizon;
		const auto									  visible = [&](const Face &face)
		{
			const auto &indices = face.mIndices;
			if (face.mNormal.Dot(p[i] - p[indices[0]]) <= Tolerance)
				return false;
			for (std::size_t e = 0; e < 3; ++e)
			{
				const auto from = indices[e], to = indices[(e + 1) % 3];
				if (!horizon.erase({to, from}))
					horizon.emplace(from, to);
			}
			return true;
		};
		std::erase_if(mFaces, visible);
		for (const auto &[from, to] : horizon)
			addFace(from, to, i);
	}
}

bool Triangle::ContainsPoint(Vec3 inPoint) const
{
	return OnTriangle(GetVertices(), inPoint);
}

bool TriangleMesh::ContainsPoint(Vec3 inPoint) const
{

	const auto &v = GetVertices();
	for (const auto &[a, b, c] : GetFaces())
		if (OnTriangle({v[a], v[b], v[c]}, inPoint))
			return true;
	return false;
}

bool ConvexHull::ContainsPoint(Vec3 inPoint) const
{
	const Vec3 p = Divide(inPoint - mData->mVertices.front(), mData->mScale);
	if (MaxComponent(p) > 1 + 4 * Tolerance)
		return false;
	for (const auto &face : mData->mFaces)
	{
		const Vec3 delta = p - Divide(mData->mVertices[face.mIndices[0]] - mData->mVertices.front(), mData->mScale);
		if (face.mNormal.Dot(delta) > Tolerance)
			return false;
	}
	return true;
}
} // namespace HibikiPhysics
