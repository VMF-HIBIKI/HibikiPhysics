#pragma once

#include <HibikiPhysics/Math.h>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace HibikiPhysics
{
class Triangle
{
  public:
	Triangle(Vec3 inA, Vec3 inB, Vec3 inC);
	[[nodiscard]] const std::array<Vec3, 3> &GetVertices() const noexcept { return mVertices; }

  private:
	friend class GeometryLibrary;
	bool				ContainsPoint(Vec3 inPoint) const;
	std::array<Vec3, 3> mVertices;
};

class ConvexHull
{
  public:
	explicit ConvexHull(std::vector<Vec3> inPoints);
	ConvexHull(const ConvexHull &) noexcept = default;
	ConvexHull							  &operator=(const ConvexHull &) noexcept = default;
	[[nodiscard]] const std::vector<Vec3> &GetVertices() const noexcept { return mData->mVertices; }

  private:
	friend class GeometryLibrary;
	bool ContainsPoint(Vec3 inPoint) const;
	struct Face
	{
		std::array<std::size_t, 3> mIndices;
		Vec3					   mNormal;
	};
	struct Data
	{
		explicit Data(std::vector<Vec3> inPoints);
		std::vector<Vec3> mVertices;
		std::vector<Face> mFaces;
		double			  mScale;
	};
	std::shared_ptr<const Data> mData;
};

class TriangleMesh
{
  public:
	using Face = std::array<std::size_t, 3>;
	TriangleMesh(std::vector<Vec3> inVertices, std::vector<Face> inFaces);
	TriangleMesh(const TriangleMesh &) noexcept = default;
	TriangleMesh						  &operator=(const TriangleMesh &) noexcept = default;
	[[nodiscard]] const std::vector<Vec3> &GetVertices() const noexcept { return mData->mVertices; }
	[[nodiscard]] const std::vector<Face> &GetFaces() const noexcept { return mData->mFaces; }

  private:
	friend class GeometryLibrary;
	bool ContainsPoint(Vec3 inPoint) const;
	struct Data
	{
		Data(std::vector<Vec3> inVertices, std::vector<Face> inFaces);
		std::vector<Vec3> mVertices;
		std::vector<Face> mFaces;
	};
	std::shared_ptr<const Data> mData;
};
} // namespace HibikiPhysics
