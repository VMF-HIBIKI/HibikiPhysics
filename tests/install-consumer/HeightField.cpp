#include <HibikiPhysics/Geometry.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::logic_error(message);
}

template <class Error, class Operation>
void ExpectError(Operation run)
{
	try
	{
		run();
	}
	catch (const Error &)
	{
		return;
	}
	throw std::logic_error("Expected height field operation to reject input");
}

void CheckValidation()
{
	const double			  inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
	const double			  max = std::numeric_limits<double>::max();
	const std::vector<double> samples(4);
	ExpectError<std::invalid_argument>([&]
									   { HeightField(1, 4, {1, 1}, 0, samples); });
	ExpectError<std::invalid_argument>([&]
									   { HeightField(2, 3, {1, 1}, 0, samples); });
	ExpectError<std::invalid_argument>([&]
									   { HeightField(std::numeric_limits<std::size_t>::max(), 2, {1, 1}, 0, samples); });
	for (double bad : {0.0, -1.0, inf, nan})
		ExpectError<std::invalid_argument>([&]
										   { HeightField(2, 2, {1, bad}, 0, samples); });
	ExpectError<std::overflow_error>([&]
									 { HeightField(2, 3, {max, 1}, 0, std::vector<double>(6)); });
	ExpectError<std::invalid_argument>([&]
									   { HeightField(2, 2, {1, 1}, nan, samples); });
	ExpectError<std::invalid_argument>([&]
									   { HeightField(2, 2, {1, 1}, 1, samples); });
	ExpectError<std::invalid_argument>([&]
									   { HeightField(2, 2, {1, 1}, 0, {0, 1, inf, 2}); });
	const HeightField flat(2, 2, {1, 1}, 0, samples);
	ExpectError<std::invalid_argument>([&]
									   { (void)flat.SurfaceHeight(nan, 0); });
	ExpectError<std::invalid_argument>([&]
									   { (void)GeometryLibrary{}.Contains(flat, {0, 0, inf}); });
	Check(GeometryLibrary{}.Contains(flat, {1, 1, 0}), "Flat patch includes base and upper corner");
	const HeightField extreme(2, 2, {1, 1}, -max, {-max, max, -max, max});
	Check(extreme.SurfaceHeight(0.5, 0.5) == 0, "Full finite range diagonal interpolation");
}

void CheckTerrain()
{
	const GeometryLibrary g;
	const HeightField	  field(2, 3, {2, 4}, -2, {0, 8, 3, 4, 0, -1});
	Check(field.SurfaceHeight(1, 2) == 0, "Fixed diagonal on a saddle");
	Check(field.SurfaceHeight(1.5, 1) == 4 && field.SurfaceHeight(0.5, 3) == 2, "Both triangles are piecewise planar");
	Check(field.SurfaceHeight(2, 2) == 4 && field.SurfaceHeight(4, 4) == -1, "Shared edge and last sample");
	for (auto p : {std::array{-0.1, 0.0}, std::array{4.1, 0.0}, std::array{2.0, 4.1}, std::array{2.0, -0.1}})
		Check(!field.SurfaceHeight(p[0], p[1]) && !g.Contains(field, {p[0], p[1], 0}), "Outside footprint");
	Check(g.Contains(field, {1.5, 1, 4}) && g.Contains(field, {4, 4, -2}), "Closed top and bottom");
	Check(!g.Contains(field, {1.5, 1, 4.1}) && !g.Contains(field, {0, 0, -2.1}), "Above surface and below base");
	const auto bounds = g.LocalBounds(field);
	Check(bounds.GetMin() == std::array{0., 0., -2.} && bounds.GetMax() == std::array{4., 4., 8.}, "Local extent");
	const Transform pose({3, 5, -2}, Quat::FromAxisAngle({1, 2, 3}, std::numbers::pi / 3));
	Check(g.Contains(field, pose, pose.TransformPoint({1.5, 1, 3})) && !g.Contains(field, pose, pose.TransformPoint({1.5, 1, 5})), "World containment");
	std::array<double, 3> low, high;
	low.fill(std::numeric_limits<double>::infinity());
	high.fill(-std::numeric_limits<double>::infinity());
	for (std::size_t row = 0; row < 2; ++row)
		for (std::size_t col = 0; col < 3; ++col)
			for (double z : {-2.0, field.GetSamples()[row * 3 + col]})
			{
				const auto [x, y, h] = pose.TransformPoint({2.0 * static_cast<double>(col), 4.0 * static_cast<double>(row), z});
				const std::array vertex{x, y, h};
				for (std::size_t i = 0; i < 3; ++i)
				{
					low[i] = std::min(low[i], vertex[i]);
					high[i] = std::max(high[i], vertex[i]);
				}
			}
	const auto world = g.WorldBounds(field, pose);
	for (std::size_t i = 0; i < 3; ++i)
		Check(std::abs(world.GetMin()[i] - low[i]) < 1e-12 && std::abs(world.GetMax()[i] - high[i]) < 1e-12, "Tight transformed terrain bounds");
}
} // namespace

int main()
{
	CheckValidation();
	CheckTerrain();
}
