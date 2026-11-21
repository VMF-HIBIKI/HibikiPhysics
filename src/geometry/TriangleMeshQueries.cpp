#include <HibikiPhysics/TriangleMeshQueries.h>
#include "Orientation.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace HibikiPhysics
{
namespace
{
using Point = std::array<double, 3>;
constexpr double EdgeRoundoff = 32 * std::numeric_limits<double>::epsilon();

double Magnitude(Vec3 v)
{
	return std::max({std::abs(v.mX), std::abs(v.mY), std::abs(v.mZ)});
}
bool Finite(Vec3 v)
{
	return std::isfinite(v.mX) && std::isfinite(v.mY) && std::isfinite(v.mZ);
}
Vec3 Divide(Vec3 v, double s)
{
	return {v.mX / s, v.mY / s, v.mZ / s};
}
Point XYZ(Vec3 v)
{
	return {v.mX, v.mY, v.mZ};
}

double RayParameter(double time, double lengthScale, double directionScale)
{
	if (time == 0)
		return 0;
	int			 tExp = 0, lExp = 0, dExp = 0;
	const double t = std::frexp(time, &tExp), l = std::frexp(lengthScale, &lExp), d = std::frexp(directionScale, &dExp);
	const double parameter = std::scalbn(t * l / d, tExp + lExp - dExp);
	if (!std::isfinite(parameter))
		throw std::overflow_error("Ray parameter exceeds finite range");
	return parameter;
}

struct FaceFrame
{
	Vec3   a, ab, ac, normal;
	double scale, area;
	FaceFrame(const TriangleMesh &mesh, std::size_t id)
	{
		const auto &[i, j, k] = mesh.GetFaces().at(id);
		const auto &v = mesh.GetVertices();
		a = v[i];
		scale = std::max(Magnitude(v[j] - a), Magnitude(v[k] - a));
		ab = Divide(v[j] - a, scale);
		ac = Divide(v[k] - a, scale);
		const Vec3 cross = ab.Cross(ac);
		area = cross.Length();
		normal = Divide(cross, area);
	}
	Vec3 Relative(Vec3 p) const
	{
		const Vec3 q = Divide(p - a, scale);
		if (!Finite(q))
			throw std::overflow_error("Triangle query exceeds representable local range");
		return q;
	}
	// Two affine coordinates of the orthogonal projection in the face plane.
	std::array<double, 2> Coordinates(Vec3 q) const
	{
		return {q.Cross(ac).Dot(normal) / area, ab.Cross(q).Dot(normal) / area};
	}
};

Vec3 OnSegment(Vec3 p, Vec3 a, Vec3 edge)
{
	const double along = (p - a).Dot(edge) / edge.Dot(edge);
	return a + std::clamp(along, 0.0, 1.0) * edge;
}

// Scaled cofactors preserve small distances to slanted faces and edges when subtracting a
// vertex or projecting onto a unit normal would round away the displacement.
struct ScaledCross
{
	Vec3 components;
	int	 exponent;
};

ScaledCross AccurateCross(Point a, Point b, Point c)
{
	std::array<Detail::ScaledDeterminant, 3> cofactors{};
	int										 exponent = std::numeric_limits<int>::min();
	for (std::size_t i = 0; i < 3; ++i)
	{
		const auto j = (i + 1) % 3, k = (i + 2) % 3;
		if (Detail::ExactOrientation<2>({{{a[j], a[k]}, {b[j], b[k]}, {c[j], c[k]}}}, &cofactors[i]) != 0)
			exponent = std::max(exponent, cofactors[i].exponent);
	}
	if (exponent == std::numeric_limits<int>::min())
		return {{}, 0};
	Point values{};
	for (std::size_t i = 0; i < 3; ++i)
		values[i] = std::scalbn(cofactors[i].fraction, cofactors[i].exponent - exponent);
	return {{values[0], values[1], values[2]}, exponent};
}

MeshClosestPoint RefineNearSurface(const TriangleMesh &mesh, const FaceFrame &f, std::size_t id, Vec3 point, bool wellInside)
{
	const auto &[i, j, k] = mesh.GetFaces()[id];
	const auto		&vertices = mesh.GetVertices();
	const std::array v{vertices[i], vertices[j], vertices[k]};
	const std::array t{XYZ(v[0]), XYZ(v[1]), XYZ(v[2])};
	const Point		 p = XYZ(point);
	if (wellInside)
		for (std::size_t axis = 0; axis < 3; ++axis)
			if (t[0][axis] == t[1][axis] && t[0][axis] == t[2][axis])
			{
				Point witness = p;
				witness[axis] = t[0][axis];
				return {{witness[0], witness[1], witness[2]}, std::abs(p[axis] - witness[axis]), id};
			}
	const auto				   normal = AccurateCross(t[0], t[1], t[2]);
	std::array<ScaledCross, 3> edges{};
	bool					   inside = true;
	if (!wellInside)
		for (std::size_t e = 0; e < 3; ++e)
		{
			edges[e] = AccurateCross(t[e], t[(e + 1) % 3], p);
			if (edges[e].components.Dot(normal.components) < 0)
				inside = false;
		}
	if (inside)
	{
		Detail::ScaledDeterminant volume;
		Detail::ExactOrientation<3>({t[0], t[1], t[2], p}, &volume);
		const double area = normal.components.Length();
		const double height = std::scalbn(volume.fraction / area, volume.exponent - normal.exponent);
		return {point + height * Divide(normal.components, area), std::abs(height), id};
	}
	MeshClosestPoint best{{}, std::numeric_limits<double>::infinity(), id};
	int				 scaleExponent;
	const double	 scaleFraction = std::frexp(f.scale, &scaleExponent);
	const std::array local{Vec3{}, f.ab, f.ac};
	const auto		 localPoint = f.Relative(point);
	for (std::size_t e = 0; e < 3; ++e)
	{
		const auto next = (e + 1) % 3;
		const Vec3 edge = local[next] - local[e];
		// Subtract at the endpoint first to retain a one-ULP gap. When the world
		// subtraction overflows, the same difference fits in the scaled face frame.
		auto		 from = Divide(point - v[e], f.scale), to = Divide(v[next] - point, f.scale);
		const double along = (Finite(from) ? from : localPoint - local[e]).Dot(edge);
		const double remaining = (Finite(to) ? to : local[next] - localPoint).Dot(edge);
		Vec3		 witness = along <= 0 ? v[e] : v[next];
		double		 distance;
		if (along > 0 && remaining > 0)
		{
			const double u = along / (along + remaining);
			witness = {std::lerp(v[e].mX, v[next].mX, u), std::lerp(v[e].mY, v[next].mY, u), std::lerp(v[e].mZ, v[next].mZ, u)};
			distance = std::scalbn(edges[e].components.Length() / (scaleFraction * edge.Length()), edges[e].exponent - scaleExponent);
		}
		else
			distance = (point - witness).Length();
		if (distance < best.mDistance)
			best = {witness, distance, id};
	}
	return best;
}

MeshClosestPoint ClosestOnFace(const TriangleMesh &mesh, const FaceFrame &f, std::size_t id, Vec3 point)
{
	const Vec3 p = f.Relative(point);
	const auto [b, c] = f.Coordinates(p);
	if (b >= 0 && c >= 0 && b + c <= 1)
	{
		const double distance = std::abs(p.Dot(f.normal));
		const double roundoff = EdgeRoundoff / f.area;
		if (distance <= roundoff)
			return RefineNearSurface(mesh, f, id, point, std::min({b, c, 1 - b - c}) > roundoff);
		const Vec3 projected = b * f.ab + c * f.ac;
		return {f.a + f.scale * projected, distance * f.scale, id};
	}
	const std::array candidates{OnSegment(p, {}, f.ab), OnSegment(p, {}, f.ac), OnSegment(p, f.ab, f.ac - f.ab)};
	Vec3			 best = candidates[0];
	double			 distance = (p - best).Length();
	for (std::size_t i = 1; i < candidates.size(); ++i)
	{
		const double other = (p - candidates[i]).Length();
		if (other < distance)
		{
			best = candidates[i];
			distance = other;
		}
	}
	if (distance <= EdgeRoundoff / f.area)
		return RefineNearSurface(mesh, f, id, point, false);
	return {f.a + f.scale * best, distance * f.scale, id};
}

std::optional<std::pair<double, TriangleRayFeature>> IntersectFace(const FaceFrame &f, Vec3 origin, Vec3 direction)
{
	const Vec3	 p = f.Relative(origin);
	const double height = p.Dot(f.normal), slope = direction.Dot(f.normal);
	if (slope == 0)
	{
		if (std::abs(height) > EdgeRoundoff * std::max(1.0, Magnitude(p)))
			return std::nullopt;
		// Clip the coplanar forward ray against all three barycentric half-spaces.
		const auto [b, c] = f.Coordinates(p);
		const auto [db, dc] = f.Coordinates(direction);
		const Point weights{1 - b - c, b, c}, speeds{-db - dc, db, dc};
		double		lo = 0, hi = std::numeric_limits<double>::infinity();
		for (std::size_t i = 0; i < 3; ++i)
		{
			if (speeds[i] > 0)
				lo = std::max(lo, -weights[i] / speeds[i]);
			else if (speeds[i] < 0)
				hi = std::min(hi, -weights[i] / speeds[i]);
			else if (weights[i] < -EdgeRoundoff)
				return std::nullopt;
		}
		if (lo > hi)
			return std::nullopt;
		return std::pair{lo, TriangleRayFeature::Coplanar};
	}
	const double time = -height / slope;
	if (time < 0 || !std::isfinite(time))
		return std::nullopt;
	const auto [b, c] = f.Coordinates(p + time * direction);
	const Point weights{1 - b - c, b, c};
	if (*std::min_element(weights.begin(), weights.end()) < -EdgeRoundoff)
		return std::nullopt;
	const auto zeros = std::count_if(weights.begin(), weights.end(), [](double w)
									 { return w <= EdgeRoundoff; });
	const auto feature = zeros >= 2 ? TriangleRayFeature::Vertex : zeros == 1 ? TriangleRayFeature::Edge
																			  : TriangleRayFeature::Face;
	return std::pair{time, feature};
}
} // namespace

struct TriangleMeshQueries::Data
{
	TriangleMesh							 mesh;
	std::vector<FaceFrame>					 faces;
	std::unique_ptr<const SpatialQueryIndex> index;
	Data(TriangleMesh inMesh, const SpatialQueryProvider &inProvider);
};

TriangleMeshQueries::TriangleMeshQueries(TriangleMesh inMesh) : TriangleMeshQueries(std::move(inMesh), *MakeCpuSpatialQueryProvider())
{
}

TriangleMeshQueries::TriangleMeshQueries(TriangleMesh inMesh, const SpatialQueryProvider &inProvider) : mData(std::make_shared<const Data>(std::move(inMesh), inProvider))
{
}

const TriangleMesh &TriangleMeshQueries::GetMesh() const noexcept
{
	return mData->mesh;
}
const SpatialQueryIndex &TriangleMeshQueries::GetIndex() const noexcept
{
	return *mData->index;
}

TriangleMeshQueries::Data::Data(TriangleMesh inMesh, const SpatialQueryProvider &inProvider) : mesh(std::move(inMesh))
{
	const auto				 &vertices = mesh.GetVertices();
	std::vector<SpatialEntry> entries;
	entries.reserve(mesh.GetFaces().size());
	faces.reserve(mesh.GetFaces().size());
	for (const auto &[i, j, k] : mesh.GetFaces())
	{
		const Point a = XYZ(vertices[i]), b = XYZ(vertices[j]), c = XYZ(vertices[k]);
		Point		low{}, high{};
		for (std::size_t n = 0; n < 3; ++n)
		{
			low[n] = std::min({a[n], b[n], c[n]});
			high[n] = std::max({a[n], b[n], c[n]});
		}
		faces.emplace_back(mesh, entries.size());
		entries.push_back({entries.size(), AABB(low, high)});
	}
	index = inProvider.Build(entries);
	if (!index)
		throw std::invalid_argument("Spatial provider returned an empty index handle");
}

MeshClosestPoint TriangleMeshQueries::Closest(Vec3 inPoint) const
{
	if (!Finite(inPoint))
		throw std::invalid_argument("Expected a finite query point");
	const auto nearest = GetIndex().Nearest(XYZ(inPoint), [&](std::size_t face)
											{ return ClosestOnFace(GetMesh(), mData->faces.at(face), face, inPoint).mDistance; });
	if (!nearest)
		throw std::overflow_error("Mesh distance exceeds finite range or provider returned no leaf");
	return ClosestOnFace(GetMesh(), mData->faces.at(nearest->mId), nearest->mId, inPoint);
}

std::vector<MeshRayHit> TriangleMeshQueries::RayHits(Vec3 inOrigin, Vec3 inDirection) const
{
	if (!Finite(inOrigin) || !Finite(inDirection) || Magnitude(inDirection) == 0)
		throw std::invalid_argument("Expected a finite origin and a finite nonzero direction");
	const auto				magnitude = Magnitude(inDirection);
	const Vec3				unitScale = Divide(inDirection, magnitude);
	std::vector<MeshRayHit> hits;
	for (const auto id : GetIndex().RayCandidates(XYZ(inOrigin), XYZ(unitScale)))
	{
		const FaceFrame &face = mData->faces.at(id);
		const auto		 hit = IntersectFace(face, inOrigin, unitScale);
		if (!hit)
			continue;
		const double time = RayParameter(hit->first, face.scale, magnitude);
		hits.push_back({id, time, hit->second});
	}
	std::sort(hits.begin(), hits.end(), [](const MeshRayHit &a, const MeshRayHit &b)
			  { return a.mParameter == b.mParameter ? a.mFace < b.mFace : a.mParameter < b.mParameter; });
	return hits;
}
} // namespace HibikiPhysics
