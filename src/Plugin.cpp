#include <HibikiPhysics/Plugin.h>

namespace HibikiPhysics
{
PluginContext::PluginContext(Bindings inBindings) : mBindings(std::move(inBindings))
{
}

PluginContext::PluginContext(const CapabilityConfig &inConfig, const Dependencies &inDependencies) : PluginContext()
{
	inConfig.Bind(*this, inDependencies);
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
	const auto [entry, inserted] = mEntries.try_emplace(inDefault.mCapability, std::move(inDefault.mImplementation), inDefault.mBindingType, std::move(inDefault.mBind), false);
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
	if (selected.mBindingType != inReplacement.mBindingType)
		throw std::invalid_argument("Incompatible replacement type: " + inReplacement.mCapability + " (" + inReplacement.mImplementation + ")");
	selected = {std::move(inReplacement.mImplementation), inReplacement.mBindingType, std::move(inReplacement.mBind), true};
}

void CapabilityConfig::Bind(PluginContext &ioContext, const Dependencies &inDependencies) const
{
	// Capture the selection before user factories can change the configuration.
	std::vector<std::pair<std::string, Entry>> selected;
	selected.reserve(inDependencies.GetRequired().size());
	for (const auto &required : inDependencies.GetRequired())
	{
		const auto entry = mEntries.find(required.mSlot);
		if (entry == mEntries.end())
			throw std::invalid_argument("Missing plugin binding: " + required.mSlot);
		if (entry->second.mBindingType != required.mBindingType)
			throw std::invalid_argument("Incompatible plugin binding type: " + required.mSlot);
		selected.emplace_back(entry->first, entry->second);
	}
	for (const auto &[slot, entry] : selected)
	{
		if (ioContext.mBindings.contains(slot))
			continue;
		auto binding = entry.mBind(ioContext);
		if (!binding.has_value())
			throw std::runtime_error("Provider factory returned null: " + slot + " (" + entry.mImplementation + ")");
		ioContext.mBindings.emplace(slot, std::move(binding));
	}
}

PluginContext CapabilityConfig::Build(const Dependencies &inDependencies) const
{
	return PluginContext(*this, inDependencies);
}

std::unique_ptr<PluginContext> CapabilityConfig::Mount(Plugin &ioPlugin) const
{
	Dependencies dependencies;
	ioPlugin.Declare(dependencies);
	auto scope = std::make_unique<PluginContext>();
	Bind(*scope, dependencies);
	ioPlugin.MountDeclared(*scope, dependencies);
	return scope;
}
} // namespace HibikiPhysics
