#include <HibikiPhysics/Scene.h>
#include <iostream>
#include <sstream>
#include <limits>

namespace
{
using namespace HibikiPhysics;

void Check(bool inCondition, const char *inMessage)
{
	if (!inCondition)
		throw std::logic_error(inMessage);
}

void CheckBridge()
{
	for (const char *record : {"mesh ground 1", "sphere ball 0 -1 1 1", "plane p 0 4294967296 1", "plane p 0 1 -1", "plane p nan 1 1", "sphere s 1.0.5 1 2", "plane p 0 1 2 unknown"})
	{
		std::istringstream input(std::string("hibiki-scene 1\n") + record);
		try
		{
			(void)ReadScene(input, "bad.hscene");
			throw std::logic_error("malformed scene accepted");
		}
		catch (const std::invalid_argument &error)
		{
			Check(std::string_view(error.what()).starts_with("bad.hscene:2: "), "source diagnostic");
		}
	}
	std::istringstream	reversed("hibiki-scene 1\nsphere ball 0.625 0.5 1 2\nplane ground 0 2 1");
	const CompiledScene fromStream(ReadScene(reversed, "stream"));
	const CompiledScene fromCode({"programmatic", SceneSphere{"ball", 0.625, 0.5, {1, 2}}, ScenePlane{"ground", 0, {2, 1}}});
	Check(fromStream.QueryContact(0.25) == -0.25 && fromCode.QueryContact(0.25) == fromStream.QueryContact(0.25), "equivalent sources");
}

class CountingService final : public CollisionService
{
  public:
	std::size_t								 mRegistrations = 0;
	mutable std::size_t						 mQueries = 0;
	std::unique_ptr<SpherePlaneRegistration> RegisterSpherePlane(SpherePlaneDetector inDetector) override
	{
		++mRegistrations;
		return CollisionService::RegisterSpherePlane(std::move(inDetector));
	}
	std::optional<double> Query(const SceneSphere &inSphere, const ScenePlane &inPlane, double inCenterZ) const override
	{
		++mQueries;
		return CollisionService::Query(inSphere, inPlane, inCenterZ);
	}
};

void CheckServiceLifetime(const SceneDescription &inScene)
{
	CountingService service;
	auto			config = DefaultSceneConfig();
	config.Replace(CollisionServiceSlot.Borrow("external.service", service));
	int					calls = 0;
	SpherePlaneDetector detector = [&](const SceneSphere &, const ScenePlane &, double center)
	{
		++calls;
		return std::optional(center);
	};
	config.Replace(SpherePlaneSlot.Borrow("external.detector", detector));
	{
		const CompiledScene scene(inScene, config);
		Check(scene.QueryContact(2) == 2 && service.Query(*inScene.mSphere, *inScene.mPlane, 3) == 3 && calls == 2 && service.mRegistrations == 1 && service.mQueries == 2, "same selected service and borrowed detector");
	}
	Check(!service.Query(*inScene.mSphere, *inScene.mPlane, 3) && calls == 2, "detector detached from borrowed service");
	Check(CompiledScene(inScene, config).QueryContact(4) == 4 && calls == 3, "service reusable after teardown");
}
void Run(const std::filesystem::path &inPath)
{
	auto scene = LoadScene(inPath);
	CheckServiceLifetime(scene);
	Check(scene.mSource == inPath.string() && scene.mSphere->mFilter.mMask == 2 && scene.mPlane->mFilter.mGroup == 2, "bridge source facts");
	auto				config = DefaultSceneConfig();
	const CompiledScene baseline(scene, config);
	config.Replace(SpherePlaneWithMargin("experiment.0.25m", 0.25));
	const CompiledScene experiment(scene, config);
	const auto			defaults = baseline.GetSelections();
	const auto			overrides = experiment.GetSelections();
	Check(defaults.size() == 3 && defaults[2].mSlot == "collision.sphere-plane.registration" && defaults[0].mSlot == "collision.service" && defaults[0].mImplementation == "hibiki.collision.service.v1" && defaults[1].mSlot == "collision.sphere-plane" && !defaults[1].mReplaced && defaults[1].mImplementation == "hibiki.sphere-plane.exact.v1", "default selection");
	Check(overrides.size() == 3 && !overrides[2].mReplaced && !overrides[0].mReplaced && overrides[1].mReplaced && overrides[1].mImplementation == "experiment.0.25m", "experiment selection");
	for (double center : {0.625, 0.25, 0.75, 0.5, 1.0})
	{
		double gap = center - 0.5;
		Check(baseline.QueryContact(center) == (gap <= 0 ? std::optional(gap) : std::nullopt), "baseline step query");
		Check(experiment.QueryContact(center) == (gap <= 0.25 ? std::optional(gap) : std::nullopt), "experiment step query");
	}
	try
	{
		CollisionService service;
		CapabilityConfig missingDetector;
		missingDetector.Add(CollisionServiceSlot.Borrow("only.service", service));
		CompiledScene missing(scene, missingDetector);
		throw std::logic_error("missing provider accepted");
	}
	catch (const std::runtime_error &error)
	{
		std::string message(error.what());
		for (const auto &fact : {scene.mSource, scene.mSphere->mName, scene.mPlane->mName, std::string("Missing plugin binding: collision.sphere-plane required by")})
			Check(message.find(fact) != std::string::npos, "missing diagnostic fact");
	}
	auto extreme = scene;
	extreme.mPlane->mHeight = std::numeric_limits<double>::max();
	try
	{
		(void)CompiledScene(extreme).QueryContact(-std::numeric_limits<double>::max());
		throw std::logic_error("unrepresentable gap accepted");
	}
	catch (const std::overflow_error &)
	{
	}
	scene.mSphere->mFilter.mMask = 0;
	const CompiledScene filtered(scene, {});
	Check(filtered.GetSelections().empty() && !filtered.QueryContact(0), "filtered scope");
	scene.mSphere->mFilter.mMask = 2;
	scene.mPlane->mFilter.mMask = 0;
	Check(!CompiledScene(scene, {}).QueryContact(0), "reverse filter");
	scene.mPlane.reset();
	Check(CompiledScene(scene, {}).GetSelections().empty(), "sphere only");
}
} // namespace

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	try
	{
		CheckBridge();
		Run(argv[1]);
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
