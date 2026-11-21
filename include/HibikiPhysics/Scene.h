#pragma once

#include <HibikiPhysics/Plugin.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <istream>

namespace HibikiPhysics
{
struct CollisionFilter
{
	std::uint32_t mGroup = 1;
	std::uint32_t mMask = 1;
};

struct SceneSphere
{
	std::string		mName;
	double			mCenterZ = 0;
	double			mRadius = 1;
	CollisionFilter mFilter;
};

struct ScenePlane
{
	std::string		mName;
	double			mHeight = 0;
	CollisionFilter mFilter;
};

// Native scene v1 supports one sphere and one horizontal plane, both optional.
struct SceneDescription
{
	std::string				   mSource;
	std::optional<SceneSphere> mSphere;
	std::optional<ScenePlane>  mPlane;
};

[[nodiscard]] SceneDescription ReadScene(std::istream &inStream, std::string inSource);
[[nodiscard]] SceneDescription LoadScene(const std::filesystem::path &inPath);

// Return the signed surface separation when within the detector's contact range.
using SpherePlaneDetector = std::function<std::optional<double>(const SceneSphere &, const ScenePlane &, double)>;
class CollisionService;
class SpherePlaneRegistration
{
	friend class CollisionService;
	explicit SpherePlaneRegistration(SpherePlaneDetector inDetector)
		: mDetector(std::make_shared<const SpherePlaneDetector>(std::move(inDetector))) {}
	std::shared_ptr<const SpherePlaneDetector> mDetector;

  public:
	virtual ~SpherePlaneRegistration() = default;
	SpherePlaneRegistration(const SpherePlaneRegistration &) = delete;
	SpherePlaneRegistration &operator=(const SpherePlaneRegistration &) = delete;

  protected:
	SpherePlaneRegistration() = default;
};

// Each registration owns its detector. Releasing the registration removes it from the service.
class CollisionService
{
  public:
	CollisionService() = default;
	virtual ~CollisionService() = default;
	CollisionService(const CollisionService &) = delete;
	CollisionService											  &operator=(const CollisionService &) = delete;
	[[nodiscard]] virtual std::unique_ptr<SpherePlaneRegistration> RegisterSpherePlane(SpherePlaneDetector inDetector);
	[[nodiscard]] virtual std::optional<double>					   Query(const SceneSphere &inSphere, const ScenePlane &inPlane, double inCenterZ) const;

  private:
	std::weak_ptr<const SpherePlaneDetector> mDetector;
};

inline const CapabilitySlot<CollisionService>	 CollisionServiceSlot("collision.service");
inline const CapabilitySlot<SpherePlaneDetector> SpherePlaneSlot("collision.sphere-plane");
[[nodiscard]] CapabilityConfig					 DefaultSceneConfig();
// Include separated contacts up to a finite, nonnegative distance (metres).
[[nodiscard]] CapabilityRegistration SpherePlaneWithMargin(std::string inImplementation, double inMargin);

class CompiledScene
{
  public:
	explicit CompiledScene(SceneDescription inScene, const CapabilityConfig &inConfig = DefaultSceneConfig());
	[[nodiscard]] const SceneDescription				   &GetSource() const noexcept { return mScene; }
	[[nodiscard]] std::span<const PluginContext::Selection> GetSelections() const noexcept { return mScope ? mScope->GetSelections() : std::span<const PluginContext::Selection>{}; }
	// Query this step's sphere centre against the compiled plane (metres, +Z up).
	[[nodiscard]] std::optional<double> QueryContact(double inCenterZ) const;

  private:
	SceneDescription			   mScene;
	std::unique_ptr<PluginContext> mScope;
	CollisionService			  *mService = nullptr;
};
} // namespace HibikiPhysics
