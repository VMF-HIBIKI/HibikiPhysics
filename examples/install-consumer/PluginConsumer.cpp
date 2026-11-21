#include <HibikiPhysics/Plugin.h>

int main()
{
	int defaultValue = 10;
	int experimentValue = 20;

	HibikiPhysics::CapabilityConfig defaults;
	defaults.Add({"input", "baseline", defaultValue});
	auto experiment = defaults;
	experiment.Replace({"input", "trial", experimentValue});

	HibikiPhysics::Dependencies required;
	required.Require<int>("input");
	auto baseline = defaults.Build(required);
	auto trial = experiment.Build(required);

	baseline.Get<int>("input") += 1;
	trial.Get<int>("input") += 2;
	return defaultValue == 11 && experimentValue == 22 ? 0 : 1;
}
