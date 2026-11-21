#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

namespace HibikiPhysics
{
class Dependencies
{
  public:
	struct Requirement
	{
		std::string		mSlot;
		std::type_index mBindingType;
	};

	// The binding type also distinguishes a writable reference from a read-only one.
	template <class Protocol>
	void Require(std::string_view inSlot)
	{
		mRequired.push_back({std::string(inSlot), typeid(std::reference_wrapper<Protocol>)});
	}

	[[nodiscard]] std::span<const Requirement> GetRequired() const noexcept { return mRequired; }

  private:
	std::vector<Requirement> mRequired;
};

// One mounting scope. Each binding holds std::ref (or std::cref) of the
// selected protocol object. Providers outlive the scope and its registrations.
class PluginContext final
{
  public:
	using Bindings = std::map<std::string, std::any, std::less<>>;

	explicit PluginContext(Bindings inBindings = {});
	void Validate(const Dependencies &inDependencies) const;
	~PluginContext();
	PluginContext(const PluginContext &) = delete;
	PluginContext &operator=(const PluginContext &) = delete;

	template <class Protocol>
	[[nodiscard]] Protocol &Get(std::string_view inSlot) const
	{
		const auto entry = mBindings.find(inSlot);
		if (entry == mBindings.end())
			throw std::out_of_range(std::string(inSlot));
		return std::any_cast<std::reference_wrapper<Protocol>>(entry->second).get();
	}

	// Transfer an RAII registration whose destructor undoes the registration.
	template <class Registration>
	void Keep(Registration inRegistration)
	{
		mRegistrations.push_back(std::make_shared<Registration>(std::move(inRegistration)));
	}

  private:
	Bindings						   mBindings;
	std::vector<std::shared_ptr<void>> mRegistrations;
};

class Plugin
{
	friend class CapabilityConfig;

  public:
	virtual ~Plugin() = default;

	// Called during scene compilation. The default declares zero dependencies.
	virtual void Declare(Dependencies &) const {}
	// The host supplies the scope. Validate all declared slots before invoking OnMount.
	virtual void Mount(PluginContext &ioContext) final;

  protected:
	virtual void OnMount(PluginContext &ioContext) = 0;

  private:
	void MountDeclared(PluginContext &ioContext, const Dependencies &inDependencies);
};

// Providers are borrowed. Their lifetime covers the configuration and its mounted scopes.
class CapabilityRegistration
{
	friend class CapabilityConfig;

  public:
	template <class Protocol>
	CapabilityRegistration(std::string inCapability, std::string inImplementation, Protocol &ioProvider)
		: mCapability(std::move(inCapability)), mImplementation(std::move(inImplementation)), mBinding(std::ref(ioProvider))
	{
	}

  private:
	std::string mCapability;
	std::string mImplementation;
	std::any	mBinding;
};

// Copy a default configuration before registering replacements for an experiment.
class CapabilityConfig
{
  public:
	void										 Add(CapabilityRegistration inDefault);
	void										 Replace(CapabilityRegistration inReplacement);
	[[nodiscard]] PluginContext					 Build(const Dependencies &inDependencies) const;
	[[nodiscard]] std::unique_ptr<PluginContext> Mount(Plugin &ioPlugin) const;

  private:
	struct Entry
	{
		std::string mImplementation;
		std::any	mBinding;
		bool		mReplaced;
	};
	[[nodiscard]] PluginContext::Bindings	  Select(const Dependencies &inDependencies) const;
	std::map<std::string, Entry, std::less<>> mEntries;
};
} // namespace HibikiPhysics
