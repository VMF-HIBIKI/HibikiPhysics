#include <HibikiPhysics/BuildInfo.h>
#include <Eigen/Core>
#include <gtest/gtest.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <cfenv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <thread>
#if defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

TEST(Build, LinkedIdentity)
{
	if (std::filesystem::exists("hibiki-controlled-failure.marker"))
	{
		FAIL() << "Hibiki controlled failure";
	}
	EXPECT_STREQ(HibikiPhysics::GetVersion(), "0.1.0");
	EXPECT_EQ(HibikiPhysics::AreInternalAssertionsEnabled(), EXPECT_INTERNAL_ASSERTS != 0);
}

class Arithmetic : public testing::TestWithParam<int>
{
};
TEST_P(Arithmetic, IntegerIdentity)
{
	const int value = GetParam();
	EXPECT_EQ((value + 7) - 7, value);
}
INSTANTIATE_TEST_SUITE_P(SignedValues, Arithmetic, testing::Values(-17, 0, 42));

TEST(Math, EigenReference)
{
	Eigen::Matrix2d matrix;
	matrix << 2.0, 1.0, 3.0, 4.0;
	const Eigen::Vector2d actual = matrix * Eigen::Vector2d(5.0, 6.0);
	EXPECT_EQ(actual.x(), 16.0);
	EXPECT_EQ(actual.y(), 39.0);
}

TEST(Logging, StreamFileAndFlush)
{
	std::ostringstream stream;
	auto			   streamSink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
	auto			   fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("acceptance.log", true);
	spdlog::logger	   logger("acceptance", {streamSink, fileSink});
	logger.info("value={}", 42);
	logger.flush();
	spdlog::info("toolchain console value={}", 42);
	spdlog::default_logger()->flush();
	EXPECT_NE(stream.str().find("value=42"), std::string::npos);
	std::ifstream	  file("acceptance.log");
	std::stringstream contents;
	contents << file.rdbuf();
	EXPECT_NE(contents.str().find("value=42"), std::string::npos);
}

TEST(Numerics, SpecialValuesAndGradualUnderflow)
{
	volatile double zero = 0.0;
	EXPECT_TRUE(std::signbit(-zero));
	EXPECT_TRUE(std::isnan(std::numeric_limits<double>::quiet_NaN()));
	EXPECT_TRUE(std::isinf(std::numeric_limits<double>::infinity()));
	volatile double minimum = std::numeric_limits<double>::min();
	EXPECT_GT(minimum * 0.5, 0.0);
	EXPECT_EQ(std::fegetround(), FE_TONEAREST);
#if defined(_M_X64) || defined(__x86_64__)
	EXPECT_EQ(_mm_getcsr() & ((1U << 15) | (1U << 6)), 0U);
#endif
}

TEST(Numerics, SeparateMultiplyAndAdd)
{
	volatile double inputA = 1.0 + 0x1p-27;
	volatile double inputB = 1.0 - 0x1p-27;
	volatile double inputC = -1.0;
	const double	a = inputA, b = inputB, c = inputC;
	EXPECT_EQ(a * b + c, 0.0);
	EXPECT_EQ(std::fma(a, b, c), -0x1p-54);
}

TEST(Numerics, RoundingScopeAndWorker)
{
	const int callerMode = std::fegetround();
	ASSERT_EQ(std::fesetround(FE_DOWNWARD), 0);
	const int savedMode = std::fegetround();
	ASSERT_EQ(std::fesetround(FE_TONEAREST), 0);
	EXPECT_EQ(std::fegetround(), FE_TONEAREST);
	ASSERT_EQ(std::fesetround(savedMode), 0);
	EXPECT_EQ(std::fegetround(), FE_DOWNWARD);
	ASSERT_EQ(std::fesetround(callerMode), 0);
	int			workerMode = -1;
	std::thread worker([&workerMode]
					   { workerMode = std::fegetround(); });
	worker.join();
	EXPECT_EQ(workerMode, FE_TONEAREST);
}
