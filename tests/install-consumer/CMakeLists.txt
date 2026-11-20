cmake_minimum_required(VERSION 4.4)
project(HibikiPhysicsInstalledTests LANGUAGES CXX)
find_package(HibikiPhysics CONFIG REQUIRED)
enable_testing()

add_subdirectory(../../examples/install-consumer examples)
add_executable(hibiki_plugin_lifecycle PluginLifecycle.cpp)
target_link_libraries(hibiki_plugin_lifecycle PRIVATE hibiki::physics)
add_test(NAME plugin.lifecycle COMMAND hibiki_plugin_lifecycle)
