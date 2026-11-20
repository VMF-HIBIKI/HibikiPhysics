#include <HibikiPhysics/Plugin.h>

namespace HibikiPhysics
{
PluginContext::PluginContext(Bindings inBindings) : mBindings(std::move(inBindings))
{
}

PluginContext::~PluginContext()
{
	// A later registration may depend on an earlier one.
	while (!mRegistrations.empty())
		mRegistrations.pop_back();
}
} // namespace HibikiPhysics
