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

void PluginContext::Validate(const Dependencies &inDependencies) const
{
	for (const auto &required : inDependencies.GetRequired())
	{
		const auto binding = mBindings.find(required.mSlot);
		if (binding == mBindings.end())
			throw std::invalid_argument("Missing plugin binding: " + required.mSlot);
		if (required.mBindingType != binding->second.type())
			throw std::invalid_argument("Incompatible plugin binding type: " + required.mSlot);
	}
}

void Plugin::Mount(PluginContext &ioContext)
{
	Dependencies dependencies;
	Declare(dependencies);
	ioContext.Validate(dependencies);
	OnMount(ioContext);
}
} // namespace HibikiPhysics
