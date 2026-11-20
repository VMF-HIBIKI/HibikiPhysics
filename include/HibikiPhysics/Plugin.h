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
  public:
	virtual ~Plugin() = default;

	// Called during scene compilation. The default declares zero dependencies.
	virtual void Declare(Dependencies &) const {}
	// Called with this plugin's resolved bindings. The host owns the scope.
	virtual void Mount(PluginContext &ioContext) = 0;
};
} // namespace HibikiPhysics
