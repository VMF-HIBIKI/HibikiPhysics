#include <HibikiPhysics/Plugin.h>
#include <iostream>

namespace
{
using namespace HibikiPhysics;

struct Tracked
{
	std::string &mLog;
	char		 mMarker;
	~Tracked() { mLog.push_back(mMarker); }
};

void Check(bool inCondition, const char *inMessage)
{
	if (!inCondition)
		throw std::logic_error(inMessage);
}

template <class Error, class Run>
void ExpectError(Run inRun, std::string_view inExpected)
{
	try
	{
		inRun();
	}
	catch (const Error &error)
	{
		if (error.what() == inExpected)
			return;
		throw;
	}
	throw std::logic_error("Expected operation to fail");
}

void CheckInjection()
{
	const CapabilitySlot<int>		left("left"), right("right");
	const CapabilitySlot<const int> offset("offset");
	int								calls = 0;
	const int						source = 3;
	CapabilityConfig				config;
	config.Add(left.Factory("left-default", [&]
							{ ++calls; return std::make_unique<int>(1); }));
	config.Add(right.Factory("right-default", []
							 { return std::make_unique<int>(5); }));
	config.Add(offset.Borrow("offset-default", source));
	auto plugin = MakePlugin([&](int &a, int &b, const int &c, int &again)
							 {
		Check(&a == &again && &a != &b && &c == &source, "slot identity and borrowed const reference");
		a += b + c; }, left, right, offset, left);

	const auto first = config.Mount(plugin);
	const auto second = config.Mount(plugin);
	Check(calls == 2 && left.Get(*first) == 9 && left.Get(*second) == 9 && &left.Get(*first) != &left.Get(*second), "one instance per requested slot and scope");
	auto experiment = config;
	experiment.Replace(left.Factory("alternative", []
									{ return std::make_unique<int>(10); }));
	const auto alternative = experiment.Mount(plugin);
	Check(left.Get(*alternative) == 18 && left.Get(*first) == 9 && calls == 2, "isolated factory replacement");

	Dependencies request;
	right.Declare(request);
	const auto subset = config.Build(request);
	Check(right.Get(subset) == 5 && calls == 2, "unrequested factory ran");
	left.Declare(request);
	request.Require<int>("missing");
	ExpectError<std::invalid_argument>([&]
									   { (void)config.Build(request); }, "Missing plugin binding: missing");
	Check(calls == 2, "factory ran before validating all requirements");
}

void CheckSelectionDuringBuild()
{
	const CapabilitySlot<int> a("a"), b("b");
	CapabilityConfig		  config;
	bool					  replaced = false;
	config.Add(a.Factory("first", [&]
						 {
		if (!replaced)
		{
			config.Replace(b.Factory("next", [] { return std::make_unique<int>(2); }));
			replaced = true;
		}
		return std::make_unique<int>(0); }));
	config.Add(b.Factory("current", []
						 { return std::make_unique<int>(1); }));
	Dependencies request;
	a.Declare(request);
	b.Declare(request);
	const auto current = config.Build(request);
	const auto next = config.Build(request);
	Check(b.Get(current) == 1 && b.Get(next) == 2, "each build uses one selection");
}

void CheckLifetime()
{
	std::string log;
	log.reserve(16);
	const CapabilitySlot<Tracked> first("first"), second("second");
	CapabilityConfig			  defaults;
	defaults.Add(first.Factory("A", [&]
							   { return std::make_unique<Tracked>(log, 'A'); }));
	defaults.Add(second.Factory("B", [&]
								{ return std::make_unique<Tracked>(log, 'B'); }));
	auto registerEffect = [&](PluginContext &ctx, Tracked &, Tracked &)
	{ ctx.Keep(std::make_unique<Tracked>(log, 'R')); };
	auto plugin = MakePlugin(registerEffect, first, second);
	{
		const auto scope = defaults.Mount(plugin);
		Check(log.empty(), "premature destruction");
	}
	Check(log == "RBA", "registrations must be released before providers");
	log.clear();
	auto failing = MakePlugin([&](PluginContext &ctx, Tracked &a, Tracked &b)
							  {
		registerEffect(ctx, a, b);
		throw std::runtime_error("mount failed"); }, first, second);
	ExpectError<std::runtime_error>([&]
									{ (void)defaults.Mount(failing); }, "mount failed");
	Check(log == "RBA", "partial mount cleanup");
	log.clear();
	Dependencies required;
	first.Declare(required);
	second.Declare(required);
	auto experiment = defaults;
	experiment.Replace(second.Factory("throwing", []() -> std::unique_ptr<Tracked>
									  { throw std::runtime_error("factory failed"); }));
	ExpectError<std::runtime_error>([&]
									{ (void)experiment.Build(required); }, "factory failed");
	Check(log == "A", "partial factory cleanup");
	auto empty = defaults;
	empty.Replace(second.Factory("empty", []
								 { return std::unique_ptr<Tracked>{}; }));
	ExpectError<std::runtime_error>([&]
									{ (void)empty.Mount(plugin); }, "Provider factory returned null: second (empty)");
	Check(log == "AA", "null factory cleanup");
}
} // namespace

int main()
{
	try
	{
		CheckInjection();
		CheckSelectionDuringBuild();
		CheckLifetime();
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
