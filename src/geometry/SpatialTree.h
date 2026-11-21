#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics::Detail
{
using Point = std::array<double, 3>;
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

// Preorder layout: the left child immediately follows its parent.
struct BvhNode
{
	AABB		mBounds;
	std::size_t mRight = 0;
	std::size_t mId = 0;
	bool		IsLeaf() const { return mRight == 0; }
};

void					  RequireFinite(const Point &point);
void					  RequireFinite(const AABB &bounds);
bool					  Intersects(const AABB &a, const AABB &b);
bool					  RayIntersects(const Point &origin, const Point &direction, const AABB &bounds);
double					  DistanceLowerBound(const Point &point, const AABB &bounds);
std::vector<SpatialEntry> CopySpatialEntries(std::span<const SpatialEntry> input);

class SpatialTreeIndex final : public SpatialQueryIndex
{
  public:
	explicit SpatialTreeIndex(std::vector<BvhNode> nodes) : mNodes(std::move(nodes)) {}
	std::vector<std::size_t>	  Overlaps(const AABB &bounds) const override;
	Pairs						  OverlapPairs() const override;
	std::vector<std::size_t>	  RayCandidates(Point origin, Point direction) const override;
	std::optional<SpatialNearest> Nearest(Point point, const std::function<double(std::size_t)> &exactDistance) const override;

  private:
	template <class Predicate>
	std::vector<std::size_t> Candidates(Predicate intersects) const;
	void					 CrossPairs(std::size_t a, std::size_t b, Pairs &out) const;
	void					 VisitNearest(std::size_t index, double lower, const Point &point, const std::function<double(std::size_t)> &exactDistance, std::optional<SpatialNearest> &best) const;
	std::vector<BvhNode>	 mNodes;
};
} // namespace HibikiPhysics::Detail
