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

namespace
{
void ValidateBindings(const PluginContext::Bindings &inBindings, const Dependencies &inDependencies)
{
	for (const auto &required : inDependencies.GetRequired())
	{
		const auto binding = inBindings.find(required.mSlot);
		if (binding == inBindings.end())
			throw std::invalid_argument("Missing plugin binding: " + required.mSlot);
		if (required.mBindingType != binding->second.type())
			throw std::invalid_argument("Incompatible plugin binding type: " + required.mSlot);
	}
}
} // namespace

void PluginContext::Validate(const Dependencies &inDependencies) const
{
	ValidateBindings(mBindings, inDependencies);
}

void Plugin::Mount(PluginContext &ioContext)
{
	Dependencies dependencies;
	Declare(dependencies);
	MountDeclared(ioContext, dependencies);
}

void Plugin::MountDeclared(PluginContext &ioContext, const Dependencies &inDependencies)
{
	ioContext.Validate(inDependencies);
	OnMount(ioContext);
}

void CapabilityConfig::Add(CapabilityRegistration inDefault)
{
	const auto [entry, inserted] = mEntries.try_emplace(inDefault.mCapability, std::move(inDefault.mImplementation), std::move(inDefault.mBinding), false);
	if (!inserted)
		throw std::invalid_argument("Capability already registered: " + inDefault.mCapability + " (" + entry->second.mImplementation + ", " + inDefault.mImplementation + "). Use Replace.");
}

void CapabilityConfig::Replace(CapabilityRegistration inReplacement)
{
	const auto entry = mEntries.find(inReplacement.mCapability);
	if (entry == mEntries.end())
		throw std::invalid_argument("Missing default for replacement: " + inReplacement.mCapability);
	auto &selected = entry->second;
	if (selected.mReplaced)
		throw std::invalid_argument("Conflicting replacements: " + inReplacement.mCapability + " (" + selected.mImplementation + ", " + inReplacement.mImplementation + ")");
	if (selected.mBinding.type() != inReplacement.mBinding.type())
		throw std::invalid_argument("Incompatible replacement type: " + inReplacement.mCapability + " (" + inReplacement.mImplementation + ")");
	selected = {std::move(inReplacement.mImplementation), std::move(inReplacement.mBinding), true};
}

PluginContext::Bindings CapabilityConfig::Select(const Dependencies &inDependencies) const
{
	PluginContext::Bindings bindings;
	for (const auto &required : inDependencies.GetRequired())
	{
		const auto entry = mEntries.find(required.mSlot);
		if (entry != mEntries.end())
			bindings.try_emplace(required.mSlot, entry->second.mBinding);
	}
	return bindings;
}

PluginContext CapabilityConfig::Build(const Dependencies &inDependencies) const
{
	auto bindings = Select(inDependencies);
	ValidateBindings(bindings, inDependencies);
	return PluginContext(std::move(bindings));
}

std::unique_ptr<PluginContext> CapabilityConfig::Mount(Plugin &ioPlugin) const
{
	Dependencies dependencies;
	ioPlugin.Declare(dependencies);
	auto scope = std::make_unique<PluginContext>(Select(dependencies));
	ioPlugin.MountDeclared(*scope, dependencies);
	return scope;
}
} // namespace HibikiPhysics
