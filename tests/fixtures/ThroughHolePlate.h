#pragma once

#include <HibikiPhysics/DiscreteShapes.h>
#include <utility>

namespace HibikiPhysics::Test
{
// Centred 40 x 40 x 10 mm plate with a centred 10 x 10 mm through-hole along Z.
inline TriangleMesh MakeThroughHolePlate()
{
	std::vector<Vec3>				vertices;
	std::vector<TriangleMesh::Face> faces;
	for (double z : {-0.005, 0.005})
		for (double halfWidth : {0.020, 0.005})
			for (auto [x, y] : {std::array{-1., -1.}, std::array{1., -1.}, std::array{1., 1.}, std::array{-1., 1.}})
				vertices.push_back({x * halfWidth, y * halfWidth, z});
	auto quad = [&](std::size_t a, std::size_t b, std::size_t c, std::size_t d)
	{
		faces.push_back({a, b, c});
		faces.push_back({a, c, d});
	};
	for (std::size_t i = 0; i < 4; ++i)
	{
		const auto j = (i + 1) % 4;
		quad(i, 4 + i, 4 + j, j);
		quad(8 + i, 8 + j, 12 + j, 12 + i);
		quad(i, j, 8 + j, 8 + i);
		quad(4 + i, 12 + i, 12 + j, 4 + j);
	}
	return TriangleMesh(std::move(vertices), std::move(faces));
}
} // namespace HibikiPhysics::Test
