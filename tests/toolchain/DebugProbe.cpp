#include <HibikiPhysics/BuildInfo.h>
#include <string>
#include <vector>

int main()
{
	std::vector<int>  values{2, 3, 5};
	const std::string version = HibikiPhysics::GetVersion();
	const int		  sum = values[0] + values[1] + values[2];
	return sum == 10 && version == "0.1.0" ? 0 : 1;
}
