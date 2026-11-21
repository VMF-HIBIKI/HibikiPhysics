add_executable(hibiki_rigid_math_tests RigidMathTests.cpp)
target_link_libraries(hibiki_rigid_math_tests PRIVATE hibiki::physics Eigen3::Eigen GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(hibiki_rigid_math_tests DISCOVERY_MODE PRE_TEST)
