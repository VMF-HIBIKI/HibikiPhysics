#include <HibikiPhysics/Plugin.h>
#include <iostream>

namespace
{
using namespace HibikiPhysics;

class AddOffset final : public Plugin
{
  public:
	int	 mMountCalls = 0;
	void Declare(Dependencies &outDeps) const override
	{
		outDeps.Require<int>("input");
		outDeps.Require<const int>("offset");
	}

  protected:
	void OnMount(PluginContext &ioContext) override
	{
		++mMountCalls;
		ioContext.Get<int>("input") += ioContext.Get<const int>("offset");
	}
};

template <class Operation>
bool Rejects(Operation inOperation, std::string_view inMessage)
{
	try
	{
		inOperation();
	}
	catch (const std::invalid_argument &error)
	{
		if (error.what() == inMessage)
			return true;
		std::cerr << error.what() << '\n';
	}
	std::cerr << "Expected: " << inMessage << '\n';
	return false;
}
} // namespace

int main()
{
	using namespace HibikiPhysics;
	int				 baseline = 10;
	int				 trial = 20;
	const int		 offset = 3;
	CapabilityConfig defaults;
	defaults.Add({"input", "official", baseline});
	AddOffset plugin;
	if (!Rejects([&]
				 { (void)defaults.Mount(plugin); }, "Missing plugin binding: offset") ||
		plugin.mMountCalls != 0)
		return 1;
	defaults.Add({"offset", "official", offset});
	auto	   experiment = defaults;
	const auto originalScope = experiment.Mount(plugin);
	if (baseline != 13 || trial != 20)
		return 2;

	if (!Rejects([&]
				 { experiment.Add({"input", "trial", trial}); }, "Capability already registered: input (official, trial). Use Replace.") ||
		!Rejects([&]
				 { experiment.Replace({"unknown", "trial", trial}); }, "Missing default for replacement: unknown") ||
		!Rejects([&]
				 { experiment.Replace({"input", "readonly", offset}); }, "Incompatible replacement type: input (readonly)"))
		return 3;
	experiment.Replace({"input", "trial", trial});
	if (!Rejects([&]
				 { experiment.Replace({"input", "other", baseline}); }, "Conflicting replacements: input (trial, other)"))
		return 4;
	const auto trialScope = experiment.Mount(plugin);
	const auto defaultScope = defaults.Mount(plugin);
	if (baseline != 16 || trial != 23 || plugin.mMountCalls != 3)
		return 5;
	if (&originalScope->Get<int>("input") != &baseline || &defaultScope->Get<int>("input") != &baseline ||
		&trialScope->Get<int>("input") != &trial || &trialScope->Get<const int>("offset") != &offset)
		return 6;

	Dependencies incompatible;
	incompatible.Require<int>("offset");
	return Rejects([&]
				   { (void)experiment.Build(incompatible); }, "Incompatible plugin binding type: offset")
			   ? 0
			   : 7;
}
