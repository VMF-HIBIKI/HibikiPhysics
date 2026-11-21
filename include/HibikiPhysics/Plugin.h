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
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace HibikiPhysics
{
class CapabilityConfig;

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
	friend class CapabilityConfig;

  public:
	using Bindings = std::map<std::string, std::any, std::less<>>;
	struct Selection
	{
		std::string mSlot;
		std::string mImplementation;
		bool		mReplaced;
	};
	// Provider construction order in a scope assembled by CapabilityConfig.
	[[nodiscard]] std::span<const Selection> GetSelections() const noexcept { return mSelections; }

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
	PluginContext(const CapabilityConfig &inConfig, const Dependencies &inDependencies);
	std::vector<Selection>			   mSelections;
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

class CapabilityRegistration
{
	friend class CapabilityConfig;
	template <class>
	friend class CapabilitySlot;
	using Binder = std::function<std::any(PluginContext &)>;

  public:
	template <class Protocol>
	CapabilityRegistration(std::string inCapability, std::string inImplementation, Protocol &ioProvider)
		: CapabilityRegistration(std::move(inCapability), std::move(inImplementation), typeid(decltype(std::ref(ioProvider))),
								 [reference = std::ref(ioProvider)](PluginContext &) -> std::any
								 { return reference; })
	{
	}

  private:
	CapabilityRegistration(std::string inCapability, std::string inImplementation, std::type_index inBindingType, Binder inBind)
		: mCapability(std::move(inCapability)), mImplementation(std::move(inImplementation)), mBindingType(inBindingType), mBind(std::move(inBind)) {}
	std::string		mCapability;
	std::string		mImplementation;
	std::type_index mBindingType;
	Binder			mBind;
	Dependencies	mDependencies;
};

// A protocol type and its stable name together distinguish slots sharing the same C++ type.
template <class Protocol>
class CapabilitySlot
{
  public:
	explicit CapabilitySlot(std::string inName) : mName(std::move(inName)) {}
	void					Declare(Dependencies &outDependencies) const { outDependencies.Require<Protocol>(mName); }
	[[nodiscard]] Protocol &Get(const PluginContext &inContext) const { return inContext.Get<Protocol>(mName); }

	template <class Provider>
		requires std::is_convertible_v<Provider *, Protocol *>
	[[nodiscard]] CapabilityRegistration Borrow(std::string inImplementation, Provider &ioProvider) const
	{
		Protocol &reference = ioProvider;
		return {mName, std::move(inImplementation), reference};
	}

	// A copyable factory returns a unique_ptr to a fresh provider on each call.
	template <class Create, class... Required>
	[[nodiscard]] CapabilityRegistration Factory(std::string inImplementation, Create inCreate, CapabilitySlot<Required>... inRequired) const
	{
		Dependencies dependencies;
		(inRequired.Declare(dependencies), ...);
		auto bind = [create = std::move(inCreate), ... slots = std::move(inRequired)](PluginContext &ioContext)
		{
			return BindOwned(ioContext, std::invoke(create, slots.Get(ioContext)...));
		};
		CapabilityRegistration registration(mName, std::move(inImplementation), typeid(std::reference_wrapper<Protocol>), std::move(bind));
		registration.mDependencies = std::move(dependencies);
		return registration;
	}

  private:
	template <class Provider, class Deleter>
	static std::any BindOwned(PluginContext &ioContext, std::unique_ptr<Provider, Deleter> inProvider)
	{
		if (!inProvider)
			return {};
		std::reference_wrapper<Protocol> reference(*inProvider);
		ioContext.Keep(std::move(inProvider));
		return reference;
	}

	std::string mName;
};

namespace detail
{
template <class Callback, class... Protocols>
class InjectedPlugin final : public Plugin
{
  public:
	InjectedPlugin(Callback inCallback, CapabilitySlot<Protocols>... inSlots)
		: mCallback(std::move(inCallback)), mSlots(std::move(inSlots)...) {}
	void Declare(Dependencies &outDependencies) const override
	{
		std::apply([&](const auto &...slots)
				   { (slots.Declare(outDependencies), ...); }, mSlots);
	}

  private:
	void OnMount(PluginContext &ioContext) override
	{
		auto inject = [&](const auto &...slots)
		{
			if constexpr (std::is_invocable_v<Callback &, Protocols &...>)
				std::invoke(mCallback, slots.Get(ioContext)...);
			else
				std::invoke(mCallback, ioContext, slots.Get(ioContext)...);
		};
		std::apply(inject, mSlots);
	}
	Callback								 mCallback;
	std::tuple<CapabilitySlot<Protocols>...> mSlots;
};
} // namespace detail

// The callback takes the declared references, optionally preceded by the mounting context.
template <class Callback, class... Protocols>
[[nodiscard]] auto MakePlugin(Callback inCallback, CapabilitySlot<Protocols>... inSlots)
{
	return detail::InjectedPlugin<Callback, Protocols...>(std::move(inCallback), std::move(inSlots)...);
}

// Copy a default configuration before registering replacements for an experiment.
class CapabilityConfig
{
	friend class PluginContext;

  public:
	void										 Add(CapabilityRegistration inDefault);
	void										 Replace(CapabilityRegistration inReplacement);
	[[nodiscard]] PluginContext					 Build(const Dependencies &inDependencies) const;
	[[nodiscard]] std::unique_ptr<PluginContext> Mount(Plugin &ioPlugin) const;

  private:
	struct Entry
	{
		std::string					   mImplementation;
		std::type_index				   mBindingType;
		CapabilityRegistration::Binder mBind;
		bool						   mReplaced;
		Dependencies				   mDependencies;
	};
	void									  Bind(PluginContext &ioContext, const Dependencies &inDependencies) const;
	std::map<std::string, Entry, std::less<>> mEntries;
};
} // namespace HibikiPhysics
