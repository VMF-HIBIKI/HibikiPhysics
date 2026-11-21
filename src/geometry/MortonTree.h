#pragma once

#include "SpatialTree.h"

namespace HibikiPhysics::Detail
{
// Nodes own all bounds and IDs; the root of a nonempty tree is at index zero.
std::vector<BvhNode> BuildMortonTree(std::span<const SpatialEntry> entries);
} // namespace HibikiPhysics::Detail
