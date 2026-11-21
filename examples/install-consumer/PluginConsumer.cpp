#include <HibikiPhysics/Plugin.h>
#include <memory>

using HibikiPhysics::CapabilityConfig;
using HibikiPhysics::CapabilitySlot;

CapabilityConfig IntegerDefaults(const CapabilitySlot<int> &inInput)
{
	CapabilityConfig config;
	config.Add(inInput.Factory("baseline", []
							   { return std::make_unique<int>(10); }));
	return config;
}

int main()
{
	const CapabilitySlot<int> input("input");
	const auto				  defaults = IntegerDefaults(input);
	auto					  experiment = defaults;
	experiment.Replace(input.Factory("trial", []
									 { return std::make_unique<int>(20); }));

	auto	   increment = HibikiPhysics::MakePlugin([](int &value)
													 { ++value; }, input);
	const auto baseline = defaults.Mount(increment);
	const auto trial = experiment.Mount(increment);
	return input.Get(*baseline) == 11 && input.Get(*trial) == 21 ? 0 : 1;
}
