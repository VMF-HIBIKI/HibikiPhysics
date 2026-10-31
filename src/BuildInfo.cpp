#include <HibikiPhysics/BuildInfo.h>

namespace HibikiPhysics
{
const char *GetVersion() noexcept
{
	return HIBIKI_VERSION;
}

bool AreInternalAssertionsEnabled() noexcept
{
	return HIBIKI_INTERNAL_ASSERTS != 0;
}
} // namespace HibikiPhysics
