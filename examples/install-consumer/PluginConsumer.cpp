#include <HibikiPhysics/Plugin.h>
#include <functional>

int main()
{
	HibikiPhysics::Dependencies dependencies;
	dependencies.Require<int>("input");

	int									   value = 10;
	HibikiPhysics::PluginContext::Bindings bindings{{"input", std::ref(value)}};
	const auto							  &input = dependencies.GetRequired().front();
	if (bindings.at(input.mSlot).type() != input.mBindingType)
		return 1;

	HibikiPhysics::PluginContext context(bindings);
	context.Get<int>("input") = 12;
	return value == 12 ? 0 : 2;
}
