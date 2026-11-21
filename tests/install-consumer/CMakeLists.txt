cmake_minimum_required(VERSION 4.4)
project(HibikiPhysicsInstalledTests LANGUAGES CXX)
find_package(HibikiPhysics CONFIG REQUIRED)
enable_testing()

add_executable(hibiki_dense_sdf_grid DenseSdfGrid.cpp)
target_link_libraries(hibiki_dense_sdf_grid PRIVATE hibiki::physics)
add_test(NAME geometry.dense_sdf_grid COMMAND hibiki_dense_sdf_grid)

add_executable(hibiki_morton_spatial_index MortonSpatialIndex.cpp)
target_link_libraries(hibiki_morton_spatial_index PRIVATE hibiki::physics)
add_test(NAME geometry.morton_spatial_index COMMAND hibiki_morton_spatial_index)

add_executable(hibiki_extended_shapes ExtendedShapes.cpp)
target_link_libraries(hibiki_extended_shapes PRIVATE hibiki::physics)
add_test(NAME geometry.extended COMMAND hibiki_extended_shapes)

add_subdirectory(../../examples/install-consumer examples)
add_executable(hibiki_triangle_queries TriangleMeshQueries.cpp)
target_link_libraries(hibiki_triangle_queries PRIVATE hibiki::physics)
add_test(NAME geometry.triangle_queries COMMAND hibiki_triangle_queries)

add_executable(hibiki_closed_mesh_sdf ClosedMeshSdf.cpp)
target_link_libraries(hibiki_closed_mesh_sdf PRIVATE hibiki::physics)
add_test(NAME geometry.closed_mesh_sdf COMMAND hibiki_closed_mesh_sdf)

add_executable(hibiki_plugin_lifecycle PluginLifecycle.cpp)
target_link_libraries(hibiki_plugin_lifecycle PRIVATE hibiki::physics)
add_test(NAME plugin.lifecycle COMMAND hibiki_plugin_lifecycle)

add_executable(hibiki_stackless_spatial_index StacklessSpatialIndex.cpp)
target_link_libraries(hibiki_stackless_spatial_index PRIVATE hibiki::physics)
add_test(NAME geometry.stackless_spatial_index COMMAND hibiki_stackless_spatial_index)

add_executable(hibiki_capability_registration CapabilityRegistration.cpp)
target_link_libraries(hibiki_capability_registration PRIVATE hibiki::physics)
add_test(NAME plugin.registration COMMAND hibiki_capability_registration)

add_executable(hibiki_discrete_shapes DiscreteShapes.cpp)
target_link_libraries(hibiki_discrete_shapes PRIVATE hibiki::physics)
add_test(NAME geometry.discrete COMMAND hibiki_discrete_shapes)

add_executable(hibiki_provider_factories ProviderFactories.cpp)
target_link_libraries(hibiki_provider_factories PRIVATE hibiki::physics)
add_test(NAME plugin.factories COMMAND hibiki_provider_factories)

add_executable(hibiki_spatial_index SpatialIndex.cpp)
target_link_libraries(hibiki_spatial_index PRIVATE hibiki::physics)
add_test(NAME geometry.spatial_index COMMAND hibiki_spatial_index)

add_executable(hibiki_four_way_spatial_index FourWaySpatialIndex.cpp)
target_link_libraries(hibiki_four_way_spatial_index PRIVATE hibiki::physics)
add_test(NAME spatial.four_way COMMAND hibiki_four_way_spatial_index)

add_executable(hibiki_provider_graph ProviderGraph.cpp)
target_link_libraries(hibiki_provider_graph PRIVATE hibiki::physics)
add_test(NAME plugin.graph COMMAND hibiki_provider_graph)

add_executable(hibiki_solid_validation SolidMeshValidation.cpp)
target_link_libraries(hibiki_solid_validation PRIVATE hibiki::physics)
add_test(NAME geometry.solid_validation COMMAND hibiki_solid_validation)

add_executable(hibiki_geometry_plugin GeometryPlugin.cpp)
target_link_libraries(hibiki_geometry_plugin PRIVATE hibiki::physics)
add_test(NAME plugin.geometry COMMAND hibiki_geometry_plugin)

add_executable(hibiki_analytic_shapes AnalyticShapes.cpp)
target_link_libraries(hibiki_analytic_shapes PRIVATE hibiki::physics)
add_test(NAME geometry.analytic COMMAND hibiki_analytic_shapes)

add_executable(hibiki_sweep_and_prune SweepAndPrune.cpp)
target_link_libraries(hibiki_sweep_and_prune PRIVATE hibiki::physics)
add_test(NAME spatial.sweep_and_prune COMMAND hibiki_sweep_and_prune)

add_executable(hibiki_height_field HeightField.cpp)
target_link_libraries(hibiki_height_field PRIVATE hibiki::physics)
add_test(NAME geometry.height_field COMMAND hibiki_height_field)

add_executable(hibiki_filtered_spatial FilteredSpatialIndex.cpp)
target_link_libraries(hibiki_filtered_spatial PRIVATE hibiki::physics)
add_test(NAME spatial.filtered COMMAND hibiki_filtered_spatial)

add_executable(hibiki_sdf_baker SdfBaker.cpp)
target_link_libraries(hibiki_sdf_baker PRIVATE hibiki::physics)
add_test(NAME sdf.baker COMMAND hibiki_sdf_baker)

add_executable(hibiki_batch_overlap BatchedOverlap.cpp)
target_link_libraries(hibiki_batch_overlap PRIVATE hibiki::physics)
add_test(NAME spatial.batch COMMAND hibiki_batch_overlap)

add_executable(hibiki_sdf_accuracy SdfAccuracy.cpp)
target_link_libraries(hibiki_sdf_accuracy PRIVATE hibiki::physics)
target_compile_definitions(hibiki_sdf_accuracy PRIVATE
    HIBIKI_EXPERIMENT_BUILD="$<CONFIG>"
    HIBIKI_EXPERIMENT_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(WIN32)
    target_link_libraries(hibiki_sdf_accuracy PRIVATE psapi)
endif()
add_test(NAME sdf.plate_accuracy_smoke COMMAND hibiki_sdf_accuracy 2.5 0.5)
