#include <benchmark/benchmark.h>
#include <array>

static void sArraySum(benchmark::State &inState)
{
	std::array<double, 256> values{};
	values.fill(0.5);
	for (auto iteration : inState)
	{
		(void)iteration;
		benchmark::DoNotOptimize(values.data());
		double sum = 0.0;
		for (double value : values)
			sum += value;
		benchmark::DoNotOptimize(sum);
	}
}
BENCHMARK(sArraySum);
