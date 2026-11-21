#include <HibikiPhysics/Plugin.h>
#include <set>
#include <type_traits>

namespace
{
using namespace HibikiPhysics;

struct CollisionService
{
	struct Registration
	{
		explicit Registration(CollisionService &ioService) : mService(ioService) { mService.mDetectors.insert(this); }
		~Registration() { mService.mDetectors.erase(this); }
		Registration(const Registration &) = delete;
		Registration	 &operator=(const Registration &) = delete;
		CollisionService &mService;
	};

	std::set<const Registration *> mDetectors;
	auto						   RegisterDetector() { return std::make_unique<Registration>(*this); }
};

class DetectorPlugin final : public Plugin
{
  public:
	void Declare(Dependencies &outDeps) const override { outDeps.Require<CollisionService>("collision"); }
	int	 mMountCalls = 0;

  protected:
	void OnMount(PluginContext &ioContext) override
	{
		++mMountCalls;
		ioContext.Keep(ioContext.Get<CollisionService>("collision").RegisterDetector());
	}
};

bool RejectsBindings(Plugin &inPlugin, PluginContext::Bindings inBindings, std::string_view inExpected)
{
	PluginContext scope(std::move(inBindings));
	try
	{
		inPlugin.Mount(scope);
	}
	catch (const std::invalid_argument &error)
	{
		return error.what() == inExpected;
	}
	return false;
}

static_assert(std::is_abstract_v<Plugin> && std::has_virtual_destructor_v<Plugin>);
} // namespace

int main()
{
	using namespace HibikiPhysics;
	CollisionService		first, second;
	auto					detector = std::make_unique<DetectorPlugin>();
	const int			   &mountCalls = detector->mMountCalls;
	std::unique_ptr<Plugin> plugin = std::move(detector);
	if (!RejectsBindings(*plugin, {}, "Missing plugin binding: collision") ||
		!RejectsBindings(*plugin, {{"collision", std::cref(second)}}, "Incompatible plugin binding type: collision") ||
		mountCalls != 0)
		return 7;
	Dependencies dependencies;
	plugin->Declare(dependencies);

	PluginContext::Bindings bindings{{"collision", std::ref(second)}};
	const auto				required = dependencies.GetRequired();
	if (required.size() != 1 || required[0].mSlot != "collision" || required[0].mBindingType != bindings.at("collision").type())
		return 1;
	{
		PluginContext scope(bindings);
		plugin->Mount(scope);
		if (mountCalls != 1 || second.mDetectors.size() != 1 || !first.mDetectors.empty())
			return 2;
		const auto *surviving = *second.mDetectors.begin();
		try
		{
			PluginContext trial(bindings);
			plugin->Mount(trial);
			if (second.mDetectors.size() != 2)
				return 3;
			// An incompatible lookup unwinds the trial scope.
			(void)trial.Get<int>("collision");
			return 4;
		}
		catch (const std::bad_any_cast &)
		{
			if (second.mDetectors.size() != 1 || !second.mDetectors.contains(surviving))
				return 5;
		}
	}
	return second.mDetectors.empty() ? 0 : 6;
}
