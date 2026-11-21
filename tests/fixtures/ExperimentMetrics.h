#pragma once

#include <HibikiPhysics/DenseSdfGrid.h>
#include <bit>
#include <cstdint>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace HibikiPhysics::Test
{
inline std::uint64_t PeakResidentBytes()
{
#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS counters{};
	if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
		throw std::runtime_error("peak working set unavailable");
	return counters.PeakWorkingSetSize;
#else
	rusage usage{};
	if (getrusage(RUSAGE_SELF, &usage) != 0)
		throw std::runtime_error("peak resident memory unavailable");
#ifdef __APPLE__
	return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
	return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#endif
#endif
}

// A word-wise fingerprint covers every binary64 node, including padding.
inline std::uint64_t GridFingerprint(const DenseSdfGrid &grid)
{
	std::uint64_t hash = 14695981039346656037ULL;
	for (double value : grid.GetValues())
		hash = (hash ^ std::bit_cast<std::uint64_t>(value)) * 1099511628211ULL;
	return hash;
}
} // namespace HibikiPhysics::Test
