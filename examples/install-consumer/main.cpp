#include <HibikiPhysics/BuildInfo.h>
#include <cstring>

int main()
{
	return std::strcmp(HibikiPhysics::GetVersion(), "0.1.0");
}
