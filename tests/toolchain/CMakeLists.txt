add_executable(hibiki_toolchain_tests ToolchainTests.cpp)
target_link_libraries(hibiki_toolchain_tests PRIVATE hibiki::physics GTest::gtest_main Eigen3::Eigen spdlog::spdlog)
target_compile_definitions(hibiki_toolchain_tests PRIVATE EXPECT_INTERNAL_ASSERTS=$<NOT:$<CONFIG:Release>>)
include(GoogleTest)
gtest_discover_tests(hibiki_toolchain_tests DISCOVERY_MODE PRE_TEST)

add_executable(hibiki_benchmark Benchmark.cpp)
target_link_libraries(hibiki_benchmark PRIVATE benchmark::benchmark_main)
add_test(NAME benchmark.smoke COMMAND hibiki_benchmark --benchmark_min_time=0.01s --benchmark_out=benchmark.json --benchmark_out_format=json)

add_executable(hibiki_debug_probe DebugProbe.cpp)
target_link_libraries(hibiki_debug_probe PRIVATE hibiki::physics)
add_test(NAME debug.normal COMMAND hibiki_debug_probe)

if(HIBIKI_ASAN)
	add_executable(hibiki_asan_probe AsanProbe.cpp)
endif()
