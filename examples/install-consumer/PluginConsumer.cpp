#include <HibikiPhysics/Plugin.h>
#include <functional>

int main()
{
	HibikiPhysics::Dependencies dependencies;
	dependencies.Require<int>("input");

	int									   value = 10;
	HibikiPhysics::PluginContext::Bindings bindings{{"input", std::ref(value)}};
	HibikiPhysics::PluginContext		   context(bindings);
	context.Validate(dependencies);
	context.Get<int>("input") = 12;
	return value == 12 ? 0 : 1;
}
