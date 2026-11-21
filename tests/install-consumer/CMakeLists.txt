cmake_minimum_required(VERSION 4.4)
project(HibikiPhysicsInstalledTests LANGUAGES CXX)
find_package(HibikiPhysics CONFIG REQUIRED)
enable_testing()

add_subdirectory(../../examples/install-consumer examples)
add_executable(hibiki_plugin_lifecycle PluginLifecycle.cpp)
target_link_libraries(hibiki_plugin_lifecycle PRIVATE hibiki::physics)
add_test(NAME plugin.lifecycle COMMAND hibiki_plugin_lifecycle)

add_executable(hibiki_capability_registration CapabilityRegistration.cpp)
target_link_libraries(hibiki_capability_registration PRIVATE hibiki::physics)
add_test(NAME plugin.registration COMMAND hibiki_capability_registration)

add_executable(hibiki_provider_factories ProviderFactories.cpp)
target_link_libraries(hibiki_provider_factories PRIVATE hibiki::physics)
add_test(NAME plugin.factories COMMAND hibiki_provider_factories)

add_executable(hibiki_provider_graph ProviderGraph.cpp)
target_link_libraries(hibiki_provider_graph PRIVATE hibiki::physics)
add_test(NAME plugin.graph COMMAND hibiki_provider_graph)
