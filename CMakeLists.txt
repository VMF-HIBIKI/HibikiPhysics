cmake_minimum_required(VERSION 4.4)
project(HibikiPhysics VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
option(HIBIKI_BUILD_ACCEPTANCE "Build toolchain acceptance programs" ON)
option(HIBIKI_ASAN "Instrument the build with AddressSanitizer" OFF)
set(HIBIKI_SOURCE_ARCHIVES "" CACHE PATH "Verified dependency archive directory")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(BUILD_SHARED_LIBS OFF)

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${HIBIKI_ASAN}>>>:Debug>DLL")
	add_compile_options(/Zi /EHsc /GR)
	add_link_options(/DEBUG /INCREMENTAL:NO)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(/clang:-fno-fast-math /clang:-ffp-contract=off /clang:-march=x86-64)
		if(HIBIKI_ASAN)
			add_compile_options(/fsanitize=address /Od /clang:-fno-omit-frame-pointer)
		endif()
	elseif(HIBIKI_ASAN)
		message(FATAL_ERROR "The ASan preset selects clang-cl")
	else()
		add_compile_options(/fp:strict)
	endif()
else()
	add_compile_options(-g -fno-fast-math -ffp-contract=off)
	if(HIBIKI_ASAN)
		add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
		add_link_options(-fsanitize=address)
	endif()
endif()

add_library(hibiki_physics STATIC src/BuildInfo.cpp)
add_library(hibiki::physics ALIAS hibiki_physics)
set_target_properties(hibiki_physics PROPERTIES EXPORT_NAME physics)
target_compile_features(hibiki_physics PUBLIC cxx_std_20)
target_include_directories(hibiki_physics PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(hibiki_physics PRIVATE
	HIBIKI_VERSION="${PROJECT_VERSION}"
	HIBIKI_INTERNAL_ASSERTS=$<NOT:$<CONFIG:Release>>)
if(MSVC)
	target_compile_options(hibiki_physics PRIVATE /W4 /WX)
else()
	target_compile_options(hibiki_physics PRIVATE -Wall -Wextra -Werror)
endif()

install(TARGETS hibiki_physics EXPORT HibikiPhysicsTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
install(EXPORT HibikiPhysicsTargets NAMESPACE hibiki:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HibikiPhysics)
configure_package_config_file(cmake/HibikiPhysicsConfig.cmake.in
	"${PROJECT_BINARY_DIR}/HibikiPhysicsConfig.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HibikiPhysics)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/HibikiPhysicsConfigVersion.cmake"
	VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${PROJECT_BINARY_DIR}/HibikiPhysicsConfig.cmake"
	"${PROJECT_BINARY_DIR}/HibikiPhysicsConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HibikiPhysics)

if(HIBIKI_BUILD_ACCEPTANCE)
	include(cmake/Dependencies.cmake)
	hibiki_declare_dependencies("${HIBIKI_SOURCE_ARCHIVES}")
	hibiki_dependency_options()
	include(FetchContent)
	FetchContent_MakeAvailable(googletest eigen spdlog benchmark)
	add_subdirectory(tests/toolchain)
endif()
