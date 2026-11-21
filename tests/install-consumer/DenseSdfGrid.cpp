#include <HibikiPhysics/DenseSdfGrid.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace
{
using namespace HibikiPhysics;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::logic_error(message);
}

template <class Error>
void ExpectError(std::string_view diagnostic, auto run)
{
	try
	{
		run();
	}
	catch (const Error &e)
	{
		Check(std::string_view(e.what()).find(diagnostic) != std::string_view::npos, "Unexpected dense SDF diagnostic");
		return;
	}
	throw std::logic_error("Expected dense SDF operation to reject input");
}

double PlaneDistance(Vec3 p)
{
	return (2 * p.mX + p.mY - 2 * p.mZ) / 3;
}

void CheckPlane()
{
	const Vec3						 origin{3, -6, 1.5};
	const std::array<std::size_t, 3> counts{3, 2, 4};
	const double					 spacing = 3;
	const std::vector<double>		 values{
		-1, 1, 3, 0, 2, 4,
		-3, -1, 1, -2, 0, 2,
		-5, -3, -1, -4, -2, 0,
		-7, -5, -3, -6, -4, -2};
	const DenseSdfGrid grid(origin, spacing, counts, values);
	Check(grid.GetOrigin().mX == 3 && grid.GetOrigin().mY == -6 && grid.GetOrigin().mZ == 1.5, "First mesh-local node");
	Check(grid.GetSpacing() == spacing && grid.GetNodeCounts() == counts && grid.GetValues() == values, "Grid layout and values");
	for (std::size_t z = 0; z < counts[2]; ++z)
		for (std::size_t y = 0; y < counts[1]; ++y)
			for (std::size_t x = 0; x < counts[0]; ++x)
			{
				const Vec3 p = origin + Vec3{static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)} * spacing;
				Check(grid.GetNode(x, y, z) == PlaneDistance(p) && grid.Sample(p) == PlaneDistance(p), "Affine field at each node");
			}
	for (Vec3 p : {Vec3{7.5, -5.25, 8.25}, Vec3{9, -3.75, 7.5}, Vec3{7.5, -3, 8.25}, Vec3{3.75, -5.25, 10.5}, Vec3{9, -3, 5.25}})
		Check(grid.Sample(p) == PlaneDistance(p), "Affine field in a cell, on each last plane and on a last edge");

	const double inf = std::numeric_limits<double>::infinity();
	for (Vec3 p : {Vec3{std::nextafter(3., -inf), -6, 1.5}, Vec3{std::nextafter(9., inf), -3, 10.5},
				   Vec3{3, std::nextafter(-6., -inf), 1.5}, Vec3{9, std::nextafter(-3., inf), 10.5},
				   Vec3{3, -6, std::nextafter(1.5, -inf)}, Vec3{9, -3, std::nextafter(10.5, inf)}})
		Check(!grid.Sample(p), "Next representable coordinate beyond each boundary is outside");
}

void CheckInterpolation()
{
	const DenseSdfGrid corner({}, 1, {2, 2, 2}, {0, 0, 0, 0, 0, 0, 0, 1});
	Check(corner.Sample({0.5, 0.25, 0.75}) == 0.5 * 0.25 * 0.75, "Each axis contributes its trilinear weight");
	const DenseSdfGrid decimal({0.3, -1.1, 1.2}, 0.1, {3, 2, 2}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
	Check(decimal.Sample({0.3 + 0.1, -1.1 + 0.1, 1.2 + 0.1}) == 10, "Exact node despite rounding in inverse coordinates");
	const double	   max = std::numeric_limits<double>::max();
	const DenseSdfGrid extreme({}, 1, {2, 2, 2}, {-max, max, -max, max, -max, max, -max, max});
	Check(extreme.Sample({0.5, 0.5, 0.5}) == 0, "Full finite range interpolation");
}

void CheckValidation()
{
	const double					 inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
	const std::size_t				 maxSize = std::numeric_limits<std::size_t>::max();
	const std::array<std::size_t, 3> counts{2, 2, 2};
	const std::vector<double>		 values(8);
	for (Vec3 point : {Vec3{nan, 0, 0}, Vec3{0, inf, 0}, Vec3{0, 0, -inf}})
		ExpectError<std::invalid_argument>("origin", [&]
										   { DenseSdfGrid(point, 1, counts, values); });
	for (double spacing : {0., -1., inf, nan})
		ExpectError<std::invalid_argument>("spacing", [&]
										   { DenseSdfGrid({}, spacing, counts, values); });
	for (auto bad : {std::array<std::size_t, 3>{0, 2, 2}, {2, 1, 2}, {2, 2, 1}})
		ExpectError<std::invalid_argument>("per axis", [&]
										   { DenseSdfGrid({}, 1, bad, values); });
	ExpectError<std::invalid_argument>("value count", [&]
									   { DenseSdfGrid({}, 1, {2, 3, 2}, values); });
	ExpectError<std::invalid_argument>("value count", [&]
									   { DenseSdfGrid({}, 1, counts, std::vector<double>(9)); });
	ExpectError<std::overflow_error>("size", [&]
									 { DenseSdfGrid({}, 1, {2, maxSize, 2}, {}); });
	ExpectError<std::overflow_error>("size", [&]
									 { DenseSdfGrid({}, 1, {2, 2, values.max_size() / 4 + 1}, {}); });
	ExpectError<std::overflow_error>("coordinate range", [&]
									 { DenseSdfGrid({std::numeric_limits<double>::max(), 0, 0}, std::numeric_limits<double>::max(), counts, values); });
	ExpectError<std::invalid_argument>("distinct", [&]
									   { DenseSdfGrid({1e20, 0, 0}, 1, counts, values); });
	for (double bad : {nan, inf, -inf})
	{
		auto invalid = values;
		invalid.back() = bad;
		ExpectError<std::invalid_argument>("values", [&]
										   { DenseSdfGrid({}, 1, counts, invalid); });
	}
	const DenseSdfGrid grid({}, 1, counts, values);
	for (auto index : {std::array<std::size_t, 3>{2, 0, 0}, {0, 2, 0}, {0, 0, maxSize}})
		ExpectError<std::out_of_range>("index", [&]
									   { (void)grid.GetNode(index[0], index[1], index[2]); });
	for (Vec3 point : {Vec3{nan, 0, 0}, Vec3{0, inf, 0}, Vec3{-1, 0, -inf}})
		ExpectError<std::invalid_argument>("query coordinates", [&]
										   { (void)grid.Sample(point); });
}
} // namespace

int main()
{
	CheckPlane();
	CheckInterpolation();
	CheckValidation();
}
