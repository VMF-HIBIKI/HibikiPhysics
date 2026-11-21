#include <HibikiPhysics/Scene.h>

#include <charconv>
#include <cmath>
#include <fstream>
#include <locale>

#include <sstream>

namespace HibikiPhysics
{
namespace
{
const CapabilitySlot<SpherePlaneRegistration> RegistrationSlot("collision.sphere-plane.registration");

template <class Number>
Number ReadNumber(std::istream &ioLine, const char *inExpected)
{
	std::string token;
	ioLine >> token;
	Number	   value{};
	const auto end = token.data() + token.size();
	const auto result = std::from_chars(token.data(), end, value);
	if (result.ec != std::errc{} || result.ptr != end)
		throw std::invalid_argument(inExpected);
	return value;
}

void Validate(const SceneDescription &inScene)
{
	const auto &sphere = inScene.mSphere;
	const auto &plane = inScene.mPlane;
	if (sphere && (sphere->mName.empty() || !std::isfinite(sphere->mCenterZ) ||
				   !std::isfinite(sphere->mRadius) || sphere->mRadius <= 0))
		throw std::invalid_argument("invalid sphere '" + sphere->mName + "'");
	if (plane && (plane->mName.empty() || !std::isfinite(plane->mHeight)))
		throw std::invalid_argument("invalid plane '" + plane->mName + "'");
	if (sphere && plane && sphere->mName == plane->mName)
		throw std::invalid_argument("duplicate shape name '" + plane->mName + "'");
}

bool CanCollide(const CollisionFilter &inFirst, const CollisionFilter &inSecond)
{
	return (inFirst.mMask & inSecond.mGroup) != 0 && (inSecond.mMask & inFirst.mGroup) != 0;
}
} // namespace

SceneDescription LoadScene(const std::filesystem::path &inPath)
{
	std::ifstream file(inPath);
	if (!file)
		throw std::runtime_error(inPath.string() + ": cannot open scene");
	return ReadScene(file, inPath.string());
}

SceneDescription ReadScene(std::istream &inStream, std::string inSource)
{
	SceneDescription scene{std::move(inSource), {}, {}};
	std::string		 line;
	bool			 header = false;
	std::size_t		 number = 0;
	while (std::getline(inStream, line))
	{
		++number;
		std::istringstream row(line);
		row.imbue(std::locale::classic());
		std::string kind;
		if (!(row >> kind))
			continue;
		try
		{
			if (!header)
			{
				std::string version;
				if (kind != "hibiki-scene" || !(row >> version) || version != "1")
					throw std::invalid_argument("expected hibiki-scene 1");
				header = true;
			}
			else if (kind == "sphere" && !scene.mSphere)
			{
				SceneSphere sphere{};
				row >> sphere.mName;
				sphere.mCenterZ = ReadNumber<double>(row, "expected sphere centre Z");
				sphere.mRadius = ReadNumber<double>(row, "expected sphere radius");
				sphere.mFilter = {ReadNumber<std::uint32_t>(row, "expected unsigned decimal filter bits"), ReadNumber<std::uint32_t>(row, "expected unsigned decimal filter bits")};
				scene.mSphere = std::move(sphere);
			}
			else if (kind == "plane" && !scene.mPlane)
			{
				ScenePlane plane{};
				row >> plane.mName;
				plane.mHeight = ReadNumber<double>(row, "expected plane height");
				plane.mFilter = {ReadNumber<std::uint32_t>(row, "expected unsigned decimal filter bits"), ReadNumber<std::uint32_t>(row, "expected unsigned decimal filter bits")};
				scene.mPlane = std::move(plane);
			}
			else
				throw std::invalid_argument("unsupported or repeated shape '" + kind + "'");
			std::string extra;
			if (row >> extra)
				throw std::invalid_argument("unexpected field '" + extra + "'");
			Validate(scene);
		}
		catch (const std::invalid_argument &error)
		{
			throw std::invalid_argument(scene.mSource + ":" + std::to_string(number) + ": " + error.what());
		}
	}
	if (inStream.bad() || !inStream.eof())
		throw std::runtime_error(scene.mSource + ": error reading scene");
	if (!header)
		throw std::invalid_argument(scene.mSource + ": missing hibiki-scene 1 header");
	return scene;
}

std::unique_ptr<SpherePlaneRegistration> CollisionService::RegisterSpherePlane(SpherePlaneDetector inDetector)
{
	if (!inDetector)
		throw std::invalid_argument("Empty sphere-plane detector");
	if (!mDetector.expired())
		throw std::logic_error("Sphere-plane detector already mounted");
	auto registration = std::unique_ptr<SpherePlaneRegistration>(new SpherePlaneRegistration(std::move(inDetector)));
	mDetector = registration->mDetector;
	return registration;
}

std::optional<double> CollisionService::Query(const SceneSphere &inSphere, const ScenePlane &inPlane, double inCenterZ) const
{
	const auto detector = mDetector.lock();
	if (!detector)
		return std::nullopt;
	return (*detector)(inSphere, inPlane, inCenterZ);
}

CapabilityRegistration SpherePlaneWithMargin(std::string inImplementation, double inMargin)
{
	if (!std::isfinite(inMargin) || inMargin < 0)
		throw std::invalid_argument("Invalid sphere-plane contact margin");
	auto detect = [inMargin](const SceneSphere &inSphere, const ScenePlane &inPlane, double inCenterZ) -> std::optional<double>
	{
		const double gap = inCenterZ - inPlane.mHeight - inSphere.mRadius;
		if (!std::isfinite(gap))
			throw std::overflow_error("Sphere-plane gap exceeds finite range");
		if (gap <= inMargin)
			return gap;
		return std::nullopt;
	};
	return SpherePlaneSlot.Factory(std::move(inImplementation), [detect]
								   { return std::make_unique<SpherePlaneDetector>(detect); });
}

CapabilityConfig DefaultSceneConfig()
{
	CapabilityConfig config;
	config.Add(CollisionServiceSlot.Factory("hibiki.collision.service.v1", []
											{ return std::make_unique<CollisionService>(); }));
	config.Add(SpherePlaneWithMargin("hibiki.sphere-plane.exact.v1", 0));
	return config;
}

CompiledScene::CompiledScene(SceneDescription inScene, const CapabilityConfig &inConfig) : mScene(std::move(inScene))
{
	try
	{
		Validate(mScene);
	}
	catch (const std::invalid_argument &error)
	{
		throw std::invalid_argument(mScene.mSource + ": " + error.what());
	}
	const auto &sphere = mScene.mSphere;
	const auto &plane = mScene.mPlane;
	if (!sphere || !plane || !CanCollide(sphere->mFilter, plane->mFilter))
	{
		auto empty = MakePlugin([] {});
		mScope = inConfig.Mount(empty);
		return;
	}
	auto query = MakePlugin([this](SpherePlaneRegistration &, CollisionService &ioService)
							{ mService = &ioService; }, RegistrationSlot, CollisionServiceSlot);
	try
	{
		auto config = inConfig;
		auto registerPair = [](CollisionService &ioService, SpherePlaneDetector &ioDetector)
		{
			return ioService.RegisterSpherePlane(std::ref(ioDetector));
		};
		config.Add(RegistrationSlot.Factory("hibiki.sphere-plane.registration.v1", registerPair, CollisionServiceSlot, SpherePlaneSlot));
		mScope = config.Mount(query);
	}
	catch (const std::exception &error)
	{
		throw std::runtime_error(mScene.mSource + ": sphere '" + sphere->mName + "' and plane '" +
								 plane->mName + "' require collision.sphere-plane: " + error.what());
	}
}

std::optional<double> CompiledScene::QueryContact(double inCenterZ) const
{
	if (!std::isfinite(inCenterZ))
		throw std::invalid_argument("Invalid sphere centre Z");
	if (!mScope || !mService)
		return std::nullopt;
	return mService->Query(*mScene.mSphere, *mScene.mPlane, inCenterZ);
}
} // namespace HibikiPhysics
