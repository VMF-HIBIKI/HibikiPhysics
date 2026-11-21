#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/Math.h>
#include <iostream>
#include <numbers>

int main()
{
	using namespace HibikiPhysics;
	const Box		box({1, 0.5, 0.2});
	const Vec3		faceCentre{box.GetHalfExtents()[0], 0, 0};
	const Quat		turn = Quat::FromAxisAngle({0, 0, 1}, std::numbers::pi_v<double> / 2);
	const Transform parentFromBox({3, 2, 0}, turn);
	const Vec3		point = parentFromBox.TransformPoint(faceCentre);
	std::cout << "Face centre in parent coordinates (m): " << point.mX << ' ' << point.mY << ' ' << point.mZ << '\n';
}
