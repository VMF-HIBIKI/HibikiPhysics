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
	const auto [entry, inserted] = mEntries.try_emplace(inDefault.mCapability, std::move(inDefault.mImplementation), inDefault.mBindingType, std::move(inDefault.mBind), false, std::move(inDefault.mDependencies));
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
	selected = {std::move(inReplacement.mImplementation), inReplacement.mBindingType, std::move(inReplacement.mBind), true, std::move(inReplacement.mDependencies)};
}

void CapabilityConfig::Bind(PluginContext &ioContext, const Dependencies &inDependencies) const
{
	// Each frame walks one declaration in order. Completing it places its factory
	// after all prerequisites. The first frame holds the caller's requested slots.
	struct Frame
	{
		std::string_view						   mSlot;
		const Entry								  *mEntry;
		std::span<const Dependencies::Requirement> mRemaining;
	};
	enum class Visit
	{
		Visiting,
		Complete
	};
	std::map<std::string_view, Visit>												 visits;
	std::vector<Frame>																 stack{{{}, nullptr, inDependencies.GetRequired()}};
	std::vector<std::pair<PluginContext::Selection, CapabilityRegistration::Binder>> plan;
	while (!stack.empty())
	{
		auto &frame = stack.back();
		if (frame.mRemaining.empty())
		{
			if (frame.mEntry)
			{
				const auto &entry = *frame.mEntry;
				plan.emplace_back(PluginContext::Selection{std::string(frame.mSlot), entry.mImplementation, entry.mReplaced}, entry.mBind);
				visits.at(frame.mSlot) = Visit::Complete;
			}
			stack.pop_back();
			continue;
		}
		const auto &required = frame.mRemaining.front();
		frame.mRemaining = frame.mRemaining.subspan(1);
		const auto entry = mEntries.find(required.mSlot);
		if (entry == mEntries.end())
			throw std::invalid_argument("Missing plugin binding: " + required.mSlot + (frame.mEntry ? " required by " + std::string(frame.mSlot) : ""));
		if (entry->second.mBindingType != required.mBindingType)
			throw std::invalid_argument("Incompatible plugin binding type: " + required.mSlot + (frame.mEntry ? " required by " + std::string(frame.mSlot) : ""));
		const auto [visit, first] = visits.try_emplace(entry->first, Visit::Visiting);
		if (first)
			stack.push_back({entry->first, &entry->second, entry->second.mDependencies.GetRequired()});
		else if (visit->second == Visit::Visiting)
		{
			std::string path;
			for (const auto &ancestor : stack)
				if (ancestor.mEntry)
					path += std::string(ancestor.mSlot) + " -> ";
			throw std::invalid_argument("Plugin dependency cycle: " + path + required.mSlot);
		}
	}

	// Every selected binder is captured before user code can change the configuration.
	ioContext.mSelections.reserve(plan.size());
	for (auto &[selection, bind] : plan)
	{
		auto binding = bind(ioContext);
		if (!binding.has_value())
			throw std::runtime_error("Provider factory returned null: " + selection.mSlot + " (" + selection.mImplementation + ")");
		ioContext.mBindings.emplace(selection.mSlot, std::move(binding));
		ioContext.mSelections.push_back(std::move(selection));
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
