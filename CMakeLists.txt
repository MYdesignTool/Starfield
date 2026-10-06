cmake_minimum_required(VERSION 3.24)

project(StarfieldCore VERSION 0.1.0 LANGUAGES CXX)

add_library(starfield_core STATIC
    src/core/CpuRenderer.cpp
    src/core/Geometry.cpp
    src/core/Graph.cpp
    src/core/GraphConstruction.cpp
    src/core/GraphEvaluation.cpp
    src/core/EmitterHistory.cpp
    src/core/ParticleSimulation.cpp
    src/core/ParticleTransform.cpp
    src/core/Random.cpp
    src/core/Render.cpp
    src/core/SequenceCodec.cpp
    src/core/Settings.cpp
    src/core/Time.cpp
)

# The AE adapter loads this DLL by versioned filename; it never links an import
# library. The shared target is useful for portable ABI checks outside MSBuild.
add_library(starfield_core_dll SHARED src/core/PluginApi.cpp)
target_link_libraries(starfield_core_dll PRIVATE starfield_core)
target_include_directories(starfield_core_dll PRIVATE include)
target_compile_features(starfield_core_dll PRIVATE cxx_std_20)
target_compile_definitions(starfield_core_dll PRIVATE SF_CORE_BUILD_DLL)
set_target_properties(starfield_core_dll PROPERTIES OUTPUT_NAME StarfieldCore)
target_include_directories(starfield_core PUBLIC include)
target_compile_features(starfield_core PUBLIC cxx_std_20)
set_target_properties(starfield_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(starfield_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(starfield_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

# Host-independent self-tests. The AE module itself is built by
# ae_plugin/BuildWindows.ps1, which also runs the SDK's PiPL tool.
include(CTest)
if(BUILD_TESTING)
    add_executable(starfield_core_tests tests/core_tests.cpp)
    target_sources(starfield_core_tests PRIVATE src/core/PluginApi.cpp)
    target_link_libraries(starfield_core_tests PRIVATE starfield_core)
    add_test(NAME starfield_core_tests COMMAND starfield_core_tests)
    add_executable(starfield_particle_transform_tests tests/particle_transform_tests.cpp)
    target_link_libraries(starfield_particle_transform_tests PRIVATE starfield_core)
    add_test(NAME starfield_particle_transform_tests COMMAND starfield_particle_transform_tests)
endif()
