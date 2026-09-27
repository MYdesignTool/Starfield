cmake_minimum_required(VERSION 3.24)

project(StarfieldCore VERSION 0.1.0 LANGUAGES CXX)

add_library(starfield_core STATIC
    src/core/CpuRenderer.cpp
    src/core/Geometry.cpp
    src/core/Graph.cpp
    src/core/ParticleSimulation.cpp
    src/core/Random.cpp
    src/core/Render.cpp
    src/core/Settings.cpp
    src/core/Time.cpp
)
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
    target_link_libraries(starfield_core_tests PRIVATE starfield_core)
    add_test(NAME starfield_core_tests COMMAND starfield_core_tests)
endif()
