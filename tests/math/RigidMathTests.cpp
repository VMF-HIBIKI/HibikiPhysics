#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/Math.h>
#include <Eigen/Geometry>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;
constexpr double Tolerance = 1e-12;
constexpr double QuarterTurn = std::numbers::pi_v<double> / 2;

void ExpectVector(Vec3 actual, const Eigen::Vector3d &expected)
{
	EXPECT_NEAR(actual.mX, expected.x(), Tolerance);
	EXPECT_NEAR(actual.mY, expected.y(), Tolerance);
	EXPECT_NEAR(actual.mZ, expected.z(), Tolerance);
}

TEST(RigidMath, VectorAlgebraAndIdentity)
{
	const Vec3			  a{1, 2, -3}, b{-2, 4, 1};
	const Eigen::Vector3d ea(1, 2, -3), eb(-2, 4, 1);
	ExpectVector(a + b, ea + eb);
	ExpectVector(a - b, ea - eb);
	ExpectVector(-a, -ea);
	ExpectVector(2.5 * a, 2.5 * ea);
	ExpectVector(a * 2.5, ea * 2.5);
	ExpectVector(a.Cross(b), ea.cross(eb));
	EXPECT_DOUBLE_EQ(a.Dot(b), ea.dot(eb));
	EXPECT_NEAR(a.Length(), ea.norm(), Tolerance);
	EXPECT_EQ(Quat{}.ToXYZW(), (std::array{0., 0., 0., 1.}));
	ExpectVector(Transform{}.TransformPoint(a), ea);
	ExpectVector(Quat{}.Rotate(a), ea);
}

TEST(RigidMath, ComponentOrderAndScaledInputs)
{
	const Eigen::Quaterniond reference(Eigen::AngleAxisd(QuarterTurn, Eigen::Vector3d::UnitZ()));
	const Vec3				 v{2, -1, 3};
	const Eigen::Vector3d	 expected = reference * Eigen::Vector3d(2, -1, 3);
	for (double scale : {2., -2., std::numeric_limits<double>::max(), std::numeric_limits<double>::denorm_min()})
	{
		const auto q = Quat::FromXYZW({0, 0, scale, scale});
		ExpectVector(q.Rotate(v), expected);
		ExpectVector(Quat::FromWXYZ({scale, 0, 0, scale}).Rotate(v), expected);
		const auto xyzw = q.ToXYZW();
		EXPECT_NEAR(Eigen::Map<const Eigen::Vector4d>(xyzw.data()).norm(), 1., Tolerance);
	}
	const std::array		 xyzw{1., 2., 3., 4.};
	const auto				 q = Quat::FromXYZW(xyzw);
	const Eigen::Quaterniond eq = Eigen::Quaterniond(4, 1, 2, 3).normalized();
	const auto				 outgoing = q.ToWXYZ();
	EXPECT_NEAR(outgoing[0], eq.w(), Tolerance);
	EXPECT_NEAR(outgoing[1], eq.x(), Tolerance);
	EXPECT_NEAR(outgoing[2], eq.y(), Tolerance);
	EXPECT_NEAR(outgoing[3], eq.z(), Tolerance);
	ExpectVector(Quat::FromWXYZ(outgoing).Rotate(v), eq * Eigen::Vector3d(2, -1, 3));
}

TEST(RigidMath, BoxRotationTranslationAndInverse)
{
	const Box				box({1, 0.5, 0.2});
	const auto				half = box.GetHalfExtents();
	const Vec3				face{half[0], 0, 0}, localX{1, 0, 0};
	const auto				rotation = Quat::FromAxisAngle({0, 0, 7}, QuarterTurn);
	const Transform			pose({3, 2, 0}, rotation);
	const Eigen::Isometry3d reference(Eigen::Translation3d(3, 2, 0) * Eigen::AngleAxisd(QuarterTurn, Eigen::Vector3d::UnitZ()));
	const auto				point = pose.TransformPoint(face);
	ExpectVector(point, {3, 3, 0});
	ExpectVector(point, reference * Eigen::Vector3d(half[0], 0, 0));
	ExpectVector(pose.TransformDirection(localX), reference.linear() * Eigen::Vector3d::UnitX());
	ExpectVector(pose.Inverse().TransformPoint(point), {1, 0, 0});
	ExpectVector(pose.Inverse().TransformPoint({2, 5, 7}), reference.inverse() * Eigen::Vector3d(2, 5, 7));
	ExpectVector(rotation.Inverse().Rotate({0, 1, 0}), {1, 0, 0});

	const Transform turn({}, rotation), shift({3, 2, 0}, {});
	ExpectVector((shift * turn).TransformPoint(face), {3, 3, 0});
	ExpectVector((turn * shift).TransformPoint(face), {-2, 4, 0});
}

TEST(RigidMath, ParentChildComposition)
{
	const Transform			parent({3, 2, 0}, Quat::FromAxisAngle({0, 0, 1}, QuarterTurn));
	const Transform			child({0.5, -1, 2}, Quat::FromAxisAngle({1, 2, 0}, -0.73));
	const Eigen::Isometry3d ep(Eigen::Translation3d(3, 2, 0) * Eigen::AngleAxisd(QuarterTurn, Eigen::Vector3d::UnitZ()));
	const Eigen::Isometry3d ec(Eigen::Translation3d(0.5, -1, 2) * Eigen::AngleAxisd(-0.73, Eigen::Vector3d(1, 2, 0).normalized()));
	const Eigen::Isometry3d expected = ep * ec;
	const Transform			world = parent * child;
	ExpectVector(world.GetPosition(), expected.translation());
	for (double x : {-1., 1.})
		for (double y : {-0.5, 0.5})
			for (double z : {-0.2, 0.2})
			{
				const Vec3			  corner{x, y, z};
				const Eigen::Vector3d e(x, y, z);
				ExpectVector(world.TransformPoint(corner), expected * e);
				ExpectVector(parent.TransformPoint(child.TransformPoint(corner)), expected * e);
				ExpectVector(world.TransformDirection(corner), expected.linear() * e);
				ExpectVector(world.Inverse().TransformPoint(world.TransformPoint(corner)), e);
			}
}

TEST(RigidMath, RepeatedCompositionMaintainsUnitRotation)
{
	const auto step = Quat::FromAxisAngle({1, 2, 3}, 0.001);
	Quat	   total;
	for (int i = 0; i < 1000; ++i)
		total = total * step;
	const auto components = total.ToXYZW();
	EXPECT_NEAR(Eigen::Map<const Eigen::Vector4d>(components.data()).norm(), 1., Tolerance);
	ExpectVector(total.Rotate({1, 0, 0}), Eigen::AngleAxisd(1., Eigen::Vector3d(1, 2, 3).normalized()) * Eigen::Vector3d::UnitX());
}

TEST(RigidMath, RejectsInvalidRotationsAndPositions)
{
	EXPECT_THROW((void)Quat::FromXYZW({}), std::invalid_argument);
	EXPECT_THROW((void)Quat::FromAxisAngle({}, 1), std::invalid_argument);
	for (double bad : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
	{
		for (std::size_t i = 0; i < 4; ++i)
		{
			std::array components{1., 2., 3., 4.};
			components[i] = bad;
			EXPECT_THROW((void)Quat::FromXYZW(components), std::invalid_argument);
		}
		EXPECT_THROW((void)Quat::FromAxisAngle({1, bad, 0}, 1), std::invalid_argument);
		EXPECT_THROW((void)Quat::FromAxisAngle({1, 0, 0}, bad), std::invalid_argument);
		EXPECT_THROW((void)Transform({bad, 0, 0}, {}), std::invalid_argument);
	}
}
} // namespace
